# Experimental RA2/YR mod — design notes

Working branch: `per-building-production` (based on Phobos `develop` @ da18972c, 2026-11-08).
Test game folder: `F:\Games\RA2Mod\Game` (copy of the CnCNet YR install, updater disabled,
original DLL kept as `Phobos.dll.original-0.4.0.2`).

## Feature 1 — Per-building production (Generals / C&C3 style)

Every production building owns its own production queue instead of the house-wide
`FactoryClass` per category.

| Building kind      | Sidebar tab        | Own queue |
|--------------------|--------------------|-----------|
| Construction Yard  | Structures + Defenses (later: "Command Center" training Dozers) | yes, one per Yard |
| Barracks           | Infantry           | yes |
| War Factory        | Vehicles           | yes |
| Naval Yard         | Vehicles (naval)   | yes |
| Airfield / Air pad | Vehicles / aircraft | yes |

### Sidebar behaviour (decided)
- No production building selected → the production tabs are **empty**. Nothing can be queued.
- Selecting a production building → the sidebar switches to that building's tab and shows
  only what *that* building can produce, with *its* progress/queue/hold state.
- Selecting several buildings of the same kind: TBD (first version: use the first one selected).

### Aircraft (decided)
- Aircraft that need a landing spot (not helicopters / `AirportBound`-style units) stay as in
  vanilla: the cameo is greyed out and cannot be trained until a pad on that airfield is free.
  This matches C&C Generals.

### Milestone 1 — selection-driven sidebar (implemented, needs in-game test)
Code: `src/Ext/Sidebar/PerBuildingProduction.*`, rules key `[General] PerBuildingProduction=` (default **yes**).
- `SidebarClass::AddCameo` (hook 0x6A6316) refuses cameos the selected producer can't make.
- Ares' cameo removal (`SidebarExt::AresTabCameo_RemoveCameo`) removes hidden cameos *silently*.
  Important: the vanilla removal path sends Abandon/AbandonAll network events, so hiding must
  never go through it — production would be cancelled.
- Main loop (0x55DDA0) polls the selection; on change: `SidebarClass::RecheckCameos` (0x6A7D20),
  `BuildingClass::UpdateConstructionOptions()` on the producer, switch tab, repaint.
- Production is still house-wide underneath (one queue per category) — milestone 2 changes that.
- Requires Ares (the silent removal lives in the Ares redirect).

### Engine work (planned)
1. Per-building `FactoryClass` stored in `BuildingExt`.
2. Hook the house's "get primary factory" lookups used by sidebar/production so they answer
   with the selected building's factory.
3. Tab auto-switch on selection; empty tabs when no producer is selected.
4. New networked event carrying the target factory building (multiplayer lockstep safety).
5. Units exit from their own building (no "primary" building concept).
6. `[General] MultipleFactory=1.0` in rules so extra factories add parallel queues, not speed.
7. AI: queue at its least busy factory.
8. Save/load of the new per-building data.

## Feature 2 — Dozer (later)

- Placeholder model: MCV.
- Construction Yards become "Command Centers" that train Dozers.
- Idea under investigation: tie the building's **build-up animation frame to the construction
  progress %** (the building stays in its construction state while the dozer works; it pauses
  when the dozer leaves).

### Unfinished building rules (decided)
- Gives **no power** and **unlocks nothing** (no prerequisites, no production) until finished.
- **Can be attacked** by enemies while under construction.
- **Health grows** with construction progress (Generals style).
