#include "stdafx.h"
#pragma once
#include "QLearningTable.h"
#include <fstream>
#include <string>

static bool GreaterDouble(const double a, const double b)
{
	return (a > b);
}

QLearningTable::QLearningTable():
	m_LearningRate(0.01f),
	m_Gamma(0.9f),
	m_Epsilon(0.9f),
	m_Lambda(0.9)
{
	
}


QLearningTable::~QLearningTable()
{
}

Action QLearningTable::ChooseAction(State s)
{
	Action a;
	CheckStateExists(s);
	//Action selection
	if(randomFloat() < m_Epsilon)
	{
		//Choose best action
		std::vector<double> b = m_QTable[s];
		auto m = std::max_element(b.begin(), b.end());
		a = static_cast<Action>(std::distance(b.begin(), m));
	}
	else
	{
		//Choose random action
		a = static_cast<Action>(rand() % (FIX_VITALS - ROAM + 1));
	}

	return a;
}

void QLearningTable::Learn(State s, Action a, int r, State s_, bool dynaQ)
{
	CheckStateExists(s_);
	double qPredict = m_QTable[s][a];
	double qTarget;
	if(!s_.bIsDead)
	{
		auto maxQ = std::max_element(m_QTable[s_].begin(), m_QTable[s_].end(), GreaterDouble);
		qTarget = r + m_Gamma * (*maxQ);
	}
	else
	{
		qTarget = r;
	}
	m_QTable[s][a] += m_LearningRate * (qTarget - qPredict); //Update
	//if(dynaQ)
	//{
	//	std::tuple<State, int, State> sas = std::tuple<State, int, State>(s, a, s_);
	//	std::unordered_map<std::tuple<State, int, State>, double>::iterator it = m_ProbabilityCount.find(sas);
	//	if (it == m_ProbabilityCount.end())
	//	{
	//		m_ProbabilityCount[sas] = 1;
	//	}
	//	else
	//	{
	//		m_ProbabilityCount[sas]++;
	//	}
	//	//Some R'[s,a]
	//	for(int i = 0; i < 100; i++)
	//	{
	//		//random s
	//		auto iter = m_QTable.begin();
	//		std::advance(iter, rand() % m_QTable.size());
	//		State sR = iter->first;
	//		//random a
	//		int aR = rand() % 5;
	//		//infer s_ from T[]
	//		//State sN = ?What T[]?
	//		//double reward = R[s,a]something
	//		//update Q
	//	}
	//}
}

void QLearningTable::Learn(State s, Action a, int r, State s_, Action a_, bool lambda)
{
	CheckStateExists(s_);
	double qPredict = m_QTable[s][a];
	double qTarget;
	if (!s_.bIsDead)
	{
		qTarget = r + m_Gamma * m_QTable[s_][a_];
	}
	else
	{
		qTarget = r;
	}
	double error = qTarget - qPredict;
	if (lambda) {
		int i = 0;
		for (auto element : m_QTable[s]) {
			if (element == a)
			{
				m_EligibilityTrace[s][i] = 1;
			}
			else
			{
				m_EligibilityTrace[s][i] *= 0;
			}
			i++;
		}
		for (auto key : m_QTable)//Update Q values
		{
			int j = 0;
			for (auto value : key.second)
			{
				m_QTable[key.first][j] += m_LearningRate * error * m_EligibilityTrace[key.first][j];
				j++;
			}
		}
		for (auto key : m_EligibilityTrace)
		{
			int k = 0;
			for (auto value : key.second)
			{
				m_EligibilityTrace[key.first][k] *= m_Gamma * m_Lambda;
				k++;
			}
		}
	}
	else
	{
		m_QTable[s][a] += m_LearningRate * error;
	}
}

bool QLearningTable::Parse(const std::string &file)
{
	std::ifstream ifile;
	ifile.open(file.c_str());
	if (!ifile) return false;
	std::string line;
	/*std::getline(ifile, line);*/
	/*Iteration = std::stoi(line);*/
	std::vector<std::string> v_key;
	std::vector<std::string> v_val;
	std::vector<std::string> v_str;
	m_QTable.clear();
	m_EligibilityTrace.clear();
	while (!ifile.eof())
	{
		if(std::getline(ifile, line))
		{
			splitString(v_str, line, '|');
			splitString(v_key, v_str[0], ' ');
			splitString(v_val, v_str[1], ' ');
			v_val.pop_back();
			State s;
			s.bIsDead = (v_key[0] == "true"? true : false);
			s.Danger = static_cast<DangerBar>(std::stoi(v_key[1]));
			s.Guns = std::stoi(v_key[2]);
			s.Health = static_cast<Healthbar>(std::stoi(v_key[3]));
			s.Houses = std::stoi(v_key[4]);
			s.Hunger = static_cast<Hungerbar>(std::stoi(v_key[5]));

			std::vector<double> doubleVec(v_val.size());
			std::transform(v_val.begin(), v_val.end(), doubleVec.begin(), [](const std::string& val) {return std::stod(val); });
			m_QTable[s] = doubleVec;
			v_str.clear();
			v_key.clear();
			v_val.clear();
		}
	}
	m_EligibilityTrace.insert(m_QTable.begin(),m_QTable.end());
	for (auto key : m_EligibilityTrace)
	{
		int i = 0;
		for (auto value : key.second)
		{
			m_EligibilityTrace[key.first][i] = 0;
			i++;
		}
	}
	return true;
}

bool QLearningTable::Save(const std::string &file)
{
	std::unordered_map<State, std::vector<double>>::const_iterator it;
	std::ofstream ofile;
	ofile.open(file.c_str());
	if(!ofile)
	{
		return false;
	}
	/*ofile << Iteration << "\n";*/
	for(it = m_QTable.begin(); it != m_QTable.end(); ++it)
	{
		ofile << (it->first.bIsDead? "true" : "false") << " " << it->first.Danger << " " << it->first.Guns << " " << it->first.Health << " " << it->first.Houses << " " << it->first.Hunger << "|";
		for (double action : it->second)
		{
			ofile << action << " ";
		}
		ofile << "\n";
	}
	return true;
}

void QLearningTable::CheckStateExists(State s)
{
	std::unordered_map<State, std::vector<double>>::iterator it = m_QTable.find(s);
	if(it == m_QTable.end())
	{
		std::vector<double> vec;
		for (int i = 0; i <= FIX_VITALS - ROAM; i++) vec.push_back(0);
		m_QTable.insert(std::pair<State, std::vector<double>>(s, vec));
		m_EligibilityTrace.insert(std::pair<State, std::vector<double>>(s, vec));
	}
}

void QLearningTable::splitString(std::vector<std::string> &v_str, const std::string &str, const char ch)
{
	std::string sub;
	std::string::size_type pos = 0;
	std::string::size_type old_pos = 0;
	bool flag = true;

	while(flag)
	{
		pos = str.find_first_of(ch, pos);
		if(pos == std::string::npos)
		{
			flag = false;
			pos = str.size();
		}
		sub = str.substr(old_pos, pos - old_pos);
		v_str.push_back(sub);
		old_pos = ++pos;
	}
}
