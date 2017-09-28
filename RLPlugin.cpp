#include "stdafx.h"
#pragma once
#include "RLPlugin.h"
#include <fstream>

RLPlugin::RLPlugin():IBehaviourPlugin(GameDebugParams(20,false,false, false,false,0.5f)),
	m_bDecide(true),
	m_RoamTarget(b2Vec2_zero),
	m_HouseTarget(b2Vec2_zero),
	m_bWasPreviouslyInHouse(false),
	m_ExtraReward(0),
	m_bWasShooting(false),
	m_LootTarget(b2Vec2_zero),
	m_bWasLooting(false),
	m_Timer(0),
	m_DeltaTime(0),
	m_ShootingTimer(0)
{
}

RLPlugin::~RLPlugin()
{
}

void RLPlugin::Start()
{
	//Set up inventory
	for (int i = 0; i < INVENTORY_GetCapacity(); ++i)
	{
		m_OpenInventory.push_back(i);
	}

	//Load in the RL table
	RL.Parse("QLearningTable.txt");
	Iteration++;

	//Initial observation
	m_Observation = Observe();

	//Choose an initial action
	m_Action = RL.ChooseAction(m_Observation);
	m_nextAction = m_Action;
}

PluginOutput RLPlugin::Update(float dt)
{
	m_World = WORLD_GetInfo();
	m_Agent = AGENT_GetInfo();
	m_FOVEntities = FOV_GetEntities();
	m_FOVHouses = FOV_GetHouses();
	m_Timer += dt;
	m_DeltaTime = dt;
	State nObs = Observe();
	if(m_bDecide)
	{
		//Get next observation and calculate reward
		int reward = CalculateReward(nObs);

		////RL learn from transition
		///QLearning
		RL.Learn(m_Observation, m_Action, reward, nObs, false);
		///Sarsa
		//RL.Learn(m_Observation, m_Action, reward, nObs, m_nextAction, false);
		///Sarsa(lambda)
		//RL.Learn(m_Observation, m_Action, reward, nObs, m_nextAction, true);
		///Dyna Q WIP
		//RL.Learn(m_Observation, m_Action, reward, nObs, true);

		//Swap observation
		m_Observation = nObs;
		m_Action = m_nextAction;

		//Choose next action
		m_nextAction = RL.ChooseAction(m_Observation);
	}
	//Do the action
	PluginOutput output = RunAction(m_Action);
	m_bWasPreviouslyInHouse = m_Agent.IsInHouse;

	//Check if dead
	if(m_Agent.Health <= 1)
	{
		Reset();
	}
	return output;
}

//Extend the UI [ImGui call only!]
void RLPlugin::ExtendUI_ImGui()
{
	ImGui::Text("Selected Action: %i", static_cast<int>(m_Action));
}

void RLPlugin::End()
{
	//Save RL Table
	RL.Save("QLearningTable.txt");
}

//[Optional]> For Debugging
void RLPlugin::ProcessEvents(const SDL_Event& e)
{
	
}

State RLPlugin::Observe()
{
	State observation;

	//Dead
	if(m_Agent.Bitten || m_Agent.Death)
		observation.bIsDead = true;
	else observation.bIsDead = false;

	//Health
	if (m_Agent.Health > 6.6f || m_Agent.Health < -1)
		observation.Health = Good;
	else if (m_Agent.Health > 3.3f)
		observation.Health = Medium;
	else
		observation.Health = Bad;

	//Hunger
	if (m_Agent.Energy > 14 || m_Agent.Health < -1)
		observation.Hunger = Full;
	else if (m_Agent.Energy > 7)
		observation.Hunger = Hungry;
	else
		observation.Hunger = Starving;

	//Houses
	for (auto ent : m_FOVHouses)
	{
		bool spotted = false;
		for (auto en : m_HousesSeen)
		{
			if (en.first.Center == ent.Center)
			{
				spotted = true;
				break;
			}
		}
		if(!spotted)
			m_HousesSeen.push_back(std::pair<HouseInfo,double>(ent,(std::numeric_limits<double>::max)()));
	}
	observation.Houses = m_HousesSeen.size();

	//Danger
	for (auto ent : m_FOVEntities)
	{
		if (ent.Type != ENEMY) continue;
		EnemyInfo i;
		ENEMY_GetInfo(ent, i);
		bool spotted = false;
		for (auto en : m_EnemiesSeen)
		{
			if (i.EnemyHash == en.info.EnemyHash)
			{
				spotted = true;
				en.lastPos = ent.Position;
				en.timeSpotted = m_Timer;
				break;
			}
		}
		if (spotted) continue;
		EnemySeen1 t;
		t.lastPos = ent.Position;
		t.info = i;
		t.timeSpotted = m_Timer;
		m_EnemiesSeen.push_back(t);
	}
	int dangerScore = 0;
	std::vector<EnemySeen1>::iterator it = m_EnemiesSeen.begin();
	while ( it != m_EnemiesSeen.end())
	{
		if ((*it).info.Health <= 0)
			 it = m_EnemiesSeen.erase(it);
		else ++it;
	}
	for (EnemySeen1 element : m_EnemiesSeen){
		dangerScore += Distance(m_Agent.Position, element.lastPos) / (m_Timer - element.timeSpotted);
	}
	if (dangerScore > 200)
		observation.Danger = Extreme;
	else if (dangerScore > 50)
		observation.Danger = High;
	else if (dangerScore > 10)
		observation.Danger = Mediocre;
	else
		observation.Danger = Low;

	//Guns
	if (m_GunsInventory.size() != 0) {
		std::vector<std::pair<ItemInfo, int>>::iterator it = m_GunsInventory.begin();
		while(it != m_GunsInventory.end())
		{
			int ammo;
			ITEM_GetMetadata((*it).first, "ammo", ammo);
			if (ammo <= 0)
			{
				INVENTORY_RemoveItem((*it).second);
				m_OpenInventory.push_back((*it).second);
				it = m_GunsInventory.erase(it);
				m_bDecide = true;
			}
			else ++it;
		}
	}
	observation.Guns = m_GunsInventory.size();
	return observation;
}

PluginOutput RLPlugin::RunAction(Action action)
{
	//Output
	PluginOutput output = {};
	m_bDecide = false;
	output.RunMode = false;
	switch (action)
	{
	case ROAM:
		{
			if (m_Agent.CurrentLinearSpeed < 5) m_bDecide = true;
			//if not set, get random pos
			if(m_RoamTarget == b2Vec2_zero)
			{
				m_ExtraReward -= 1;
				m_RoamTarget = m_Agent.Position;
				m_RoamTarget += (randomVector2(10) - b2Vec2(5,5));
			}
			//get velocity
			auto targetVelocity = NAVMESH_GetClosestPathPoint(m_RoamTarget) - m_Agent.Position;
			//get velocity length
			auto distance = targetVelocity.Normalize();
			//if close to target, reset target and let RL decide
			if (distance < 1)
			{
				m_RoamTarget = b2Vec2_zero; 
				m_bDecide = true;
			}
			//lengthen velocity with maxlinearspeed
			targetVelocity *= m_Agent.MaxLinearSpeed;
			//set output
			output.LinearVelocity = targetVelocity - m_Agent.LinearVelocity;
			break;
		}
	case VISIT_HOUSE:
		{
			if(m_HousesSeen.size() == 0)
			{
				m_ExtraReward -= 10;
				m_bDecide = true;
				break;
			}
			
			//if not set, get closest house longest not looted
			if(m_HouseTarget == b2Vec2_zero)
			{
				m_ExtraReward -= 1;
				if (m_Agent.IsInHouse) m_ExtraReward -= 5;
				std::pair<HouseInfo, double> bestHouse;
				int bestHouseIt = 0;
				double bestHouseDistTime = (std::numeric_limits<double>::max)();
				for (std::pair<HouseInfo, double> house : m_HousesSeen){
					auto distance = (house.first.Center - m_Agent.Position).Normalize();
					auto time = std::abs(m_Timer - house.second);
					if (distance / time < bestHouseDistTime) {
						bestHouse = house; bestHouseDistTime = distance / time;	m_BestHouseIt = bestHouseIt;}
					bestHouseIt++;
				}
				m_HouseTarget = bestHouse.first.Center;
			}
			//if reached, set timer of best house, reset target and let RL decide
			if((!m_bWasPreviouslyInHouse && m_Agent.IsInHouse) || (m_HouseTarget - m_Agent.Position).Normalize() < 2)
			{
				m_HousesSeen[m_BestHouseIt].second = m_Timer;
				m_HouseTarget = b2Vec2_zero;
				m_bDecide = true;
			}
			//go to target
			auto targetVelocity = NAVMESH_GetClosestPathPoint(m_HouseTarget) - m_Agent.Position;
			auto distance = targetVelocity.Normalize();
			targetVelocity *= m_Agent.MaxLinearSpeed;

			output.LinearVelocity = targetVelocity - m_Agent.LinearVelocity;
			break;
		}
	case DEFEND:
		{
			//Check if enemy spotted
			if(m_EnemiesSeen.size() == 0)
			{
				if(!m_bWasShooting)m_ExtraReward -= 10;
				m_bDecide = true;
				m_bWasShooting = false;
				m_ShootingTimer = 0;
				break;
			}
			
			//Check if we have guns
			if(m_GunsInventory.size() == 0)
			{
				//Reset shooting
				m_bWasShooting = false;
				//Tactical retreat
				output.RunMode = true;
				b2Vec2 runTarget = m_Agent.Position;
				for (auto enemy : m_EnemiesSeen){
					auto run =  m_Agent.Position - enemy.lastPos;
					auto distance = run.Normalize();
					runTarget += run*(5 / distance);
				}
				auto targetVelocity = runTarget - m_Agent.Position;
				auto dist = targetVelocity.Normalize();
				targetVelocity *= m_Agent.MaxLinearSpeed;
				//if safe, decide next action
				if(dist > 5)
				{
					m_bDecide = true;
					m_ShootingTimer = 0;
				}
				//move
				output.LinearVelocity = targetVelocity - m_Agent.LinearVelocity;
			}
			else
			{
				if(m_bWasShooting && m_ShootingTimer > 0.5f)
				{
					//Shoot
					INVENTORY_UseItem(m_GunsInventory[0].second);
				}
				//Aim
				EnemySeen1 shootTarget;
				auto shortestDist = (std::numeric_limits<float32>::max)();
				for (auto enemy : m_EnemiesSeen) {
					auto run = enemy.lastPos - m_Agent.Position;
					auto distance = run.Normalize();
					if (distance < shortestDist) shootTarget = enemy;
				}
				auto targetVelocity = shootTarget.lastPos - m_Agent.Position;
				auto distance = targetVelocity.Normalize();
				output.LinearVelocity = targetVelocity - m_Agent.LinearVelocity;
				output.AngularVelocity = m_Agent.MaxAngularSpeed;
				m_bWasShooting = true;
				m_ShootingTimer += m_DeltaTime;
				if(distance > 10)
				{
					m_bDecide = true;
					m_bWasShooting = false;
					m_ShootingTimer = 0;
				}
			}
			//Clean out enemylist
			std::vector<EnemySeen1>::iterator it = m_EnemiesSeen.begin();
			while( it != m_EnemiesSeen.end()) {
				if(((*it).lastPos - m_Agent.Position).Normalize() > 25 || (m_Timer - (*it).timeSpotted) > 30)
				{
					it = m_EnemiesSeen.erase(it);
				}
				else ++it;
			}
			break;
		}
	case LOOT:
		{
			if(m_OpenInventory.size() <= 0)
			{
				if(!m_bWasLooting)m_ExtraReward -= 10;
				m_bDecide = true;
				break;
			}
			//have loot target?
			if(m_LootTarget == b2Vec2_zero)
			{
				m_ExtraReward += 5;
				if (m_Agent.IsInHouse) m_ExtraReward += 5;
				if(m_FOVEntities.size() == 0)
				{
					if(!m_bWasLooting)m_ExtraReward -= 15;
					m_bDecide = true;
					break;
				}
				//get loot target
				double closestDist = (std::numeric_limits<double>::max)();
				for (EntityInfo element : m_FOVEntities){
					//Check if enemy
					if (element.Type == ENEMY) continue;
					//Check if garbage
					std::vector<b2Vec2>::iterator it;
					it = std::find(m_GarbagePositions.begin(), m_GarbagePositions.end(), element.Position);
					if (it != m_GarbagePositions.end()) continue;
					//Check if closest
					auto run = element.Position - m_Agent.Position;
					auto distance = run.Normalize();
					if(distance < closestDist)
					{
						m_ClosestLoot = element;
						closestDist = distance;
					}
				}
				if(closestDist == (std::numeric_limits<double>::max)()) //Nothing but garbage
				{
					m_ExtraReward -= 10;
					m_bDecide = true;
					break;
				}
				m_LootTarget = m_ClosestLoot.Position;
			}
			m_bWasLooting = false;
			//if close to loot
			if ((m_LootTarget - m_Agent.Position).Normalize() < m_Agent.GrabRange)
			{
				ItemInfo item;
				if (ITEM_Grab(m_ClosestLoot,item))
				{
					switch (item.Type)
					{
					case PISTOL: 
						m_GunsInventory.push_back(std::pair<ItemInfo, int>(item, m_OpenInventory.back()));
						INVENTORY_AddItem(m_OpenInventory.back(), item);
						m_OpenInventory.pop_back();
						m_ExtraReward += 5;
						break;
					case HEALTH:
						m_HealthInventory.push_back(std::pair<ItemInfo, int>(item, m_OpenInventory.back()));
						INVENTORY_AddItem(m_OpenInventory.back(), item);
						m_OpenInventory.pop_back();
						m_ExtraReward += 5;
						break;
					case FOOD: 
						m_FoodInventory.push_back(std::pair<ItemInfo, int>(item, m_OpenInventory.back()));
						INVENTORY_AddItem(m_OpenInventory.back(), item);
						m_OpenInventory.pop_back();
						m_ExtraReward += 5;
						break;
					case GARBAGE: 
						m_GarbagePositions.push_back(m_LootTarget);
						break;
					default: break;
					}
					m_LootTarget = b2Vec2_zero;
					m_bWasLooting = true;
					m_bDecide = true;
				}
			}
			//go to loot
			auto targetVelocity = NAVMESH_GetClosestPathPoint(m_LootTarget) - m_Agent.Position;
			auto distance = targetVelocity.Normalize();
			targetVelocity *= m_Agent.MaxLinearSpeed;
			output.LinearVelocity = targetVelocity - m_Agent.LinearVelocity;
			break;
		}
	case FIX_VITALS:
		{
			//Need healing?
			if(m_Observation.Health != Good)
			{
				//Have potions?
				if(m_HealthInventory.size() <= 0)
				{
					m_ExtraReward -= 5;
				}
				else
				{
					INVENTORY_UseItem(m_HealthInventory[m_HealthInventory.size() - 1].second);
					m_ExtraReward += 15;
				}
			}
			//Need Energy?
			if(m_Observation.Hunger != Full)
			{
				//Have food?
				if(m_FoodInventory.size() <= 0)
				{
					m_ExtraReward -= 5;
				}
				else
				{
					INVENTORY_UseItem(m_FoodInventory[m_FoodInventory.size() - 1].second);
					m_ExtraReward += 15;
				}
			}
			if(m_Observation.Hunger == Full && m_Observation.Health == Good)
			{
				m_ExtraReward -= 10;
			}
			//Clean inventory
			if (m_HealthInventory.size() != 0)
			{
				std::vector<std::pair<ItemInfo, int>>::iterator it = m_HealthInventory.begin();
				while (it != m_HealthInventory.end())
				{
					int health;
					ITEM_GetMetadata((*it).first, "health", health);
					if (health <= 0)
					{
						INVENTORY_RemoveItem((*it).second);
						m_OpenInventory.push_back((*it).second);
						it = m_HealthInventory.erase(it);
					}
					else ++it;
				}
			}
			if (m_FoodInventory.size() != 0) {
				std::vector<std::pair<ItemInfo, int>>::iterator it = m_FoodInventory.begin();
				while (it != m_FoodInventory.end())
				{
					int energy;
					ITEM_GetMetadata((*it).first, "energy", energy);
					if (energy <= 0)
					{
						INVENTORY_RemoveItem((*it).second);
						m_OpenInventory.push_back((*it).second);
						it = m_FoodInventory.erase(it);
					}
					else ++it;
				}
			}
			//Let RL decide
			m_bDecide = true;
			break;
		}
	default: m_bDecide = true;
	}
	return output;
}

int RLPlugin::CalculateReward(State newObservation)
{
	int reward = m_ExtraReward;
	if (newObservation.bIsDead) reward -= 100;
	if (newObservation.Guns > m_Observation.Guns) reward += 10;
	if (newObservation.Health > m_Observation.Health) reward -= 10;
	else if (newObservation.Health < m_Observation.Health) reward += 10;
	if (newObservation.Hunger > m_Observation.Hunger) reward -= 10;
	else if (newObservation.Hunger < m_Observation.Hunger) reward += 10;
	if (newObservation.Danger > m_Observation.Danger) reward -= 5;
	else if (newObservation.Danger < m_Observation.Danger) reward += 5;
	m_ExtraReward = 0;
	return reward;
}

void RLPlugin::Reset()
{
	//kill
	End();
	Results();
	std::exit(EXIT_SUCCESS);
}

void RLPlugin::Results()
{
	std::string file = "Results.txt";
	std::ofstream ofile; //Append what's inside
	ofile.open(file.c_str(), std::ofstream::out | std::ofstream::app);
	if (ofile)
	{
		ofile << Iteration << " " << m_Timer << "\n";
	}
}

