# Outer Facility

`createOuterFacilityWorld()` and `makeOuterFacilityTargets()` provide a second,
independent playable training map. The player starts at (0, 0, -26), facing +Z.
The 70 x 70 m bounded site has an open central courtyard, a west operations
building and an east shelter. Each building has doors through both end walls;
low courtyard cover and interior consoles provide alternate routes and cover.
A cyan stripe marks the west exterior wallrun face. Eight 0.4 m steps on the
courtyard side ascend to a clear west-roof landing at 3.2 m.
The three targets are located in the court, west exit and east entry. Their
facility-only hit zones approximate both legs, torso and head of the floor-rooted
0.92-scale mannequin; they share one damage value and the nearest zone competes
with the nearest cover. Dev Range targets retain their original sphere trace.

The world primitives are the source of movement and shot collision, and the
same boxes are rendered. Four decorative grove instances each contain six trees;
24 separate collision boxes match their trunks. When the grove resource is valid,
these collision boxes are not separately drawn; if it is missing they are shown
as solid fallback trunks. Four existing low foliage clusters add undergrowth.
Vegetation is kept at the outer edges and at the sides of the spawn, away from
building entrances and the central sightline.

The textured trim GLB embeds the existing `movement_accent_basecolor.png`,
providing repeated material coverage for both sides of building walls and five
walkable route strips. It requires the existing material texture path (for the
current launcher, `--material-textures`); otherwise the renderer falls back to
its ordinary untextured material. Both new meshes are generated from versioned
Python sources and use one meter per unit, Y up. No runtime `.blend` is loaded.

## Content budget and validation

- 35 visible world boxes, plus 24 non-rendered matching tree colliders; three
  target boxes and target meshes are separate. One batched trim mesh has one
  material primitive and 58 triangles. Four grove instances each submit two
  material primitives (216 triangles per instance); four undergrowth instances
  each submit one primitive (168 triangles per instance). Trees and undergrowth
  do not submit dynamic or contact shadows. No new light or shadow map is added.
- Existing procedural sky, directional sun and ambient/fill policy are reused.
  This small fixed map has no authored LOD or occlusion culling. The scene and
  foliage budget is a design cap, not a measured GPU-time guarantee.
- CPU checks exercise the real KCC adapter through both building doorways,
  the spawn and interior routes, a continuous swept stair-to-roof ascent, the
  marked wallrun contact, all tree trunks, limb and head hit zones, visible cover, target line of
  sight, packaged material/texture data, scene selection and resource fallbacks.
  Vulkan appearance, textured draw counts, timing and a complete playthrough
  require separate in-game validation on the target hardware.
