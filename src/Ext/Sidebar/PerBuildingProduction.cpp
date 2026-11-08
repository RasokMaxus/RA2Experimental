#include "PerBuildingProduction.h"

#include <BuildingClass.h>
#include <EventClass.h>
#include <FactoryClass.h>
#include <HouseClass.h>
#include <MouseClass.h>
#include <ObjectClass.h>
#include <SidebarClass.h>
#include <TechnoTypeClass.h>
#include <Unsorted.h>
#include <VoxClass.h>

#include <Ext/Rules/Body.h>

DWORD PerBuildingProduction::LastProducerID = 0;
bool PerBuildingProduction::HasLastProducer = false;

bool PerBuildingProduction::IsEnabled()
{
	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt || !pRulesExt->PerBuildingProduction)
		return false;

	// Observers have no production of their own; leave their sidebar alone.
	return HouseClass::CurrentPlayer && !HouseClass::IsCurrentPlayerObserver();
}

BuildingClass* PerBuildingProduction::GetSelectedProducer()
{
	const auto pPlayer = HouseClass::CurrentPlayer;

	if (!pPlayer)
		return nullptr;

	for (const auto pObject : ObjectClass::CurrentObjects)
	{
		const auto pBuilding = abstract_cast<BuildingClass*>(pObject);

		if (!pBuilding || pBuilding->Owner != pPlayer)
			continue;

		if (pBuilding->Type->Factory == AbstractType::None)
			continue;

		if (!pBuilding->IsAlive || pBuilding->InLimbo || pBuilding->Health <= 0)
			continue;

		// Still playing its build-up animation, or being sold.
		const auto mission = pBuilding->CurrentMission;

		if (mission == Mission::Construction || mission == Mission::Selling)
			continue;

		return pBuilding;
	}

	return nullptr;
}

bool PerBuildingProduction::CanProducerBuild(BuildingClass* pProducer, TechnoTypeClass* pType)
{
	if (!pProducer || !pType)
		return false;

	const auto pProducerType = pProducer->Type;

	// BuildingType::Factory says which kind of object this building makes:
	// BuildingType (Construction Yard), InfantryType, UnitType or AircraftType.
	if (pType->WhatAmI() != pProducerType->Factory)
		return false;

	// A Naval Yard and a War Factory both make UnitTypes; Naval= tells them apart.
	if (pType->WhatAmI() == AbstractType::UnitType && pType->Naval != pProducerType->Naval)
		return false;

	// TODO: respect Ares' BuiltAt= once we can read Ares' TechnoTypeExt data.
	return true;
}

bool PerBuildingProduction::IsCameoVisible(TechnoTypeClass* pType)
{
	return CanProducerBuild(GetSelectedProducer(), pType);
}

int PerBuildingProduction::GetTabForProducer(BuildingClass* pProducer)
{
	switch (pProducer->Type->Factory)
	{
	case AbstractType::BuildingType:
		// Tab 0 = structures, tab 1 = defenses/support. Always open structures:
		// support powers (e.g. Spy Plane) keep tab 1 populated while nothing is
		// selected, so "stay where the player is" would land on defenses.
		return 0;
	case AbstractType::InfantryType:
		return 2;
	case AbstractType::UnitType:
	case AbstractType::AircraftType:
		return 3;
	default:
		return -1;
	}
}

void PerBuildingProduction::Reset()
{
	LastProducerID = 0;
	HasLastProducer = false;
}

void PerBuildingProduction::Update()
{
	if (!IsEnabled())
		return;

	// Finished items whose cameo is hidden still need to leave their factory.
	ProcessHiddenProduction();

	const auto pProducer = GetSelectedProducer();

	// Compare by UniqueID rather than by pointer: a destroyed building's memory
	// can be reused by a new one, and we must never dereference a stale pointer.
	const bool hasProducer = pProducer != nullptr;
	const DWORD producerID = hasProducer ? pProducer->UniqueID : 0;

	if (hasProducer == HasLastProducer && producerID == LastProducerID)
		return;

	HasLastProducer = hasProducer;
	LastProducerID = producerID;

	Refresh(pProducer);
}

void PerBuildingProduction::Refresh(BuildingClass* pProducer)
{
	auto& sidebar = SidebarClass::Instance;

	// 1) Drop every cameo the new selection can't produce.
	//    0x6A7D20 = SidebarClass::RecheckCameos: runs StripClass::RecheckCameos on all
	//    four tabs, which asks (Ares ->) SidebarExt::AresTabCameo_RemoveCameo per cameo.
	reinterpret_cast<void(__thiscall*)(SidebarClass*)>(0x6A7D20)(&sidebar);

	if (pProducer)
	{
		// 2) Let the producer add its cameos. This is the same call the game makes
		//    for every building when the tech tree changes; AddCameo is filtered by us.
		//    AddCameo announces "New construction options" unless Unsorted::ScenarioInit
		//    is non-zero (the game raises it while setting up a map). These options
		//    aren't new - they were only hidden - so keep EVA quiet.
		++Unsorted::ScenarioInit;
		pProducer->UpdateConstructionOptions();
		--Unsorted::ScenarioInit;

		// 3) Re-link the fresh cameos to their factories. A cameo remembers the factory
		//    building it (BuildType::CurrentFactory), but that link is only set when
		//    production starts. Without this, a re-added cameo has no progress clock and
		//    clicking it tries to start a second production ("building in progress").
		RelinkFactories(pProducer);

		// 4) Show the matching tab.
		const int tab = GetTabForProducer(pProducer);

		if (tab >= 0 && tab < 4)
			TabClass::Instance.SetTab(tab);
	}

	// 5) Redraw.
	sidebar.SidebarNeedsRepaint();
}

void PerBuildingProduction::ProcessHiddenProduction()
{
	// In vanilla the SIDEBAR drives the last step of production: every frame
	// StripClass::Update (0x6A8DC6..0x6A8F49) looks at each cameo's factory, and when
	// that factory reports "progress changed" + "done", it
	//  - for infantry/vehicles/aircraft: sends a Place event, which makes the
	//    finished unit leave the factory, so the next queued item can start;
	//  - for buildings: plays "Construction complete" and remembers the building
	//    as ready to place.
	// A hidden cameo never gets that update, so we do the same here for factories
	// whose cameo isn't on the sidebar. Visible ones are left to the strip.
	const auto pPlayer = HouseClass::CurrentPlayer;

	for (const auto pFactory : FactoryClass::Array)
	{
		if (pFactory->Owner != pPlayer)
			continue;

		const auto pObject = pFactory->Object;

		if (!pObject || !pFactory->IsDone())
			continue;

		const auto pType = pObject->GetTechnoType();

		if (!pType || IsCameoVisible(pType))
			continue;

		// Consumes the flag, exactly like the strip does, so each completion is
		// handled once - by us or by the strip, never both.
		if (!pFactory->HasProgressChanged())
			continue;

		switch (pObject->WhatAmI())
		{
		case AbstractType::Unit:
		case AbstractType::Aircraft:
		case AbstractType::Infantry:
		{
			// Same arguments as 0x6A8EB8..0x6A8EE8: heap id -1, the object's naval flag,
			// and the (empty) cell stored at 0xB07DD8.
			const auto& cell = *reinterpret_cast<CellStruct*>(0xB07DD8);
			EventClass::OutList.Add(EventClass(pObject->GetOwningHouseIndex(), EventType::Place,
				pObject->WhatAmI(), -1, pType->Naval, cell));
			break;
		}
		case AbstractType::Building:
			// 0x6A8E25: announce and register the building as "ready to place"
			// (0x734250 stores it per BuildCat for the place-building logic).
			VoxClass::Play("EVA_ConstructionComplete");
			reinterpret_cast<void(__thiscall*)(TechnoClass*)>(0x734250)(pObject);
			break;
		default:
			break;
		}
	}
}

void PerBuildingProduction::RelinkFactories(BuildingClass* pProducer)
{
	// 0x6A6140 = SidebarClass::SetFactoryForObject(FactoryClass*, AbstractType, int typeIndex):
	// finds the cameo for that type and stores the factory in it (Ares hooks the lookup).
	using SetFactoryForObject_t = bool(__thiscall*)(SidebarClass*, FactoryClass*, AbstractType, int);
	const auto SetFactoryForObject = reinterpret_cast<SetFactoryForObject_t>(0x6A6140);

	auto& sidebar = SidebarClass::Instance;
	const auto pPlayer = HouseClass::CurrentPlayer;

	auto link = [&](FactoryClass* pFactory, TechnoTypeClass* pType)
	{
		if (pType && CanProducerBuild(pProducer, pType))
			SetFactoryForObject(&sidebar, pFactory, pType->WhatAmI(), pType->GetArrayIndex());
	};

	for (const auto pFactory : FactoryClass::Array)
	{
		if (pFactory->Owner != pPlayer)
			continue;

		// The item being built right now (or finished and waiting to be placed)...
		if (const auto pObject = pFactory->Object)
			link(pFactory, pObject->GetTechnoType());

		// ...and everything queued behind it.
		for (const auto pQueued : pFactory->QueuedObjects)
			link(pFactory, pQueued);
	}
}
