#pragma once
#include "IBehaviourPlugin.h"
#include "BT/Behaviors.h"
class FidgetPlugin :
	public IBehaviourPlugin
{
public:
	FidgetPlugin();
	~FidgetPlugin();

	void Start() override;
	PluginOutput Update(float dt) override;
	void ExtendUI_ImGui() override;
	void End() override;
	void ProcessEvents(const SDL_Event& e) override;

protected:
	bool m_bPanic = false;
	bool m_bShoot = false;
	bool m_bGrab = false;
	bool m_bInHouse = false;

	b2Vec2 m_Target = b2Vec2_zero;
	b2Vec2 m_AvoidDirection = b2Vec2_zero;
	b2Vec2 m_ShootTarget = b2Vec2_zero;
	b2Vec2 m_ClosestLoot = b2Vec2_zero;

	float m_SpinTimerLimit = 5.f;
	float m_TimeSinceLastSpin = 0.f;
	float m_GunNeedMultiplier = 1.1f;
	float m_GunNeed = 1.0f;
	float m_MedNeedMultiplier = 1.1f;
	float m_MedNeed = 2.0f;
	float m_FoodNeedMultiplier = 1.1f;
	float m_FoodNeed = 2.0f;
	float m_GunNeedFinal = 0.f;
	float m_MedNeedFinal = 0.f;
	float m_FoodNeedFinal = 0.f;
	float m_LowDanger = 15.f;
	float m_HighDanger = 8.f;
	float m_CriticalDanger = 4.f;
	float m_TakeMedsLevel = 5.f;
	float m_TakeFoodLevel = 10.f;
	float m_Timer = 0.f;

	int m_GunSlot = -1;

	AgentInfo m_Agent;
	WorldInfo m_World;

	std::vector<EntityInfo> m_FOVEntities = std::vector<EntityInfo>();
	std::vector<HouseInfo> m_FOVHouses = std::vector<HouseInfo>();
	std::vector<EntityInfo> m_LootSeen = std::vector<EntityInfo>();
	std::vector<EnemySeen> m_EnemiesSeen = std::vector<EnemySeen>();
	std::vector<InventoryItem> m_Inventory = std::vector<InventoryItem>();

	BehaviorTree* m_pBehaviorTree = nullptr;
	Blackboard* m_pBlackboard = nullptr;

};

