# Nemisis test audit

Baseline: `ff88eb0a7997f238cb76381dc377525c942991c1`, inspected 2026-10-18.
Read all **33 test sources / 177 test functions**, their fixtures/assertions and
CMake registrations, plus the shared runtime-smoke runner. This is a function/
behavior inventory, not merely an executable list. Function names below omit the
leading `test`; render-scene names also omit `DevRangeRenderScene`.

CMake registers 33 executables and four runtime smokes. CPU presets select 34
cases; the three graphics smokes retain their `gpu` label and active-backend
requirement. No registration, function or assertion is removed by this PR.

Categories: CONTRACT = stable behavior; REGRESSION = focused failure/edge-case
protection (historical bug provenance not assumed); INTEGRATION = subsystem seam;
GAME-SPECIFIC = game/tool policy, correctly owned here; SCAFFOLDING = development
structure; UNCERTAIN = not sufficiently understood and retained. Decisions are
KEEP/CONSOLIDATE/MOVE/DELETE/REVIEW. H/M/L means removal risk without replacement.

## Input, movement and collision

| Source and functions/groups | Category | Protected behavior | Decision / risk |
| --- | --- | --- | --- |
| `input_command_tests.cpp`: `MovementActionsBuildCommand`, `PressedActionsBuildCommand`, `HeldActionsBuildCommand`, `ControllerActionsBuildCommand`, `ControllerDeadzoneDoesNotMove`, `MouseLookBuildsCommand`, `ControllerLookBuildsCommand`, `LookSensitivitySettingsScaleCommand`, `WeaponInteractionActionsBuildCommand` | INTEGRATION | Bindings reach move/look/device/pressed/held/ADS/weapon command fields | KEEP / H |
| Same: `DevRangeUtilityActionsAreBound` | GAME-SPECIFIC | Existing V-key drill selection | KEEP / M; active tool |
| Same: `GameplayInputBufferCarriesEdgesAcrossFrameBeforeFixedTick` | REGRESSION | Tap/release before next tick preserves jump/double-jump/reload exactly once | KEEP / H |
| `movement_replay_tests.cpp`: `SprintReplay`, `GroundAccelerationAndFrictionReplay`, `DashCooldownReplay`, `SlideDurationAndSlideJumpReplay`, `CrouchTransitionReplay`, `AirControlPreservesMomentumReplay` | GAME-SPECIFIC | Current caps, acceleration/friction, timers, momentum and crouch interpolation | KEEP / H |
| Same: `JumpDoubleJumpReplay`, `SecondSpacePressUsesAirJumpNotCoyoteGroundJump`, `BufferedDoubleJumpSurvivesCoyoteWindow`, `CoyoteJumpReplay`, `JumpBufferReplay`, `LandingConsumesBufferedJumpImmediately`, `SlideBufferMakesSprintSlideReliable`, `LandingAndBufferedSlideTransitionReplay` | REGRESSION | Distinct ground/air jump consumption, coyote/buffer boundaries, same-tick landing/slide | KEEP / H; different timing scenarios |
| Same: `WallRunContactAndWallJumpReplay`, `WallRunContactGraceReplay`, `MantleCandidateReplay`, `WallRunTransitionDoesNotRetriggerReplay` | REGRESSION | Contact/grace/cooldown, mantle interpolation and nonrepeating presentation cues | KEEP / H |
| Same: `IdenticalCommandReplayIsStable`; `MovementTuningConfigReplay` | CONTRACT; INTEGRATION | Equal commands reproduce state; settings reach tuning | KEEP / H |
| `greybox_world_tests.cpp`: `DevRangeGreyboxComposition` | SCAFFOLDING | Current blockout counts and named collision proxies | REVIEW, retain / M |
| `greybox_collision_tests.cpp`: `BoundsClamp`, `CoverPushout`, `OpenLaneStaysFree`, `RampSurfaceGroundsPlayer`, `AssetCollisionProxiesBlockAndGroundPlayer`, `LowStepIsWalkable` | INTEGRATION | Map/physics adapter, clearance, support and correct contact identities | KEEP / H |
| Same: `RisingJumpDoesNotSnapBackToGround`, `AirborneStepUpCanBeDisabled`, `LeavingRaisedSupportBecomesAirborne` | REGRESSION | Rising-jump snap prevention, airborne policy and no stale support | KEEP / H |
| Same: `SweepStopsFastLedgeTunneling`, `SweepAllowsValidLowStep`, `MidLedgeStillBlocksWithoutMantle`, `MidLedgeReportsMantleCandidate`, `WallRunPanelReportsSurfaceContact`, `SweepReportsWallRunPanelContact` | REGRESSION | Fast-motion collision, valid low steps, mantle and wall probes | KEEP / H |
| Same: `DepenetrationIsIndependentOfColliderInsertionOrder`, `CapsuleSweepUsesRoundedCorner`, `VerticalCapsuleSweepCatchesFastLanding`, `GroundSnapDistanceAndSlopeLimit` | CONTRACT | Public engine physics: overlapped depenetration order, rounded-corner clear path, vertical sweep, explicit snap/slope limits | MOVE candidate later, retain now / H; no equivalent pinned-engine cases |

## Player, presentation and assets

| Source and functions/groups | Category | Protected behavior | Decision / risk |
| --- | --- | --- | --- |
| `player_view_tests.cpp`: `ApplyLookWrapsYawAndClampsPitch`, `ViewVectorsUseYaw`, `CommandMovesRelativeToView` | CONTRACT | Look bounds and +Z-facing/world-relative movement basis | KEEP / H |
| `player_camera_rig_tests.cpp`: `CameraInitializesAtEyeHeight`, `SprintExpandsFovAndBob`, `WeaponRecoilAffectsVisualView`, `AdsNarrowsVisualFov` | GAME-SPECIFIC | Eye, sprint, recoil and ADS presentation | KEEP / H |
| Same: `FastLookProducesBoundedWeaponInertia`, `LandingKickStartsAndRecovers` | REGRESSION | Bounded sway/ADS stabilization and decaying landing impulse | KEEP / H; renamed second test, no physics-state proof claimed |
| `player_first_person_rig_tests.cpp`: `RigBuildsStableSkeletonHierarchy` | CONTRACT | Semantic hierarchy, visible hands and attachment names | KEEP / H |
| Same: `AdsMovesWeaponAndHandsTowardSightline` | GAME-SPECIFIC | Weapon centering/raising/pullback, arms lift and clipping scale | KEEP / H |
| Same: `LookDownRevealsBodyWithoutLiftingItIntoCamera` | REGRESSION | Body reveal without camera-root clipping | KEEP / H |
| Same: `ReloadFireAndWallrunDriveSocketTransforms`, `CameraBasisDrivesWeaponSocketsAndHands` | CONTRACT | Additive presentation, camera-to-mesh Euler sign/full roll, forward basis and hand/grip coincidence | KEEP / H |
| Same: `SidearmUsesShorterMuzzleAndUprightRoll` | GAME-SPECIFIC | Shorter barrel/socket and upright normalized roll | KEEP / H; renamed |
| Same: `RigSocketsExposeCameraHandsAndWeaponAttachments` | CONTRACT | Camera/weapon/muzzle/eject/grip placement and ADS socket movement | CONSOLIDATE duplicate lookup only / H for whole test |
| `player_animation_tests.cpp`: `IdleProducesBreathingViewmodelPose`, `WalkAndSprintAdvanceGaitAndLowerWeapon`, `AdsDampsFreeWeaponPose`, `SlideWallRunAndMantleProduceDistinctPoses`, `ReloadFireAndEnergyPlatformDriveUpperBody` | GAME-SPECIFIC | Existing locomotion/weapon/tech presentation and layer selection | KEEP / H |
| Same: `ResetClearsTransientAnimationState`, `DeterministicTransitionsAndNormalizedClipTime` | CONTRACT | Reset and deterministic fixed-tick/normalized-time blending | KEEP / H |
| Same: `JumpFallLandStateAndEvents`, `CrouchDirectionAndTurnLean`, `ConcurrentUpperBodyAdditivesAndMarkers`, `FootstepAndMovementTechMarkers`, `SkeletonPoseAndSocketsAreQueryable` | CONTRACT | Transitions/events, local direction, concurrent layer ordering, phase crossings and semantic bindings | KEEP / H |
| `player_skeletal_animator_tests.cpp`: `CookedCharacterImportsAndAnimates` | INTEGRATION | Real cooked character import, joints/clips/weights, deformation, ADS layer and animated hand socket | KEEP / H; fixture thresholds reviewed at legitimate recook |
| `asset_catalog_tests.cpp`: `CatalogLoadsDevSandboxAssets` | INTEGRATION | Manifest IDs/tags and sandbox preload zone | KEEP / H; radius is game policy |
| `dev_asset_bindings_tests.cpp`: `RequiredDevAssetsBindToMeshCatalog`; `MissingMetadataIsReported` | INTEGRATION; CONTRACT | Required list -> metadata -> GLB -> CPU catalog/handles/counts; missing metadata prevents readiness and reports errors | KEEP / H |
| `render_tuning_tests.cpp`: `RenderTuningLoadsLightingAndCamera`, `RenderTuningClampsUnsafeValues`, `RenderTuningPreservesFallbacksWhenMissing` | CONTRACT | Config propagation/direction normalization, safe bounds and supplied defaults | KEEP / H |
| `player_spawn_tests.cpp`: `SpawnCreatesExpectedPlayerComponents` | INTEGRATION | Live ECS identity/team/transform/health/movement/view and loaded weapon/full magazine | KEEP / H |
| `player_health_tests.cpp`: `BodyDamageAndReset`, `HeadAndLegMultipliers`, `EliminationClampsDamage` | GAME-SPECIFIC | Reset, zone multipliers, health floor/lethal clamp and ignoring damage after death | KEEP / H |

## Weapons and networking

| Source and functions/groups | Category | Protected behavior | Decision / risk |
| --- | --- | --- | --- |
| `weapon_simulation_tests.cpp`: `FireConsumesAmmoAndStartsCooldown`, `CooldownBlocksImmediateSecondShot`, `ReloadCompletesAfterReloadTime`, `DryFireWhenEmpty`, `AdsBlendAndRecoilRecovery`, `MovementSpreadTelemetry` | GAME-SPECIFIC | Ammo/index, RPM gate, timed reload, dry fire, ADS/recoil and movement spread | KEEP / H |
| `weapon_shot_tests.cpp`: `ShotTraceIsDeterministic`, `ShotTraceUsesWeaponRangeAndDamage`, `AdsHasLessSpreadThanHipFire`, `MovementAndAirborneIncreaseSpread`, `RuntimeRecoilOverridesFallback`, `DirectionIsNormalized` | CONTRACT | Seeded trace, range/damage, spread factors, recoil precedence and unit direction | KEEP / H |
| `weapon_metrics_tests.cpp`: `RangeBandsAndDamage`, `ArTtkUsesShotCountAndFireRate`, `SmgCloseTtk` | GAME-SPECIFIC | Damage/head multiplier, shots-to-eliminate and (shots-1)*interval arithmetic | KEEP / H; synthetic metrics, not shipped balance proof |
| `weapon_attachments_tests.cpp`: `PrototypeRegistryBuildsEffectiveWeapon`, `MagazineAndSlotCycling`, `InvalidAttachmentIsIgnored` | GAME-SPECIFIC | Effective stats/capacity/mobility, slot wrap and missing-ID tolerance | KEEP / H |
| `player_command_queue_tests.cpp`: `QueueKeepsCommandsInTickOrder`, `DuplicateTickReplacesLatestCommand`, `OutOfOrderCommandIsRejected`, `CapacityDropsOldestCommand`, `AcknowledgementRemovesProcessedCommands` | CONTRACT | Tick ordering/replacement/stale rejection, capacity and inclusive ACK | KEEP / H |
| `command_messages_tests.cpp`: `CommandPacketRoundTrip`, `CommandAckRoundTrip`, `InvalidPayloadFails` | CONTRACT | Asserted command/device/ACK wire fields and short-payload rejection | KEEP / H |
| `loopback_command_bridge_tests.cpp`: `LoopbackBridgeAcknowledgesPendingCommands` | INTEGRATION | Queue -> wire transport -> server ACK -> client pruning and counters | KEEP / H |
| `prediction_history_tests.cpp`: `PredictionHistoryRecordsAndFindsSamples`, `PredictionHistoryPrunesAcknowledgedTicks`, `PredictionHistoryRejectsStaleWrappedTicks`, `PredictionHistoryComparesAuthoritativeState` | CONTRACT | Sample/history ring, inclusive ACK prune, stale rejection and error thresholds | KEEP / H |
| `snapshot_messages_tests.cpp`: `SnapshotPacketRoundTrip`, `SnapshotPacketRejectsInvalidPayload` | CONTRACT | Asserted snapshot/mask/ACK wire fields and short invalid input | KEEP / H |
| Same: `PlayerSnapshotInterpolatorSamplesRemoteState`; `SnapshotConvertsToAuthoritativePredictionState` | CONTRACT; INTEGRATION | Neighbor ticks/alpha/position, shortest yaw, newer discrete weapon, delay/prune/rollback; reconciliation input mapping | KEEP / H |

## Developer tools, UI and settings

| Source and functions/groups | Category | Protected behavior | Decision / risk |
| --- | --- | --- | --- |
| `debug_target_tests.cpp`: `ForwardShotHitsDebugTarget`, `MissDoesNotDamageTarget`, `TargetCanBeEliminatedAndReset` | GAME-SPECIFIC | Nonmutating trace, hit distance/damage, misses, elimination/reset | KEEP / H; separate implementation from PlayerHealth |
| `dev_target_range_tests.cpp`: `DefaultRangeHasMultipleLanes` | SCAFFOLDING | Current four-lane fixture and active-target accessors | REVIEW, retain / M |
| Same: `ShotSelectsAimedLane`, `LaneEliminatesAndRespawnsIndependently`, `ResetRestoresAllLanes`, `LanePressureBuildsAndDecays` | GAME-SPECIFIC | Lane-specific damage, respawn/reset isolation and pressure budget/decay | KEEP / H; aimed fixture does not test overlapping-nearest ordering |
| `dev_range_session_tests.cpp`: `ScoreAndAccuracy`, `RespawnAndResetTimers`, `PlayerDamageRegenAndRespawn` | GAME-SPECIFIC | Score/streak/accuracy, timer/reset and regen/respawn integration | KEEP / H |
| Same: `TimedDrillLaneTtkAndRecoilScoring`, `DrillTimerCompletesAndResetRestarts`, `DrillVariantsChangeRulesAndPreserveBestScores`, `VariantMultiplierBreakdownIsDeterministic` | GAME-SPECIFIC | Drill scoring/TTK/recoil, terminal timer, per-variant bests and deterministic nonunderflow penalties | KEEP / M-H; active tool |
| `dev_sandbox_tests.cpp`: `SummaryIncludesPlayableTelemetry`; `ClearColorReflectsState` | SCAFFOLDING | Diagnostic text exposes relevant state; firing changes debug feedback | REVIEW, retain / M; L for color |
| `game_menu_tests.cpp`: `DirectDevRangeSelection`, `MenuConfirmAndBack`, `MenuTabsSettingsAndLoadoutMutateRuntimeData`, `PointerNavigationActivatesMenuRows` | GAME-SPECIFIC | Keyboard/pointer routing, loading, settings/loadout changes | KEEP / M-H; TDM screen routing is not a match proof |
| Same: `DebugToggle`, `DebugPageCycle`, `NetworkDebugOverlayIncludesPredictionTelemetry` | SCAFFOLDING | Live diagnostic switch/pages/telemetry consumer | REVIEW, retain / M |
| Same: `DevRangeHudUsesPlayableResponsiveLayout` | GAME-SPECIFIC | Gameplay/drill/pressure labels and 1920x1080 panel placement | KEEP / M; not general visual/responsive proof |
| `ui_canvas_tests.cpp`: `CanvasRecordsAndFlushesCommands`, `ImmediateHelpersAppendToFrame`, `CanvasBackbonePrimitives` | CONTRACT + SCAFFOLDING | Semantic commands, channels, text metrics/fitting; exact expansion counts are implementation-sensitive | KEEP / M; REVIEW counts on backend change |
| `game_settings_tests.cpp`: `SettingsParseAndClamp`, `SettingsAdjustmentsAndLookScale` | CONTRACT | Parse/clamp/mutation and effective ADS/inverted look scales | KEEP / H |
| `user_settings_persistence_tests.cpp`: `SerializeSaveAndLoadUserSettings`, `MissingAndInvalidFilesUseFallback` | INTEGRATION | Settings/attachment disk roundtrip, optional missing file vs malformed-file error | KEEP / H |
| `player_profile_tests.cpp`: `PrototypeAccountStatsAreUsefulForUi`; `StatsHandleZeroDenominators` | SCAFFOLDING; CONTRACT | Placeholder UI profile IDs/values; defined empty ratios | REVIEW, retain / L; KEEP ratios / M |

## Render-scene functions

Source: [`tests/dev_range_render_scene_tests.cpp`](../../tests/dev_range_render_scene_tests.cpp).

| Function suffix | Category | Protected behavior and historical-detail decision |
| --- | --- | --- |
| `BuildsExpectedSubmissions` | INTEGRATION + SCAFFOLDING | KEEP/H camera propagation, world/viewmodel isolation, profiles/shadows/submissions. REVIEW/M exact 34 meshes, +17 boxes, four body primitives and lighting/FOV fixture choices. |
| `MovesWeaponTowardSightlineInAds` | GAME-SPECIFIC | KEEP/H final-submission ADS; rig-unit ADS does not replace the seam. |
| `ViewmodelFollowsCameraAngles` | CONTRACT | KEEP/H exact yaw, negated mesh pitch and full roll are coordinate semantics, not import repairs. |
| `AlignsWeaponMeshToCookedMuzzleSocket` | INTEGRATION | KEEP/H cooked-marker vs fallback path, binding counts/root shift with stable orientation. |
| `KeepsCookedWeaponAxes` | CONTRACT | KEEP/H canonical SMG/sidearm mount yaw/roll and art scale. Synthetic triangles do not prove cooked-GLB geometry orientation. |
| `PlacesShowcaseAssetsInSpawnView` | SCAFFOLDING | REVIEW, retain/M stage visibility/rack/operator scale and fabric/weapon profiles. Renamed from A2 because Project assets are also checked. |
| `HidesLocalWorldBodyWhenCameraRigIsActive` | REGRESSION | KEEP/H mutually exclusive local-world/body and first-person paths; relative count encodes a replacement invariant. |
| `RigsFirstPersonBodyForLookDown` | REGRESSION | KEEP/H no full T-pose body at camera, below-camera arms/weapon and socket-near arms; reveal primitives remain. |
| `DrawsSocketDrivenShotFeedback` | INTEGRATION | KEEP/H muzzle/casing/hit/trace submissions from gameplay feedback. |
| `DrawsLanePressure` | GAME-SPECIFIC + SCAFFOLDING | KEEP/M visible pressure cues; REVIEW exact +19 boxes/two lines tied to other layout. |
| `AcceptsSpeedClearTargetLayout` | INTEGRATION | KEEP/M five-lane variant reaches builder with actor-per-lane/active lane. |
| `CountsMissingMeshHandles` | REGRESSION + SCAFFOLDING | KEEP/H missing accounting, no invalid viewmodel and primitive fallback; REVIEW exact totals at showcase changes. |
| `KeepsSkyboxMeshFallbackWhenSkyPassDisabled` | INTEGRATION | KEEP/M explicitly disabled-sky fallback is active behavior, not a dead historical path. |
| `HandlesMissingInputs` | CONTRACT | KEEP/H no partial scene/camera with missing world/resources. |
| `CanDisableDebugLines` | CONTRACT | KEEP/M respects caller debug-line switch. |
| `DrawsMantleCandidateDebugLines`, `DrawsSweepDebugLines`, `DrawsContactDebugLines` | SCAFFOLDING | REVIEW, retain/M three different live diagnostic branches; exact 3/5/3 counts are not equivalent scenarios. |

## Cleanup applied and retained protection

* Consolidated identical `WeaponRoot` references `weapon` and `hipWeapon` in the
  rig-socket test. Lookup is a pure indexed const-reference return; use `weapon`
  for the existing ADS comparison. Every assertion and frame is retained.
* Removed two explicit zero mount assignments already provided by value/default
  initialization. The tests now consume the same public rig defaults directly.
* Renamed five tests without changing any body/assertion: PistolRoll -> UprightRoll,
  ArTtkTargetsBoLikeBaseline -> ArTtkUsesShotCountAndFireRate,
  ShotSelectsNearestHitLane -> ShotSelectsAimedLane,
  LandingKickRecoversWithoutMovingPhysicsState -> LandingKickStartsAndRecovers,
  PlacesA2AssetsInSpawnView -> PlacesShowcaseAssetsInSpawnView. Names now state
  what the fixtures actually establish. No nearest-order, balance or physics
  immutability coverage was deleted: those old names overstated their assertions.

No whole test is proven safely redundant. Development scenarios remain REVIEW
rather than being deleted just because they are outside the shipping milestone.

## Overlap and ownership that must remain distinct

* ADS spans animation, camera, rig, sockets and final submissions: different seams.
* Damage in DebugTarget and PlayerHealth uses different implementations. Score,
  console telemetry and HUD telemetry have different consumers.
* Shared scene assertions run with different position/angle/movement inputs;
  showcase tests uniquely check placement/fabric profiles. They are not exact
  equivalent fixtures and should not be wholesale consolidated.
* Four pure PhysicsWorld cases are later MOVE candidates, not deletions. Engine
  corner tests cover two-wall sweep hits; Nemisis also covers initially overlapped
  depenetration order. Engine rounded-corner tests require a radial hit; Nemisis
  requires a nearby clear path, avoiding an expanded-AABB false positive. Direct
  vertical sweep/snap/slope tests differ from motor landing integration. Moving
  them requires exact scenario preservation in an engine PR and pin validation.

## Coordinate evidence still missing from these tests

The scene fixture binds synthetic triangles to 35 IDs. Dev-asset binding fixtures
have metadata advertising only `socket_root`. They validate CPU import/accounting
and missing metadata, not per-class required sockets or real weapon orientation.
The muzzle test verifies counters/root displacement, not exact reconstructed
muzzle/grip coordinates under combined rotation/scale. Static scene socket code
uses marker positions and ignores marker forward/up. The real-character test
proves deformation and an animated hand socket, not first-person weapon alignment.

The independent coordinate-contract PR should add focused nested-transform,
marker-direction, reconstructed-muzzle/scale and representative cooked-weapon
checks. No new GLB binary, generic validation platform or screenshot harness is
needed. CPU geometry assertions remain separate from actual image inspection.

The smoke runner is KEEP/INTEGRATION/H: it requires process success, a nonempty
catalog, completed shutdown, no missing-data diagnostics, and an active Vulkan
backend for the hardware cases. None of the 37 smokes/executables was disabled.

Phase 2 geometry selection (main741aeae):
`testFirstPersonArmsExcludeAuthoringHelpers` is CONTRACT/INTEGRATION, KEEP/H:
real asset selection excludes guide/alternate-LOD primitives while retaining
weighted geometry and socket markers. `testFirstPersonArmsHaveOnePresentationOwner`
is REGRESSION/INTEGRATION, KEEP/H: one arm mesh across simultaneous movement cues,
preserved effects, explicit rig diagnostics and missing-mesh fallback. Result181
functions (179 after Phase1 plus2), unchanged registrations. Existing submission
counts intentionally drop proxy boxes when actual arms exist. The reconstructed
grip test explicitly enables diagnostic proxies; it checks target transforms,
not anatomical contact by skinned hands. All original functions remain.

Phase 2B shared-pose extraction adds two CONTRACT/INTEGRATION functions in the
existing scene suite. Actual M4 markers and independently reconstructed render
matrices cover hip, ADS, combined camera/recoil/reload/procedural input, cached
loadout ownership, one arms submission and authored/invalid grip orientation.
All previous functions/assertions remain. Missing-primary/fallback accounting is
preserved as well as geometry. Registrations remain 34 standard CPU cases, 35
with the optional ozz probe; no GPU acceptance follows from these assertions.
