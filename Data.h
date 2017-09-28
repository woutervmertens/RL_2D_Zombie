#pragma once
#include "IBehaviourPlugin.h"

enum DangerBar
{
	Low = 20,
	Mediocre,
	High,
	Extreme
};

enum Hungerbar
{
	Full = 40,
	Hungry,
	Starving
};

enum Healthbar
{
	Good = 30,
	Medium,
	Bad
};


static int Iteration = -1;

#ifndef ENEMYSEEN
#define ENEMYSEEN
struct EnemySeen1
{
	b2Vec2 lastPos = b2Vec2_zero;
	float timeSpotted = 0.f;
	EnemyInfo info;
};
#endif

struct State
{
	bool bIsDead;
	Healthbar Health;
	Hungerbar Hunger;
	DangerBar Danger;
	int Houses;
	int Guns;

	bool operator<(const State& state) const
	{
		double sum = Health + Hunger + Houses + Danger + Guns;
		double otherSum = state.Health + state.Hunger + state.Houses + state.Danger + state.Guns;
		if (sum == otherSum) return bIsDead == state.bIsDead;
		return sum < otherSum;
	}

	bool operator==(const State &other) const
	{
		return (bIsDead == other.bIsDead
			&& Health == other.Health
			&& Hunger == other.Hunger
			&& Danger == other.Danger
			&& Houses == other.Houses
			&& Guns == other.Guns);
	}
};

//struct Sas
//{
//	Sas(State s, int a, State s_):
//	tuple(s,a,s_){}
//	std::tuple<State, int, State> tuple;
//};

template<>
struct std::hash<State>
{
	std::size_t operator()(const State& s) const
	{
		using std::size_t;
		using std::hash;
		size_t res = 17;
		res = res * 31 + hash<bool>()(s.bIsDead);
		res = res * 31 + hash<int>()(static_cast<int>(s.Health));
		res = res * 31 + hash<int>()(static_cast<int>(s.Hunger));
		res = res * 31 + hash<int>()(static_cast<int>(s.Danger));
		res = res * 31 + hash<int>()(s.Houses);
		res = res * 31 + hash<int>()(s.Guns);
		return res;
		/*return ((hash<bool>()(s.bIsDead)
			^ (hash<int>()(static_cast<int>(s.Health))) >> 1)
			^ (hash<int>()(static_cast<int>(s.Hunger)) << 1)
			^ ((hash<int>()(s.Danger))
				^ (hash<int>()(s.Houses)) >> 1)
			^ (hash<int>()(s.Guns)) << 1);*/
	}
};


//template <>
//struct std::hash<std::tuple<State,int,State>>
//{
//	std::size_t operator()(const std::tuple<State, int, State>& t) const
//	{
//		using std::size_t;
//		using std::hash;
//		size_t res = 17;
//		res = res * 31 + hash<State>()(std::get<0>(t));
//		res = res * 31 + hash<int>()(static_cast<int>(std::get<1>(t)));
//		res = res * 31 + hash<State>()(std::get<2>(t));
//		return res;
//	}
//};

enum Action
{
	ROAM = 0,
	VISIT_HOUSE,
	DEFEND,
	LOOT,
	FIX_VITALS
};