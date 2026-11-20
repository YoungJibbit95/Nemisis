---
name: level-design
description: Shape level flow, pacing, navigation, encounter staging, and spatial teaching.
origin: everything-game-dev-code
category: design
---

# Level Design

## Purpose
Shape level flow, pacing, navigation, encounter staging, and spatial teaching.

## Use When
- building levels, missions, dungeons, arenas, or traversal spaces
- players get lost or overwhelmed
- encounters need better pacing

## Inputs
- core loop
- combat or traversal rules
- narrative beats
- content constraints

## Process
1. define the player path, beats, and teaching moments
2. place pressure, relief, discovery, and decision points intentionally
3. support readability, landmarks, and recovery from mistakes
4. coordinate with UI, narrative, and combat timing
5. review the space with playtest and traversal metrics

## Outputs
- level beat map
- encounter flow notes
- spatial teaching plan
- revision targets

## Quality Bar
- supports the core fantasy and player goals
- defines readable rules, edge cases, and feedback
- creates concrete hooks for tuning, telemetry, and QA

## Common Failure Modes
- adding systems that do not serve the core loop
- shipping vague rules that QA and engineering must guess at
- tuning without instrumentation or hypotheses

## Related Agents
- level-designer
- combat-designer
- narrative-designer

## Related Commands
- level-beat
- playtest-report
- verify

## Notes
- Keep this skill aligned with the relevant rules layer and current project documentation.
- If engine-specific constraints materially change the workflow, hand off to the matching engine skill or engine-specific reviewer.
