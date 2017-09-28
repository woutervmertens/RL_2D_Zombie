#include "stdafx.h"
#include "TestBoxPlugin.h"

TestBoxPlugin::TestBoxPlugin():IBehaviourPlugin(GameDebugParams(20,true,false, false,false,0.5f))
{
}

TestBoxPlugin::~TestBoxPlugin()
{
}

void TestBoxPlugin::Start()
{
}

PluginOutput TestBoxPlugin::Update(float dt)
{
	//Output
	PluginOutput output = {};

	auto worldInfo = WORLD_GetInfo(); //Contains the location of the center of the world and the dimensions
	auto agentInfo = AGENT_GetInfo(); //Contains all Agent Parameters, retrieved by copy!

	DEBUG_DrawCircle(agentInfo.Position, agentInfo.GrabRange, { 0,0,1 }); //DEBUG_... > Debug helpers (disabled during release build)

	//STEERING
	DEBUG_DrawSolidCircle(m_Target, 0.3f, { 0.f,0.f }, { 1.f,0.f,0.f });

	//GOTO Target (Seek & Arrive)
	auto targetVelocity = m_Target - agentInfo.Position;
	auto distance = targetVelocity.Normalize() - m_TargetRadius;

	if (distance < m_SlowRadius) //Inside SlowRadius
	{
		targetVelocity *= agentInfo.MaxLinearSpeed * (distance / (m_SlowRadius + m_TargetRadius));
	}
	else
	{
		targetVelocity *= agentInfo.MaxLinearSpeed;
	}

	output.LinearVelocity = targetVelocity - agentInfo.LinearVelocity;
	output.RunMode = m_RunAction; //Activates/Deactivates RunMode
	//output.AutoOrientate = false; //Ignores AngularVelocity if true
	//output.AngularVelocity = b2_pidiv2;

	//INVENTORY Example
	if(m_GrabAction)
	{
		auto entitiesInFOV = FOV_GetEntities(); //Retrieve Items in FOV
		//Walk to item and grab item for inspection

		ItemInfo item;
		//Grab Item in range (if there is an item in range)
		//This demo has the GameDebugParam AutoGrabClosestItem to true, ITEM_Grab ignores the given EntityInfo 
		//(normally you should pass a valid EntityInfo, retrieved through FOV_GetEntities)
		if(ITEM_Grab(EntityInfo(), item)) 
		{
			//Check out item
			//Example (using Item MetaData)
			//Extract metadata from items (these are the possibilities, for now... ;) )
			switch (item.Type)
			{
			case PISTOL: //ammo[INT],dps[FLOAT],range[FLOAT]
				int ammo;
				ITEM_GetMetadata(item, "ammo", ammo); //returns bool, use checks (use the correct type)
				float dps;
				ITEM_GetMetadata(item, "dps", dps);
				float range;
				ITEM_GetMetadata(item, "range", range);
				break;
			case HEALTH: //health[INT]
				int health;
				ITEM_GetMetadata(item, "health", health);
				break;
			case FOOD: break;
			case GARBAGE: break;
			default: break;
			}

			//Must be stored in Inventory during the same frame! (otherwise EntityInfo::EntityHash becomes invalid)
			//So forget about caching the hashes ;) - This would be cheating :P
			INVENTORY_AddItem(m_SelectedInventorySlot, item);
		}
	}

	if(m_UseItemAction) //Using an item
	{
		INVENTORY_UseItem(m_SelectedInventorySlot); //returns bool
	}

	if (m_RemoveItemAction) //Removing an item ('empty' items stay in inventory, you're responsible of removing them)
	{
		INVENTORY_RemoveItem(m_SelectedInventorySlot); //again returns bool (do checks)
	}

	//Reset Action Flags
	m_GrabAction = false;
	m_UseItemAction = false;
	m_RemoveItemAction = false;

	return output;
}

//Extend the UI [ImGui call only!]
void TestBoxPlugin::ExtendUI_ImGui()
{
	ImGui::Text("Selected Slot: %i",m_SelectedInventorySlot);
}

void TestBoxPlugin::End()
{
	//NOTHING
}

//[Optional]> For Debugging
void TestBoxPlugin::ProcessEvents(const SDL_Event& e)
{
	switch(e.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	{
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			int x, y;
			SDL_GetMouseState(&x, &y);
			auto pos = b2Vec2(static_cast<float>(x), static_cast<float>(y));
			m_Target = DEBUG_ConvertScreenPosToWorldPos(pos);
		}
	}
		break;
	case SDL_KEYDOWN:
	{
		m_GrabAction = (e.key.keysym.sym == SDLK_SPACE);
		m_UseItemAction = (e.key.keysym.sym == SDLK_u);
		m_RemoveItemAction = (e.key.keysym.sym == SDLK_r);
		if (e.key.keysym.sym == SDLK_LCTRL)
		{
			m_RunAction = !m_RunAction;
		}

		//Slot Selection
		auto currSlot = m_SelectedInventorySlot;
		m_SelectedInventorySlot = (e.key.keysym.sym == SDLK_0) ? 0 : m_SelectedInventorySlot;
		m_SelectedInventorySlot = (e.key.keysym.sym == SDLK_1) ? 1 : m_SelectedInventorySlot;
		m_SelectedInventorySlot = (e.key.keysym.sym == SDLK_2) ? 2 : m_SelectedInventorySlot;
		m_SelectedInventorySlot = (e.key.keysym.sym == SDLK_3) ? 3 : m_SelectedInventorySlot;
		m_SelectedInventorySlot = (e.key.keysym.sym == SDLK_4) ? 4 : m_SelectedInventorySlot;
	}
		break;
	}
}