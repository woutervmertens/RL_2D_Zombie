#pragma once
#include "IBehaviourPlugin.h"

class TestBoxPlugin : public IBehaviourPlugin
{
public:
	TestBoxPlugin();
	~TestBoxPlugin();

	void Start() override;
	PluginOutput Update(float dt) override;
	void ExtendUI_ImGui() override;
	void End() override;
	void ProcessEvents(const SDL_Event& e) override;

protected:

	float m_SlowRadius = 10.f;
	float m_TargetRadius = 2.f;
	bool m_GrabAction = false;
	bool m_UseItemAction = false;
	bool m_RemoveItemAction = false;
	bool m_RunAction = false;
	int m_SelectedInventorySlot = 0;

	b2Vec2 m_Target;
};

