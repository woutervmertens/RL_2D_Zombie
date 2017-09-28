#pragma once
#include "Data.h"
#include <vector>
#include <map>
#include <unordered_map>

class QLearningTable
{
public:
	QLearningTable();
	~QLearningTable();

	Action ChooseAction(State s);
	void Learn(State s, Action a, int r, State s_, bool dynaQ);
	void Learn(State s, Action a, int r, State s_, Action a_, bool lambda);
	bool Parse(const std::string &file);
	bool Save(const std::string &file);
private:
	void CheckStateExists(State s);
	void splitString(std::vector<std::string>& v_str, const std::string& str, const char ch);

	float m_LearningRate;
	float m_Gamma;
	float m_Epsilon;
	float m_Lambda;
	//std::map<State, std::vector<double>> m_QTable;
	std::unordered_map<State, std::vector<double>> m_QTable;
	std::unordered_map<State, std::vector<double>> m_EligibilityTrace;
	//std::unordered_map<std::tuple<State, int, State>, double> m_ProbabilityCount;
};

