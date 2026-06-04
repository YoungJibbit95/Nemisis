# 07 Utility and Gamemodes

## Utility Goal

Utility is sandbox-oriented, not hero-kit-oriented. It should give players tools that interact with movement, space, and timing without making the first game a hero shooter.

## Utility Types

Early candidates:

- Frag-style explosive.
- Smoke or vision blocker.
- Flash or disorient effect.
- Stim or mobility boost.
- Deployable test object.
- Jump/mobility prototype.

The plan allows experimental sandbox utility, but v1 balance should focus on a small shippable subset.

## Utility Data

Utility definitions include:

- Cooldown.
- Charges.
- Throw/fire behavior.
- Projectile speed.
- Fuse time.
- Radius.
- Effect duration.
- Team interaction.
- Replication policy.

## Game Modes

### TDM

Rules:

- Two teams.
- Score by kills.
- Respawn enabled.
- Score limit or time limit.
- Spawn safety checks.

### Control

Rules:

- Objective zones.
- Capture progress.
- Contest rules.
- Team score over time.
- Respawn enabled.
- Match phases.

## Match Lifecycle

1. Lobby/direct connect.
2. Load map.
3. Warmup.
4. Countdown.
5. Active match.
6. End state.
7. Scoreboard.
8. Restart or return to menu.

## Spawn System

Spawn scoring inputs:

- Distance from enemies.
- Line of sight danger.
- Objective relevance.
- Recent deaths.
- Team clustering.
- Map-defined spawn groups.

M1 only needs placeholders; real spawn scoring begins with modes.

## Acceptance

Game modes are acceptable when:

- TDM can complete a full match.
- Control objective scoring is correct.
- Spawn points are not obviously unsafe.
- Utility events replicate.
- Match state is visible in UI.







