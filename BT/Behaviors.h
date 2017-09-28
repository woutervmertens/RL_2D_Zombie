#pragma once
#include "stdafx.h"
#include "Blackboard.h"
#include "BehaviorTree.h"
#include "IBehaviourPlugin.h"

struct InventoryItem
{
	bool isEmpty = true;
	bool Remove = false;
	bool Use = false;
	int Points = 0;
	ItemInfo item;
	eItemType type;
};

struct EnemySeen
{
	b2Vec2 lastPos = b2Vec2_zero;
	float timeSpotted = 0.f;
	EnemyInfo info;
	float lastDist = 0.f;
};

//CONDITIONALS
//************
inline bool IsInHouse(Blackboard* pBlackboard)
{
#pragma region DATA
	auto inHouse = false;
	auto result = pBlackboard->GetData("IsInHouse", inHouse);
	if (!result) return false;
#pragma endregion
	return inHouse;
}

inline bool HasTarget(Blackboard* pBlackboard)
{
#pragma region DATA
	auto target = b2Vec2_zero;
	auto result = pBlackboard->GetData("Target", target);
	if (!result) return false;
#pragma endregion
	return target == b2Vec2_zero;
}

inline bool SpinTimerReached(Blackboard* pBlackboard)
{
#pragma region DATA
	float timerLimit = 0.f;
	float timeSinceLastSpin = 0.f;
	auto result = pBlackboard->GetData("SpinTimerLimit", timerLimit) 
		|| pBlackboard->GetData("TimeSinceLastSpin",timeSinceLastSpin);
	if (!result) return false;
#pragma endregion
	return timerLimit < timeSinceLastSpin;
}

inline bool SeesEntities(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<EntityInfo> entities;
	auto result = pBlackboard->GetData("FOVEntities", entities);
	if (!result) return false;
#pragma endregion
	return !entities.empty();
}

inline bool SeesEnemies(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<EnemySeen> enemiesSeen;
	auto result = pBlackboard->GetData("EnemiesSeen", enemiesSeen);
	if (!result) return false;
#pragma endregion
	if (!enemiesSeen.empty()) return true;
	return false;
}

inline bool SeesLoot(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<EntityInfo> entities;
	auto result = pBlackboard->GetData("FOVEntities", entities);
	if (!result) return false;
#pragma endregion
	int i = 0;
	for (std::vector<EntityInfo>::iterator it = entities.begin(); it != entities.end(); i++)
	{
		*it = entities.at(i);
		if (it->Type == ITEM)
		{
			return true;
		}
	}
	return false;
}

inline bool NeedLoot(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	auto gunNeedMul = 0.f;
	auto gunNeed = 0.f;
	auto medNeedMul = 0.f;
	auto medNeed = 0.f;
	auto foodNeedMul = 0.f;
	auto foodNeed = 0.f;
	auto gunNeedFinal = 0.f;
	auto medNeedFinal = 0.f;
	auto foodNeedFinal = 0.f;
	auto result = pBlackboard->GetData("Inventory", inventory)
		|| pBlackboard->GetData("GunNeedMul", gunNeedMul)
		|| pBlackboard->GetData("GunNeed", gunNeed)
		|| pBlackboard->GetData("MedNeedMul", medNeedMul)
		|| pBlackboard->GetData("MedNeed", medNeed)
		|| pBlackboard->GetData("FoodNeedMul", foodNeedMul)
		|| pBlackboard->GetData("FoodNeed", foodNeed)
		|| pBlackboard->GetData("GunNeedFinal", gunNeedFinal)
		|| pBlackboard->GetData("MedNeedFinal", medNeedFinal)
		|| pBlackboard->GetData("FoodNeedFinal", foodNeedFinal);
	if (!result) return false;
#pragma endregion
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if (it->isEmpty) continue;
		if(it->type == FOOD)
		{
			foodNeed--;
		}
		else if(it->type == HEALTH)
		{
			medNeed--;
		}
		else if(it->type == PISTOL)
		{
			gunNeed--;
		}
	}
	if(foodNeed + medNeed + gunNeed < 1)
		return false;

	gunNeedFinal = gunNeed * gunNeedMul;
	medNeedFinal = medNeed * medNeedMul;
	foodNeedFinal = foodNeed * foodNeedMul;
	pBlackboard->ChangeData("GunNeedFinal", gunNeedFinal);
	pBlackboard->ChangeData("MedNeedFinal", medNeedFinal);
	pBlackboard->ChangeData("FoodNeedFinal", foodNeedFinal);
	return true;
}

inline bool DangerLevel(Blackboard* pBlackboard)
{
#pragma region DATA
	auto avoidDirection = b2Vec2_zero;
	auto avoidLength = 0.f;
	std::vector<EnemySeen> enemiesSeen;
	AgentInfo agent;
	auto result = pBlackboard->GetData("EnemiesSeen", enemiesSeen)
		|| pBlackboard->GetData("Agent", agent);
	if (!result) return false;
#pragma endregion
	avoidDirection = agent.Position;
	int i = 0;
	if (enemiesSeen.size() == 0) return false;
	for (std::vector<EnemySeen>::iterator it = enemiesSeen.begin(); it != enemiesSeen.end(); i++)
	{
		*it = enemiesSeen.at(i);
		//vector from enemy to agent
		b2Vec2 vec = (it->lastPos - agent.Position);
		avoidLength = vec.Length();
		it->lastDist = avoidLength;
		vec = vec /avoidLength;
		avoidDirection += vec;
	}
	pBlackboard->ChangeData("EnemiesSeen", enemiesSeen);
	pBlackboard->ChangeData("AvoidDirection", avoidDirection);
	return true;
}

inline bool ReactToDanger(Blackboard* pBlackboard)
{
#pragma region DATA
	auto lowDanger = 0.f;
	auto highDanger = 0.f;
	auto criticalDanger = 0.f;
	AgentInfo agent;
	std::vector<EnemySeen> enemiesSeen;
	auto result = pBlackboard->GetData("LowDanger", lowDanger)
		|| pBlackboard->GetData("HighDanger", highDanger)
		|| pBlackboard->GetData("CritDanger", criticalDanger)
		|| pBlackboard->GetData("Agent", agent)
		|| pBlackboard->GetData("EnemiesSeen", enemiesSeen);
	if (!result) return false;
#pragma endregion
	int i = 0;
	for (std::vector<EnemySeen>::iterator it = enemiesSeen.begin(); it != enemiesSeen.end(); i++)
	{
		*it = enemiesSeen.at(i);
		if (it->lastDist < lowDanger) continue;
		if(it->lastDist > criticalDanger)
		{
			pBlackboard->ChangeData("ShootTarget", it->lastPos);
			return true;
		}
		agent.RunMode = true;
	}
	pBlackboard->ChangeData("Agent", agent);
	return false;
}

inline bool CanShoot(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	auto result = pBlackboard->GetData("Inventory", inventory);
	if (!result) return false;
#pragma endregion
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if (it->isEmpty) continue;
		if(it->type == PISTOL && it->Points > 0)
		{
			return true;
		}
	}
	return false;
}

inline bool Panic(Blackboard* pBlackboard)
{
#pragma region DATA
	auto panic = true;
#pragma endregion
	pBlackboard->ChangeData("Panic", panic);
	return true;
}

inline bool CheckHealth(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	AgentInfo agent;
	auto takeMedsLevel = 0.f;
	auto medNeed = 0.f;
	auto result = pBlackboard->GetData("Inventory", inventory) 
		|| pBlackboard->GetData("Agent",agent) 
		|| pBlackboard->GetData("TakeMedsLevel",takeMedsLevel) 
		|| pBlackboard->GetData("MedNeed", medNeed);
	if (!result) return false;
#pragma endregion
	if (agent.Health >= takeMedsLevel) return false;
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if (it->isEmpty) continue;
		if (it->type == HEALTH)
		{
			return true;
		}
	}
	medNeed++;
	pBlackboard->ChangeData("MedNeed", medNeed);
	return false;
}

inline bool CheckEnergy(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	AgentInfo agent;
	auto takeFoodLevel = 0.f;
	auto foodNeed = 0.f;
	auto result = pBlackboard->GetData("Inventory", inventory)
		|| pBlackboard->GetData("Agent", agent)
		|| pBlackboard->GetData("TakeFoodLevel", takeFoodLevel)
		|| pBlackboard->GetData("FoodNeed", foodNeed);
	if (!result) return false;
#pragma endregion
	if (agent.Energy >= takeFoodLevel) return false;
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if (it->isEmpty) continue;
		if (it->type == FOOD)
		{
			return true;
		}
	}
	foodNeed++;
	pBlackboard->ChangeData("FoodNeed", foodNeed);
	return false;
}

//ACTIONS
//*******
inline BehaviorState SetRandTarget(Blackboard* pBlackboard)
{
#pragma region DATA
	auto target = b2Vec2_zero;
	WorldInfo worldInfo;
	auto t = pBlackboard->GetData("Target", target);
	auto w = pBlackboard->GetData("WorldInfo", worldInfo);
	auto result = t && w;
	if (!result) return Failure;
#pragma endregion
	target.Set(randomFloat(worldInfo.Dimensions.x), randomFloat(worldInfo.Dimensions.y));
	pBlackboard->ChangeData("Target", target);
	return Success;
}

inline BehaviorState Shoot(Blackboard* pBlackboard)
{
#pragma region DATA
	auto gunSlot = -1;
	auto shoot = false;
	std::vector<InventoryItem> inventory;
	auto result = pBlackboard->GetData("Inventory", inventory)
		|| pBlackboard->GetData("GunSlot", gunSlot);
	if (!result) return Failure;
#pragma endregion
	if(gunSlot == -1)
	{
		int i = 0;
		for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
		{
			*it = inventory.at(i);
			if (it->isEmpty) continue;
			if(it->type == PISTOL && it->Points > 0)
			{
				gunSlot = i;
			}
		}
	}
	shoot = true;
	pBlackboard->ChangeData("ShootClosest", shoot);
	pBlackboard->ChangeData("GunSlot", gunSlot);
	return Success;
}

inline BehaviorState UseMeds(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	auto result = pBlackboard->GetData("Inventory", inventory);
	if (!result) return Failure;
#pragma endregion
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if (it->type == HEALTH)
		{
			it->Use = true;
			pBlackboard->ChangeData("Inventory", inventory);
			return Success;
		}
	}
	return Success;
}

inline BehaviorState UseFood(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	auto result = pBlackboard->GetData("Inventory", inventory);
	if (!result) return Failure;
#pragma endregion
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if (it->type == FOOD)
		{
			it->Use = true;
			pBlackboard->ChangeData("Inventory", inventory);
			return Success;
		}
	}
	return Success;
}

inline BehaviorState DropItems(Blackboard* pBlackboard)
{
#pragma region DATA
	std::vector<InventoryItem> inventory;
	auto result = pBlackboard->GetData("Inventory", inventory);
	if (!result) return Failure;
#pragma endregion
	int i = 0;
	for (std::vector<InventoryItem>::iterator it = inventory.begin(); it != inventory.end(); i++)
	{
		*it = inventory.at(i);
		if(it->Points = 0)
		{
			it->Remove = true;
		}
	}
	pBlackboard->ChangeData("Inventory", inventory);
	return Success;
}

inline BehaviorState Loot(Blackboard* pBlackboard){
#pragma region DATA
auto goToPos = b2Vec2_zero;
std::vector<EntityInfo> entities;
AgentInfo agent;
auto grab = false;
auto result = pBlackboard->GetData("FOVEntities", entities) 
	|| pBlackboard->GetData("ClosestLootPos", goToPos) 
	|| pBlackboard->GetData("Agent", agent) 
	|| pBlackboard->GetData("GrabBool", grab);
if (!result) return Failure;
#pragma endregion
if(goToPos != b2Vec2_zero)
{
	float dist_x = (goToPos.x - agent.Position.x) * (goToPos.x - agent.Position.x);
	float dist_y = (goToPos.y - agent.Position.y) * (goToPos.y - agent.Position.y);

	float dist = sqrtf(dist_x + dist_y);

	if(dist < agent.GrabRange)
	{
		grab = true;
		pBlackboard->ChangeData("GrabBool", grab);
		return Success;
	}
	return Running;
}

float closestRange = b2_maxFloat;

int i = 0;
for (std::vector<EntityInfo>::iterator it = entities.begin(); it != entities.end(); i++)
{
	*it = entities.at(i);
	float dist_x = (it->Position.x - agent.Position.x) * (it->Position.x - agent.Position.x);
	float dist_y = (it->Position.y - agent.Position.y) * (it->Position.y - agent.Position.y);

	float dist = sqrtf(dist_x + dist_y);

	if (dist < closestRange)
	{
		goToPos = it->Position;
		closestRange = dist;
	}
}
pBlackboard->ChangeData("ClosestLootPos", goToPos);
return Running;
}

inline BehaviorState Turn(Blackboard* pBlackboard)
{
#pragma region DATA
	AgentInfo agent;
	PluginOutput output;
	auto result = pBlackboard->GetData("Agent", agent) && pBlackboard->GetData("Output", output);
	if (!result) return Failure;
#pragma endregion
	return Success;
}
