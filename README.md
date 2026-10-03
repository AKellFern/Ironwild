# Ironwild

**An Unreal Engine 5 survival-game vertical slice, built as a gameplay and systems programming portfolio project.**

Ironwild is where I learn, design and debug gameplay systems in Unreal Engine. It started from a structured multiplayer survival course. It's now my own project: I study what a feature needs to do, then research, implement, integrate and debug it in my own way.

The current milestone is the **player character foundation**:

- Motion Matching locomotion (standing and crouched)
- A custom airborne animation state machine with data-driven jump and landing selection
- Animation retargeting pipelines that bring several animation sources onto one third-party character
- An editor-only C++ plugin that records per-frame runtime state during play-in-editor (PIE) sessions, for investigating animation and movement bugs

Survival mechanics come next.

| | |
|---|---|
| **Engine** | Unreal Engine 5.8.1 |
| **Languages** | C++ (editor tooling plugin), Blueprints (gameplay and animation, currently) |
| **Focus areas** | Character movement ↔ animation integration, Motion Matching / Pose Search, retargeting, runtime diagnostics |
| **Status** | Active development: character and animation foundation in progress; survival systems not started |
| **Internal name** | The `.uproject` and asset paths still use `SurvivalGame`. That's deliberate for now. |

---

## At a glance

- **Motion Matching locomotion.** Standing and crouched locomotion run on Pose Search, with separate databases switched by crouch state. Both databases share one schema.
- **Custom airborne state machine.** Jump, fall and landing states (`SM_Airborne`) sit alongside Motion Matching. Clip selection depends on speed tier, the active foot and landing impact, and is driven by data.
- **Crouch-jump as its own feature.** Jumping while crouched works by extending Unreal's jump check rather than replacing it, so the engine's built-in safeguards stay intact.
- **Retargeting pipelines.** Several animation sources (Epic's Game Animation Sample, a third-party combat animation pack) are retargeted onto a third-party character. Shared IK Rigs keep this consistent.
- **Runtime diagnostics tooling.** An editor-only C++ plugin samples input, movement and animation-graph state every frame during PIE and writes it to CSV. Nearly every animation fix in this repo was checked against these captures and frame-by-frame video.

---

## Current status

| System | Status | Notes |
|---|---|---|
| Third-person character (mesh, skeleton, camera modes) | ✅ Implemented | Third-party survival character mesh on a UE5-mannequin-compatible skeleton; first-person / third-person view actions |
| Enhanced Input setup (move, look, jump, crouch, walk, sprint) | ✅ Implemented | Walk rebound off Left Alt (see [debugging notes](#selected-engineering-problems)) |
| Motion Matching locomotion: standing (walk / run / sprint) | ✅ Implemented | `PSD_Survival_Locomotion` |
| Motion Matching locomotion: crouched | ✅ Implemented | `PSD_Survival_Crouch`, shared schema `PSS_Survival_Locomotion` |
| Standing jump: start, fall, land | ✅ Implemented | Data-driven clip selection by speed tier and foot phase |
| Crouch-jump: takeoff and fall | ✅ Implemented | Dedicated `JumpStart_Crouch` / `FallLoop_Crouch` states; fall pose blends continuously with vertical velocity |
| Crouched landing impact (pelvis dip) | 🚧 In progress | Dip currently moves the whole mesh, so the feet sink ~8 cm at its lowest point; moving it to the pelvis with leg IK |
| Standing landing: return to locomotion | 🐞 Known issue | `Land` state can hold until the next input; suspected live-vs-latched ground-speed read |
| Crouched air-control speed | ❓ Design decision | Crouch-hops pick up standing air speed; deciding whether that's intended |
| Runtime diagnostics plugin (C++, editor-only) | ✅ Implemented | See [Runtime diagnostics](#runtime-diagnostics-c-editor-plugin) |
| Core gameplay in a C++ game module | 📋 Planned | Gameplay is currently Blueprint; see [How C++ is used](#how-c-is-used-today) |
| Survival systems (interaction, inventory, harvesting, crafting, stats) | 📋 Planned | Sequenced from the course roadmap; not yet in the repository |
| Multiplayer / replication | 📋 Planned | Not implemented; current jump logic is written with prediction constraints in mind |

---

## Architecture

### Character and input

The playable character (`Content/FirstPerson/Blueprints/BP_FirstPersonCharacter`) derives from `ACharacter` and uses `UCharacterMovementComponent`. It started from Epic's First Person template, and is now a third-person survival character. It carries:

- **Enhanced Input** bindings (`Content/Input/`): Move, Look, Jump, Crouch, Walk, Sprint, and first-person / third-person view toggles.
- **Crouch and jump rules.** Unreal blocks jumping while crouched by default. Ironwild allows it by overriding `CanJumpInternal`. The override rebuilds the stock UE 5.8.1 check (ledge/fall guard, `JumpMaxCount`, `bWantsToCrouch` gate) and adds one narrow exception for jumping from a grounded crouch. Crouch also persists through landing, which matches stock engine behavior.

The crouch-jump exception only reads state the Character Movement Component already predicts and replicates (`bIsCrouched`, `IsFalling()`, `JumpCurrentCount`). It adds no separate Blueprint flag. The game isn't networked yet, but this keeps a later multiplayer pass from turning the rule into a client/server prediction mismatch.

### Animation: Motion Matching plus an airborne state machine

The animation Blueprint is `Content/Animation/MotionMatching/ABP_Survival_MotionMatching`.

```
ABP_Survival_MotionMatching
└── SM_Airborne (state machine)
    ├── Ground ─── Blend Poses by Bool (isCrouched)
    │                ├── true  → Motion Matching → PSD_Survival_Crouch
    │                └── false → Motion Matching → PSD_Survival_Locomotion
    │                            (both databases share schema PSS_Survival_Locomotion)
    ├── JumpStart         → standing jump start, chosen by JC_SelectJumpStart
    ├── FallLoop          → standing fall loop
    ├── Land              → landing clip + start offset, chosen by JC_SelectLanding
    ├── JumpStart_Crouch  → AS_E_Crouch_to_Jump
    └── FallLoop_Crouch   → blend(takeoff end pose → crouch pose), alpha driven by VerticalVelocity
                            (crouched landings go straight to Ground, skipping Land)
```

<!-- TODO(diagram): Replace the tree above with an exported screenshot of SM_Airborne from the editor. -->

Design decisions worth calling out:

- **Motion Matching for locomotion, explicit states for airborne.** Pose search handles continuous ground movement well. Jumps and landings are short, timing-sensitive moments, so they're explicit states with selection logic I can tune and test directly.
- **Data-driven jump and landing selection.** Clips are chosen from speed-tier thresholds (`JC_StandSpeedThreshold` / `JC_RunSpeedThreshold` / `JC_SprintSpeedThreshold`), the active foot, and a heavy-landing threshold on downward speed. Each landing clip's start offset lives in a lookup table (`JC_LandStartMap`) instead of being hard-coded. The offsets were computed offline from the clips themselves with editor Python scripts.
- **Separate crouch states rather than shared tuning.** Standing and crouched jump starts first went through one shared transition, whose blend duration served two conflicting goals: standing jumps wanted 0.05 s, crouch jumps wanted more. Splitting them into dedicated states fixed this structurally.
- **Blending by vertical velocity instead of time.** The crouched fall blends from the takeoff end pose into the crouch pose using `VerticalVelocity`: 0 at the top of the jump, fully tucked just before touchdown. Two time-based versions read badly: holding the takeoff pose looked like a freeze, and a single timed crossfade looked like a snap.

### Retargeting pipeline

The character is a third-party survival character on its own skeleton (`SKEL_Survival_Character`). It's bone-for-bone compatible with the UE5 mannequin: 161/161 bones, same order. The animation sources are retargeted onto it:

| Source | Retargeter | Output folder |
|---|---|---|
| Epic Game Animation Sample locomotion / jump / land set (UEFN mannequin) | `RTG_UEFN_to_Survival` → ShoulderFix variant | `Content/Characters/Survival_Retargeted_ShoulderFix/`, `RetargetedAnimations/` |
| En_Combat animation pack (crouch idle, crouch-to-jump) | `RTG_EnCombat_to_Survival` | `Content/Characters/Survival_Retargeted_EnCombat/` |

All retargeters share one target IK Rig (`IK_Survival_Character`) and one operation stack: Pelvis Motion → FK Chains → Root Motion → Remap Curves. That way a pose or rig fix lands in one place. The ShoulderFix variant corrects shoulder, jacket and backpack deformation from the UEFN skeleton's different bone conventions.

---

## How C++ is used today

Right now, the project's C++ is **engine-level tooling, not gameplay**. Gameplay and animation logic live in Blueprints, because the current milestone is mostly animation-graph and character-movement integration.

Next, core gameplay moves into a C++ game module, starting with the character and the survival systems. Moving the crouch-jump rule from its Blueprint override into C++ is a natural first step: it's already written as an extension of the engine's own check.

### Runtime diagnostics (C++ editor plugin)

[`Plugins/RuntimeDiagnostics`](Plugins/RuntimeDiagnostics) is a content-free, **editor-only** plugin with two modules. It observes PIE characters without modifying their classes, input or assets.

**`RuntimeDiagnostics`: core module** ([source](Plugins/RuntimeDiagnostics/Source/RuntimeDiagnostics))

- `URuntimeDiagnosticsSubsystem` (`UEditorSubsystem`) hooks `FWorldDelegates::OnWorldPostActorTick` and samples the selected character once per game tick. Each sample is one CSV row with structured JSON columns:
  - **Input:** raw key state plus each Enhanced Input action's value, trigger event and elapsed time.
  - **Movement:** location, velocity, acceleration, movement mode, crouch/fall flags, and floor hit / distance / normal from the Character Movement Component.
  - **Animation graph:** every sequence player's asset, time, start position and blend weight, plus each state machine's current state and time in state. It reads these generically through `IAnimClassInterface::GetAnimNodeProperties()`, with no game-specific code.
- **Optional adapter.** `SurvivalDiagnostics.cpp` uses `TFieldIterator<FProperty>` to read the animation Blueprint's selection variables (`JC_*`, `VerticalVelocity`, `isCrouched`) by reflection. The plugin doesn't link against any game module or asset.
- **Bounded, fail-safe capture:**
  - Capture length is capped at 300 s.
  - A second capture can't start while one is running.
  - A watchdog stops the capture if the character is destroyed or time runs out.
  - Captures close automatically when PIE ends.
  - A write error stops the capture and records the reason.
- **Multi-world aware.** You can select PIE instances and players explicitly, and each world reports its net mode, so multi-client sessions can be captured.

**`RuntimeDiagnosticsMCP`: bridge module.** It registers the same API as a `ToolsetRegistry` toolset, so editor automation tools can query it. It's separate so the core module has no dependency on the experimental toolset plugin.

Every animation fix in this README was investigated by lining these captures up against recorded gameplay video frame by frame.

<!-- TODO(media): Add an annotated capture strip, e.g. video frames with state / blend-weight labels under each frame. Good candidate: the crouch-jump fall-blend investigation. -->

---

## Selected engineering problems

Short case studies from the animation and movement work. Each followed the same loop: reproduce, capture, line the data up against video, form a hypothesis, fix, verify with a fresh capture.

- **Edits that had no effect.** Two changes to the jump-start blend time made no measurable difference. The captures showed they had landed on `Land → Ground`, not `Ground → JumpStart`. Every transition in the state machine shared the same priority and mostly the same variables, so the only reliable way to identify one was to trace its rule logic and the arrow it draws in the graph.
- **A replaced engine check.** An early `CanJumpInternal` override replaced the engine's whole jump-eligibility check with `JumpCurrentCount < JumpMaxCount`. That silently dropped the ledge/fall guard, so the character could air-jump after walking off any ledge. I rebuilt it from the stock 5.8.1 logic plus one narrow exception. The rule since: extend engine virtuals, never wholesale-replace them.
- **Feet sinking on crouched landings.** Playing the crouch-to-jump clip in reverse as a fake landing put the feet below the floor. The clip's opening frames were authored for a character already in the air, not one standing on the ground. I dropped the reversed clip, and crouched landings now go straight back to locomotion.
- **"Freeze then snap" during the crouched fall.** Holding the takeoff pose for the whole fall looked frozen; a single timed crossfade looked like a snap. Blending continuously by vertical velocity fixed both.
- **Input that never fired.** `IA_Walk` was bound to Left Alt, which Windows handles as a system-menu key (`WM_SYSKEYDOWN`), stealing focus from the PIE viewport. Captures showed the action never triggered even with the key held. Walk was rebound to `C`.
- **Sliding with frozen legs.** Movement, speed tier and state-machine data were all correct while the legs didn't animate. The cause was isolated to pose selection. An editor restart cleared it, consistent with a stale Pose Search index; it hasn't recurred, so that diagnosis is unconfirmed.

---

## Project origins

Ironwild began as my project for Smart Poly's **[UE5 Multiplayer Steam Survival Game Course – Remastered](https://smartpoly.teachable.com/p/ue5-multiplayer-steam-survival-game-course-remastered)**.

The course was the initial learning scaffold. It still sets the rough order of features: I watch the relevant section to understand what a mechanic is supposed to do and how the course approaches it. Then I decide how it should work in Ironwild:

- research current Unreal Engine features and alternatives
- design an implementation that fits this project's architecture
- integrate, debug and verify it with the tooling above

How the repository breaks down today:

| Category | What's in the repo |
|---|---|
| Feature roadmap from the course | Survival-game scope and the order of upcoming systems |
| My own design and implementation beyond the course | Motion Matching locomotion (replaced an earlier Blend Space state machine), the airborne state machine and data-driven jump/land selection, crouch-jump and crouched-fall blending, the retargeting setup, the `CanJumpInternal` extension |
| Built for this project, independent of the course | The `RuntimeDiagnostics` plugin and the capture/video analysis workflow |
| Epic Games content and templates | First Person template base, Game Animation Sample locomotion content and helper Blueprints, UE5/UEFN mannequins |
| Third-party assets | Survival character mesh pack; En_Combat animation pack |

<!-- TODO: if any course-implementation Blueprints remain substantially intact as survival systems land, list them here explicitly. -->

---

## Development workflow and AI-assisted tooling

I use AI tools (Anthropic's Claude) as development support. Mostly that means:

- diagnosing runtime problems
- analyzing diagnostic captures, logs and recorded gameplay frame by frame
- researching engine internals
- documentation
- repetitive editor and repository tasks, including git housekeeping

Some editor-side changes have been applied through an MCP connection to the Unreal Editor, at my direction.

I use AI as development tooling rather than as a substitute for implementation. It does not autonomously author the gameplay systems represented by this portfolio project. I own the engineering decisions and implementation: what to build, how systems are structured, which approaches to take, and what ultimately goes into the project. AI-assisted changes and diagnoses are reviewed, tested in PIE, and verified against runtime evidence before I keep them.

---

## Third-party content and attribution

The project uses existing art and animation content so the work can focus on programming and systems integration. **I did not create these assets.** My work is the systems built around them: retargeting setup, animation graphs, selection logic, movement rules and tooling.

| Content | Source | Location |
|---|---|---|
| First Person template (base character, game mode, input, prototyping level) | Epic Games (Unreal Engine template) | `Content/FirstPerson/`, `Content/Input/`, `Content/LevelPrototyping/` |
| Game Animation Sample content (locomotion animations, helper Blueprints, data types, foley notifies) | Epic Games (Game Animation Sample Project) | `Content/Blueprints/`, `Content/Characters/UEFN_Mannequin/`, `Content/Audio/`, retargeted copies under `Content/Characters/` |
| UE5 Mannequins | Epic Games | `Content/Characters/Mannequins/` |
| Survival character mesh and materials | [Survival Character](https://www.fab.com/listings/11d20d01-b764-4936-8163-cb20d05c369e) (Fab) | `Content/Survival_Character/` |
| En_Combat animation pack | [Environmental Combat Animation Pack](https://www.fab.com/listings/1c7dc7c9-4ea4-413f-b0a7-b3c37192f7c6) (Fab) | `Content/En_Combat/` |
| Course material and feature roadmap | Smart Poly, [*UE5 Multiplayer Steam Survival Game Course – Remastered*](https://smartpoly.teachable.com/p/ue5-multiplayer-steam-survival-game-course-remastered) | — |

All third-party content is subject to its own license. Unreal Engine content is used under the Unreal Engine EULA.

---

## Building and running

**Requirements**

- Unreal Engine **5.8.1**
- **Git LFS**: all `.uasset` / `.umap` and binary media are stored in LFS
- A Visual Studio C++ toolchain supported by UE 5.8, to compile the editor plugin

**Steps**

1. Clone with LFS, then pull the LFS objects:
   ```bash
   git lfs install
   git clone https://github.com/AKellFern/Ironwild.git
   cd Ironwild
   git lfs pull
   ```
   Active development currently happens on the `character-setup` branch.
2. Open `SurvivalGame.uproject`. Plugin binaries aren't committed, so the editor will offer to compile `RuntimeDiagnostics`. Alternatively, build the plugin from PowerShell:
   ```powershell
   Plugins/RuntimeDiagnostics/Build.ps1 -EngineRoot "<UE_5.8 install>" -ProjectFile "SurvivalGame.uproject"
   ```
3. Press Play. The default map is `Lvl_FirstPerson`.

**Notes**

- The project enables the experimental `ModelContextProtocol` and `ToolsetRegistry` editor plugins. They're used only by the diagnostics bridge module.
- To run without them, remove the `RuntimeDiagnosticsMCP` module entry from `RuntimeDiagnostics.uplugin`. The core diagnostics module doesn't depend on them.

---

## Roadmap

1. **Finish the character foundation.**
   - Pelvis-only landing dip with leg IK foot planting.
   - Fix standing landings that hang in `Land`.
   - Decide on crouched air control.
2. **Add a C++ gameplay module.** Move the character and movement rules into C++ first.
3. **Build survival systems,** following the course's feature order but designed for this architecture:
   - interaction
   - inventory
   - harvesting and resources
   - crafting
   - player stats
4. **Add networking.** Bring in replication and move prediction-sensitive rules into the Character Movement Component's predicted state.
