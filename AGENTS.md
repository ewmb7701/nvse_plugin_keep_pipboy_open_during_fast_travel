# Keep Pip-Boy Open During Fast Travel

Standalone xNVSE plugin for Fallout: New Vegas 1.4.0.525. Requires xNVSE 6+.
Keeps the Pip-Boy open during map travel, refreshes/recenters the player marker,
and restores Pip-Boy HUD visibility after arrival. No ESP or scripts required.

Build `source/nvse_plugin_keep_pipboy_open_during_fast_travel.vcxproj` with MSBuild, Release/Win32 (v145).
Add `/p:RunTests=true` to build the isolated native test executable.
Copy `target/Release/keep_pipboy_open_during_fast_travel.dll` into `mods/Keep Pip-Boy Open During Fast Travel/nvse/plugins`.
Restart the game. Remove that DLL to restore vanilla closing behavior.

Reverse engineering from the running 1.4.0.525 executable:
- 0x798710 is the confirmation callback. Yes passes 0x798A00 to the UI manager's
  ClosePipBoy routine through call 0x79872E (E8 5D 6F F7 FF).
- The close adapter invokes the supplied continuation without changing Pip-Boy
  mode to closing. It preserves thiscall ret 4 cleanup. It does not process travel.
- Native callback 0x798A00 allocates/submits the game's position request and
  clears selectedMarker. Leave submission at 0x798A9D completely untouched.
- Main-loop routine 0x86E650 calls 0x86F940 at 0x86E760 regardless of menu pause.
  0x86F940 calls the native position processor 0x93BEA0 at 0x86F94F BEFORE its
  menu-pause check at 0x86F95F. Native travel therefore does not require forced
  processing by this plugin while the Pip-Boy is open.
- Hook the fast-travel-only JMP at 0x93BF2B (E9 41 02 00 00), after the
  call returns and the game sets its success byte at 0x93BF27. The naked
  adapter preserves general registers/flags, calls RefreshSceneAndMapAfterFastTravel,
  then resumes at the original jump target 0x93C171 with the stack unchanged.
- Do not hook the CALL at 0x93BF22. Live inspection confirmed JIP LN replaces
  that call after our DeferredInit, bypassing the 0.10 wrapper. Leave the call
  intact; no originalFastTravel variable or chaining is needed in 0.12.
- Install the four hooks directly with void WriteRelCall/WriteRelJump helpers.
  No byte guards, installation-result checks, request reads or identity snapshots.

Version 0.7 supersedes the forced submission wrapper in experimental 0.6.
No request submission hook, selection writes, forced processing, pending state,
executing flag, refresh queue, or xNVSE MainGameLoop travel listener is needed.
Version 0.10 removes request inspection entirely.
The original claim that menu pause prevented processing was incorrect; do not
reintroduce the workaround. See analysis/native-completion-hooks.md for evidence.

The plugin affects confirmed map travel without changing the executable on disk
or depending on Fast Travel Rerouted. Keep downstream native fast-travel hooks
intact. The inspected executable had other plugins loaded. Native tests cannot
establish rendering or complete in-game compatibility.

Native active-tab setup at 0x79AF30 restores +0x84 from +0x38 on the world-map
branch and calls SetupWorldMap at 0x79CDB0, including marker setup 0x79DBB0.
Call the higher-level routine; do not manually write +0x84 or +0x110.
The native texture-release routine 0x7A1670 sets global 0x11DA36C when it
clears the texture. SetupWorldMap consumes that flag to reload filename and
dimensions. The user confirmed the black background is gone in 0.9;
0.12 fixes the live-confirmed JIP call-site overwrite.
Require a map and world-map tab; skip if closing/closed.
Rely on the initialized MapMenu tiles; do not add redundant tile checks.
Restore native HUD kPipBoy state with 0x771700 (state 3) in the loading-finish
hooks while Pip-Boy mode is 3, not in the travel-completion helper.
See analysis/native-map-setup.md for the disassembly evidence.

Use the native setup's position and zoom behavior without any tile writes
afterward. Stewie's optional Map Remembers Position hooks 0x79E04E within this
path; position persistence belongs to that feature. Do not reintroduce custom
recentering, position/zoom preservation or a separate recenter setting.
Texture/marker restoration still runs when position persistence is enabled.
Native setup recreates missing markers; replacing a retained old-worldspace
marker catalog is not explicitly implemented. See TESTING.md for in-game checks.

## Development workflow

Use `scripts/build.ps1 -Configuration Release -Test` to build and run native
checks. Add `-Install` to copy the DLL to the standalone mod in this workspace.
Use `scripts/package.ps1 -Version 0.16` to create a mod-manager-ready ZIP in dist.
Keep the DLL filename stable: keep_pipboy_open_during_fast_travel.dll. The
nvse_plugin_ prefix identifies the source project, not a second runtime plugin.

Follow the reference layout: source contains C++ and the Visual Studio project,
scripts contains build/package entry points, .vscode contains editor settings,
target and dist contain ignored generated files. analysis contains local
reverse-engineering notes and is not packaged. Do not copy reference-game
headers or introduce runtime dependencies merely to match its file list.

Configuration: none as of 0.5. The old bRecenterAfterTravel INI is ignored;
installation leaves existing user INIs alone, and new packages contain no INI.
README.md and TESTING.md are copied to the installed mod and release package.

Version 0.12 uses the close CALL and post-travel JMP sites, with two direct hook
handlers (KeepOpen and AfterFastTravel) and one RefreshMap helper. The small
relative-branch memory-writing utility lives in source/hooks.h. No runtime callback
injection, singleton-slot overrides or wrapper layers exist just for testing.
Native tests cover close continuation and hook writing; game behavior needs
in-game validation. Keep this implementation small and direct.

Version 0.12 also hooks the loading-finish HUD calls at 0x457F37 and
0x457F8B (normal and timeout exits). These normally request gameplay state 2.
If the Pip-Boy is still fully open (UI +0x4BC == 3), request Pip-Boy state 3
instead through native HUD function 0x771700. Otherwise pass the state through.
This handles loading cleanup that occurs after the travel completion refresh;
no pending state, timer or per-frame handler is involved. Verify loading-screen
travel, closing the Pip-Boy afterward, and ordinary loading with it closed.
The user confirmed the loading-finish HUD fix. Runtime diagnostic logging was
subsequently removed; do not require a log message to validate this behavior.

Version 0.13 removes byte guards and installation-result checks. The four
working hooks and runtime menu-state guards are retained. Native tests verify
branch opcodes, destinations, the untouched neighboring call and page protection.

## Current implementation and validation (0.16)

Development details belong here; README.md is for installation and player-facing
behavior. Historical analysis files describe earlier experiments and may use
old function names. Current responsibilities:

- `SkipPipBoyCloseForFastTravel` invokes the native travel continuation.
- `HandleFastTravelDone` preserves registers/flags and calls
  `RefreshSceneAndMapAfterFastTravel` before resuming at 0x93C171.
- `RefreshSceneAndMapAfterFastTravel` checks the retained world map, calls
  native sky update and map setup, and requests native background recapture.
- `RestoreHudAfterLoading` preserves Pip-Boy HUD state when still open and
  otherwise passes the requested state through. Its scope is the two native
  loading-finish calls, not exclusively fast travel; no travel-pending flag exists.
- `HandleNvseMessage` installs the four hooks on DeferredInit (message 18).
- Named game wrappers, addresses, and field access live in source/game.h.
  Relative branch writes and function-pointer conversion live in source/hooks.h.
  Preserve the user's comments and names when changing source.

The native sky update is Sky::Advance(float) at 0x63AC70; singleton slot
0x11CCB78. Call it with zero delta to refresh destination lighting without
advancing game time. The user confirmed this fixed the stale lighting in 0.15.
See analysis/native-lighting-refresh.md for live measurements and disassembly.

The native Pip-Boy opening path sets byte 0x11D890A at 0x7F805E. The renderer
consumes and clears it at 0x86FF99/0x86FFA8, hides player meshes for the capture,
then calls 0x871DC0 at 0x87000F and restores their visibility. Set that engine
flag after travel to replace the cached background with the destination view.
Do not call the renderer directly from travel processing or introduce a plugin
queue. See analysis/native-background-capture.md. The user reported the 0.16
background fix worked following the Goodsprings-to-Long-15 reproduction.

0.14 removed the early HUD-state call from travel completion; HUD restoration
remains in the loading-finish hooks. The user confirmed a short
Goodsprings-to-Goodsprings trip. These reports are targeted gameplay checks,
not exhaustive compatibility coverage. Native tests verify continuation,
branch targets/opcodes, the untouched neighboring travel call, and protection
restoration. They do not prove rendering or all loading/camera combinations.

Do not add a saved travel target, position-request inspection, forced travel
processing, or game-loop polling. Leave the JIP LN travel call and Stewie's
optional native map-position hooks intact. The old custom position/zoom INI
was removed in 0.5; an existing INI is ignored. Map texture reload flags remain
owned by the game, and the plugin does not write map fields.
