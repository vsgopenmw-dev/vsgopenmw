# vsgopenmw — upstream merge plan

Living document. Update at the end of every session.

## Objective

Keep the VSG overlay riding as thin a diff as possible on top of stock upstream openmw. Every non-rendering divergence is a merge conflict every future upstream pull.

## Strategy

Instead of the direct 0.49→0.51 jump (which produced 174 `ponytail:` markers on `merge/upstream-0.51` — the current mess), step one upstream version at a time and validate rendering at each hop:

`master (0.49 + VSG, working)` → merge upstream 0.50 → validate → merge upstream 0.51 → validate

At each hop we A/B-compare a scripted render from the vsg build against stock upstream at the same version, using automated scripts (see "Test rig" below).

## Baselines

| Branch | Version | State | Notes |
|---|---|---|---|
| `master` | fork 0.49 + VSG | **Working**. `--skip-menu --new-game --start "Balmora"` loads 9 cells, 0 fallbacks, 0 Lua errors. Quits peacefully. Movement broken (pre-existing fork bug, not blocking merge). Run cwd must be `OpenMW.app/Contents/MacOS/` (macOS bundle-relative config path). | This is our render baseline. |
| `merge/upstream-0.51` | fork attempted 0.49→0.51 | Mess of ~174 ponytail shims. Retire once we've cherry-picked the fixes below. Uncommitted WIP stashed as `merge/upstream-0.51 WIP: fixes for possible cherry-pick`. | Keep for cherry-picks only. |
| `../openmw` (separate repo) at `openmw-50` | stock upstream 0.50 | Being built by a subagent. Will be the reference for A/B renders during the 0.49→0.50 merge. |

## Test rig

Confirmed by subagent probe on stock upstream 0.50: **`--script-run <file>` piping `lua …` lines does NOT work reliably.** Bug: `ui.setConsoleMode` is queued via `luaManager->addAction` (see `apps/openmw/mwlua/uibindings.cpp:112` on stock upstream), so a `lua player` on line 1 doesn't take effect until the next frame — but `--script-run` executes every line synchronously before the frame loop starts. Line 2 arrives with `mConsoleMode == ""` and the console just prints usage help. Present on stock 0.50; will be present on the vsg 0.50 merge; would not be worth fixing via the script-run path.

Subagent added two engine flags **on stock upstream 0.50** to sidestep this:

- `--auto-screenshot=<frames>` — after N frames in `State_Running`, take one screenshot (via a new `openmw.debug.takeScreenshot()` Lua binding).
- `--auto-quit=<frames>` — after N frames in `State_Running`, `requestQuit()`.

Wrapper script `tools/render_gold.sh` in the stock upstream repo (`/Users/bret.curtis/Workspace/private/openmw`) wraps them with a 20s wall-clock kill. Real runtime ~4s. **We should mirror these three additions on the vsg side** so both sides have parity — cherry-pick candidates:
- `apps/openmw/mwlua/debugbindings.cpp` — `openmw.debug.takeScreenshot()` binding
- `apps/openmw/engine.hpp` + `engine.cpp` — auto-screenshot / auto-quit frame counters in main loop
- `apps/openmw/options.cpp` + `main.cpp` — CLI wiring
- `tools/render_gold.sh` — the recipe

Render comparison target: **Bob_Bitchen/Quicksave.omwsave** — Seyda Neen dock (post-tutorial). Busy frame: water, dock, palms, HUD, minimap. Gold screenshot at `~/Library/Application Support/openmw-050/screenshots/screenshot000.png`.

## Working plan — 0.49 → 0.50

- [x] Confirm `master` (0.49+VSG) builds and runs.
- [x] Subagent (`../openmw` on `openmw-50`): built stock upstream 0.50, discovered `--script-run + lua` async bug, added `--auto-screenshot`/`--auto-quit` flags + `openmw.debug.takeScreenshot()` binding + `tools/render_gold.sh` wrapper, captured gold render at Bob_Bitchen/Quicksave (Seyda Neen dock).
- [ ] **Mirror the auto-screenshot/auto-quit tooling on the vsg side** (before merging 0.50), verify it works against master (0.49+VSG loading the same save), and capture a vsg-side reference screenshot. Compare to gold — if 0.49 vs 0.50 renders diverge already for Morrowind content, we'll know it's a version/schema thing and not caused by the merge.
- [ ] Cut `merge/upstream-0.50` from `master`.
- [ ] `git merge upstream/openmw-0.50`. Expect small diff surface (much smaller than 0.49→0.51).
- [ ] Fix compile conflicts. Prefer adopting upstream signatures over adding fork shims. Add `ponytail:` marker only if there's no cleaner path (rendering-touching or truly fork-only).
- [ ] Run `tools/render_gold.sh` on merged branch. Diff PNG against upstream gold.
- [ ] Commit clean. Merge into master. Retire branch.

## Working plan — 0.50 → 0.51 (after 0.50 lands)

- [ ] Repeat the same sequence. New subagent for a stock 0.51 baseline in `../openmw`.
- [ ] Now that render + input rig is proven at 0.50, the 0.51 delta should show up as diffable regressions rather than "everything is on fire".

## Cherry-pick candidates from `merge/upstream-0.51`

Real fixes worth keeping (still relevant after the clean rebase); everything else lives and dies with that branch.

| Fix | File | Take? | Why |
|---|---|---|---|
| `getBloodTexture` on `MWWorld::Class` (virtual) + Creature override | `apps/openmw/mwworld/class.hpp`, `mwclass/creature.{hpp,cpp}`, `mwclass/npc.hpp`, `worldimp.cpp` | Yes | Real virtual gap. |
| `useTorches` null-guard | `apps/openmw/mwworld/worldimp.cpp:3010` | Yes | Cheap defensive. |
| `Game::mLuaManager` declared before `mWorld` | `apps/openmw/game.hpp` | Yes | Real lifetime bug (dtor order). |
| `Preview::mContext` before `npc` | `apps/openmw/mwrender/preview.hpp` | Yes | Same class of dtor-order fix. |
| `Resource::ScaledTriangleMeshShape` owning wrapper | `components/resource/bulletshape.hpp`, `components/nifbullet/bulletnifloader.cpp` | Yes | Real physics correctness for scaled statics. |
| `getModel` returns `string_view` from persistent `mBase->mModel` | `apps/openmw/mwclass/classmodel.hpp` | Only if `master` already has the same bug — verify. |
| `main.cpp` comma-operator `newGame` fix | `apps/openmw/main.cpp:91` | Yes — trivial, independent. |
| `MacOsPath::getLocalPath()` returns absolute path | `components/files/macospath.cpp` | Consider — makes running the binary from any cwd work (currently only works from inside the bundle's MacOS dir). |
| `mHitAttemptActorId` real int storage + session counter | `mwmechanics/creaturestats.{hpp,cpp}` | **Skip** — better upstream path is to migrate fork to `ESM::RefNum` end-to-end (long-term task). |
| ACTC `skipRecord` | `mwmechanics/creaturestats.cpp` | Skip — 0.49 doesn't have the ACTC schema this reads. |
| `LuaStorage::setActive(true)` on init | `mwlua/luamanagerimp.cpp` | Skip — different storage API on 0.49. |
| Force-add `builtin.omwscripts` | `main.cpp` | Skip on 0.49; **revisit on 0.50 merge** — 0.50 has the framework this feeds. |
| `input.ACTION_TYPE` compat shim in inputbindings.cpp | `mwlua/inputbindings.cpp` | **Skip and never revive.** This is the biggest anti-pattern in the 0.51 branch — writing a fake compat layer for a real API. Better to port the real API when the time comes. |
| `nodeOptions.findFileCallback` guards against double `meshes/` prefix | `components/resource/resourcesystem.cpp` | Yes — the callers-pass-mixed-paths problem exists on master too. Verify + apply. |
| `actorOptions.findFileCallback` same guard | `apps/openmw/mwrender/animcontext.cpp` | Yes — same. |
| `correctTexturePath` used in texture findFileCallback | `components/resource/resourcesystem.cpp` | Cleaner than what's there. |
| `HeightfieldSurface::mSize = verts` (not `CellSizeInUnits`) | `apps/openmw/mwworld/scene.cpp` | Skip — bug was created by the 0.51 merge tail, doesn't exist on 0.49. |
| `LocalScripts::remove(RefData*)` overload | `mwworld/localscripts.{hpp,cpp}`, `worldimp.cpp` | Yes — real fix for phantom-script bug on count=0. |
| `RestPermitted` treated as enum not bitmask | `apps/openmw/mwgui/waitdialog.cpp` | Yes if the same code shape exists on master — verify. |

## Known bugs on `master` (0.49+VSG) unrelated to the merge

- **Movement doesn't work.** WASD does nothing even in `tcl`. Likely a fork-specific mwinput/mechanics glue break — not touched by the 0.49→0.51 merge attempt. Should be investigated as its own issue.
- Bundle-relative config: `MacOsPath::getLocalPath()` returns `"../Resources/"` (relative). Must run the binary from inside `OpenMW.app/Contents/MacOS/`, not from `build/`.
- Lua `Unknown tag 'MENU'` warnings when loading vfs-mw scripts. Cosmetic on 0.49 — the scripts get rejected wholesale rather than partially loading. Resolves itself on 0.50.

## Rules of engagement for the merges

1. **Prefer adopting the upstream signature** over adding a fork shim. Every shim = future merge conflict.
2. **Only add a `ponytail:` marker** when the file being touched is *rendering-specific* or the divergence is genuinely irreducible (e.g., "OSG → VSG" type things). Never for a signature mismatch that could be fixed by touching the call site.
3. **Every merge commit must be validated** by a `--skip-menu --new-game --start "Balmora"` run that:
   - Loads cells without fallbacks (except `water_nm.png`, which upstream doesn't ship either)
   - Reaches "Quitting peacefully" on clean SIGTERM
   - Matches the same-version stock upstream gold render (visual A/B)

## Additional 0.50 gotchas surfaced by subagent probe

- **`--start "Balmora"` is silently ignored during `--new-game`.** The intro cell (`Imperial Prison Ship`) wins because `mStartCell` is only honored after the intro sequence. Use `--load-savegame` for reproducible A/B, not `--new-game --start`.
- **`--user-data <dir>` redirects saves/screenshots but NOT the primary config dir.** To fully isolate, use `--config <dir> --replace=config` (or seed `~/Library/Preferences/<name>/openmw.cfg` by hand). The subagent seeded `~/Library/Preferences/openmw-050/` from the real user cfg to avoid clobbering vsg's cfg.
- **`auxUtil.shallowCopy`** is a fork/master-only helper (commit `e978c230dc`, Oct 2025). NOT in stock 0.50. Any vsg-side Lua that imports it will explode on 0.50; audit before/after the merge.
- **Stale `resources/vfs-mw/scripts/`** from an older build broke actor scripts on the subagent's stock 0.50 (leftover `combat/local.lua`). Do a clean install of vfs-mw when landing 0.50.

## Session log

- **2026-09-08** — Reoriented from "keep fixing 0.51 branch" to "step-merge from master 0.49". Verified master baseline (works). Stashed 0.51-branch WIP. Spawned subagent to build stock upstream 0.50 in `../openmw` for A/B gold render.
- **2026-09-08 (cont.)** — Subagent completed: stock upstream 0.50 built (`OpenMW 0.50.0 rev 47d78e004b`), gold render captured for Bob_Bitchen/Quicksave (Seyda Neen dock) at `~/Library/Application Support/openmw-050/screenshots/screenshot000.png`. Discovered `--script-run + lua` async bug in stock upstream 0.50 → subagent added `--auto-screenshot`/`--auto-quit` engine flags + `openmw.debug.takeScreenshot()` lua binding + `tools/render_gold.sh` wrapper (20s hard timeout). These four bits should be **cherry-picked to the vsg side** before the 0.50 merge so both sides have parity. Plan doc updated with the new gotchas + tooling.
