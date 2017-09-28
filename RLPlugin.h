#pragma once
#include "QLearningTable.h"



class RLPlugin : public IBehaviourPlugin
{
public:
	RLPlugin();
	~RLPlugin();

	void Start() override;
	PluginOutput Update(float dt) override;
	void ExtendUI_ImGui() override;
	void End() override;
	void ProcessEvents(const SDL_Event& e) override;

private:
	State Observe();
	PluginOutput RunAction(Action action);
	int CalculateReward(State newObservation);
	void Reset();
	void Results();

	State m_Observation;
	Action m_Action;
	Action m_nextAction;

	WorldInfo m_World;
	AgentInfo m_Agent;
	std::vector<EntityInfo> m_FOVEntities;
	std::vector<HouseInfo> m_FOVHouses;
	std::vector < std::pair<HouseInfo, double>> m_HousesSeen; //info and time last looted
	std::vector<EnemySeen1> m_EnemiesSeen;
	std::vector<std::pair<ItemInfo, int>> m_GunsInventory; //info and inventory slot
	double m_Timer;

	QLearningTable RL;
	bool m_bDecide;

	int m_ExtraReward;

	//Roam
	b2Vec2 m_RoamTarget;
	//Visit House
	b2Vec2 m_HouseTarget;
	bool m_bWasPreviouslyInHouse;
	int m_BestHouseIt;
	//Defend
	bool m_bWasShooting;
	double m_ShootingTimer;
	float m_DeltaTime;
	//Loot
	b2Vec2 m_LootTarget;
	bool m_bWasLooting;
	std::vector<std::pair<ItemInfo, int>> m_HealthInventory;
	std::vector<std::pair<ItemInfo, int>> m_FoodInventory;
	std::vector<int> m_OpenInventory;
	EntityInfo m_ClosestLoot;
	std::vector<b2Vec2> m_GarbagePositions;
	//Fix Vitals
};

