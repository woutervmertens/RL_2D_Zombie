#include "stdafx.h"
#pragma once
#include "FidgetPlugin.h"


FidgetPlugin::FidgetPlugin() : IBehaviourPlugin(GameDebugParams(5,true , true, true))
{
}


FidgetPlugin::~FidgetPlugin()
{
	if (m_pBehaviorTree) delete(m_pBehaviorTree);
	if (m_pBlackboard) delete(m_pBlackboard);
}

void FidgetPlugin::Start()
{
	m_Agent = AGENT_GetInfo();
	m_pBlackboard = new Blackboard();
	m_pBlackboard->AddData("IsInHouse", m_bInHouse);
	m_pBlackboard->AddData("SpinTimerLimit", m_SpinTimerLimit);
	m_pBlackboard->AddData("TimeSinceLastSpin", m_TimeSinceLastSpin);
	m_pBlackboard->AddData("FOVEntities", m_FOVEntities);
	m_pBlackboard->AddData("FOVHouses", m_FOVHouses);
	m_pBlackboard->AddData("EnemiesSeen", m_EnemiesSeen);
	m_pBlackboard->AddData("Inventory", m_Inventory);
	m_pBlackboard->AddData("GunNeedMul", m_GunNeedMultiplier);
	m_pBlackboard->AddData("GunNeed", m_GunNeed);
	m_pBlackboard->AddData("MedNeedMul", m_MedNeedMultiplier);
	m_pBlackboard->AddData("MedNeed", m_MedNeed);
	m_pBlackboard->AddData("FoodNeedMul", m_FoodNeedMultiplier);
	m_pBlackboard->AddData("FoodNeed", m_FoodNeed);
	m_pBlackboard->AddData("GunNeedFinal", m_GunNeedFinal);
	m_pBlackboard->AddData("MedNeedFinal", m_MedNeedFinal);
	m_pBlackboard->AddData("FoodNeedFinal", m_FoodNeedFinal);
	m_pBlackboard->AddData("Agent", m_Agent);
	m_pBlackboard->AddData("AvoidDirection", m_AvoidDirection);
	m_pBlackboard->AddData("LowDanger", m_LowDanger);
	m_pBlackboard->AddData("HighDanger", m_HighDanger);
	m_pBlackboard->AddData("CritDanger", m_CriticalDanger);
	m_pBlackboard->AddData("ShootTarget", m_ShootTarget);
	m_pBlackboard->AddData("Panic", m_bPanic);
	m_pBlackboard->AddData("TakeMedsLevel", m_TakeMedsLevel);
	m_pBlackboard->AddData("TakeFoodLevel", m_TakeFoodLevel);
	m_pBlackboard->AddData("Target", m_Target);
	m_pBlackboard->AddData("GunSlot", m_GunSlot);
	m_pBlackboard->AddData("ShootClosest", m_bShoot);
	m_pBlackboard->AddData("ClosestLootPos", m_ClosestLoot);
	m_pBlackboard->AddData("GrabBool", m_bGrab);
	m_pBlackboard->AddData("WorldInfo", m_World);

	//Behaviour tree
	m_pBehaviorTree = new BehaviorTree(m_pBlackboard,
		new BehaviorSequence
		({
			//***SET RAND TARGET***
			new BehaviorSequence
			({
				new BehaviorConditional(HasTarget),
				new BehaviorAction(SetRandTarget)
			}),
			//***SPIN***
			new BehaviorSequence
			({
				new BehaviorConditional(SpinTimerReached),
				new BehaviorAction(Turn)
			}),
			new BehaviorParallelSequence
			({
				//***DROP ITEMS***
				new BehaviorAction(DropItems),
				//***ENTITIES***
				new BehaviorSequence
				({
					new BehaviorConditional(SeesEntities),
					new BehaviorParallelSequence
					({
						new BehaviorSequence
						({
							new BehaviorConditional(SeesLoot),
							new BehaviorConditional(NeedLoot),
							//Set LootTarget
							new BehaviorAction(Loot)
						}),
						new BehaviorSequence
						({
							new BehaviorConditional(SeesEnemies),
							new BehaviorConditional(DangerLevel),
							new BehaviorConditional(ReactToDanger),
							new BehaviorSelector
							({
								new BehaviorSequence
								({
									new BehaviorConditional(CanShoot),
									new BehaviorAction(Shoot)
								}),
								new BehaviorConditional(Panic)
							})
						})
					})
				}),
				//***HOUSES***
				new BehaviorSequence
				({
					//SeesHouses
					new BehaviorConditional(NeedLoot),
					new BehaviorSelector
					({
						new BehaviorSequence
						({
							//new BehaviorConditional(HasHouseTarget),
							//SetGoToHouse
						}),
						new BehaviorSequence
						({
							new BehaviorConditional(IsInHouse),
							//new BehaviorConditional(IsHouseTargetReached),
							new BehaviorSelector
							({
								new BehaviorSequence
								({
									new BehaviorConditional(SeesLoot),
									new BehaviorConditional(NeedLoot),
									//Loot
								}),
								//Leave
								//Set new target
							})
						})
					})
				}),
				//***CHECK VITALS***
				new BehaviorSelector
				({
					new BehaviorSequence
					({
						new BehaviorConditional(CheckHealth),
						new BehaviorAction(UseMeds)
					}),
					new BehaviorSequence
					({
						new BehaviorConditional(CheckEnergy),
						new BehaviorAction(UseFood)
					})
				})
			})
		})
	);
}

PluginOutput FidgetPlugin::Update(float dt)
{
	PluginOutput output = {};

	//Pull data
	m_World = WORLD_GetInfo();
	m_Agent = AGENT_GetInfo();
	m_FOVEntities = FOV_GetEntities();
	m_FOVHouses = FOV_GetHouses();
	m_Timer += dt;
	m_pBlackboard->GetData("Target", m_Target);

	//Go To Target
	auto targetVelocity = m_Target - m_Agent.Position;
	auto distance = targetVelocity.Normalize();
	targetVelocity *= m_Agent.MaxLinearSpeed;
	
	output.LinearVelocity = targetVelocity - m_Agent.LinearVelocity;

	//SetRunMode
	output.RunMode = m_Agent.RunMode;

	//Filter data
	for (auto ent : m_FOVEntities)
	{
		if (ent.Type != ENEMY) continue;
		EnemyInfo i;
		ENEMY_GetInfo(ent, i);
		for (auto en : m_EnemiesSeen)
		{
			if(i.EnemyHash == en.info.EnemyHash)
			{
				break;
			}
		}
		EnemySeen t;
		t.lastPos = ent.Position;
		t.info = i;
		t.timeSpotted = m_Timer;
		m_EnemiesSeen.push_back(t);
	}

	//Distribute data
	m_bInHouse = m_Agent.IsInHouse;

	m_TimeSinceLastSpin += dt;

	//Update Blackboard data
	m_pBlackboard = m_pBehaviorTree->GetBlackboard();
	if (!m_pBlackboard) return output;
	m_pBlackboard->ChangeData("IsInHouse", m_bInHouse);
	m_pBlackboard->ChangeData("TimeSinceLastSpin", m_TimeSinceLastSpin);
	m_pBlackboard->ChangeData("FOVEntities", m_FOVEntities);
	m_pBlackboard->ChangeData("FOVHouses", m_FOVHouses);
	m_pBlackboard->ChangeData("EnemiesSeen", m_EnemiesSeen);
	m_pBlackboard->ChangeData("Agent", m_Agent);
	m_pBlackboard->ChangeData("Panic", m_bPanic);
	m_pBlackboard->ChangeData("Target", m_Target);
	m_pBlackboard->ChangeData("GunSlot", m_GunSlot);
	m_pBlackboard->ChangeData("ShootClosest", m_bShoot);
	m_pBlackboard->ChangeData("ClosestLootPos", m_ClosestLoot);
	m_pBlackboard->ChangeData("GrabBool", m_bGrab);
	m_pBlackboard->ChangeData("WorldInfo", m_World);

	//Update BT
	if (m_pBehaviorTree) m_pBehaviorTree->Update();

	return output;
}

void FidgetPlugin::ExtendUI_ImGui()
{
}

void FidgetPlugin::End()
{
}

void FidgetPlugin::ProcessEvents(const SDL_Event& e)
{
}
