#pragma once

#include <Windows.h>

class BuildingClass;
class TechnoTypeClass;

// Experimental: Generals / C&C3-style production UI.
//
// The production tabs of the sidebar only show what the currently selected
// production building (Construction Yard, Barracks, War Factory, Naval Yard,
// Airfield) can build. With no production building selected, the tabs are empty.
//
// How it works (milestone 1 - UI only, production queues are still house-wide):
//  - Adding a cameo (SidebarClass::AddCameo) is refused for anything the
//    selected building cannot produce.
//  - Ares' cameo removal (redirected to SidebarExt::AresTabCameo_RemoveCameo)
//    silently removes such cameos, WITHOUT sending abandon-production events,
//    so production keeps running while its cameo is hidden.
//  - Every frame we check whether the selected producer changed; if so the
//    strips are rechecked, the producer re-adds its cameos and the tab switches.
//
// All of this is local UI state of the current player. It never touches the
// synchronised game state, so it cannot cause multiplayer desyncs.
class PerBuildingProduction
{
public:
	static bool IsEnabled();

	// The production building the player has selected, or nullptr.
	static BuildingClass* GetSelectedProducer();

	// Whether this building is the kind of factory that makes this type.
	static bool CanProducerBuild(BuildingClass* pProducer, TechnoTypeClass* pType);

	// Whether this type's cameo may be shown on the sidebar right now.
	static bool IsCameoVisible(TechnoTypeClass* pType);

	// Sidebar tab (0-3) that belongs to this producer.
	static int GetTabForProducer(BuildingClass* pProducer);

	// Called once per frame from the main loop.
	static void Update();

	// Forget the cached selection (new game / load game).
	static void Reset();

private:
	static void Refresh(BuildingClass* pProducer);
	static void RelinkFactories(BuildingClass* pProducer);
	static void ProcessHiddenProduction();

	static DWORD LastProducerID;
	static bool HasLastProducer;
};
