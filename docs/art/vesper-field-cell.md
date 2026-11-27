# Vesper field cell

The Vesper is an original fictional field prop for Nemisis. Its editable Blender scene contains 107 mesh objects. Twelve form the amber cell assembly under the animated exchange control. Three packed images provide the material atlas. The exported GLB uses two rigid batches, each with a near, medium and far index stream.

| Mesh | Near triangles | Medium | Far |
| --- | ---: | ---: | ---: |
| VesperReceiver | 8,104 | 4,052 | 2,754 |
| VesperCell | 1,332 | 770 | 600 |

The source actions drive the cell's translation and rotation and the left index and little finger controls. Each exported control has 101 normalized samples. The cell starts and ends in its seat. Three sample rays through the opening intersect the seated cell and reach the receiver after withdrawal.

## Recook

Use Blender 5.1.0 with its bundled glTF exporter 5.1.18. The Blender command reads the packed scene and checks both index recipes against the exact mesh streams. The independent validator requires Python 3 and NumPy. Run from the repository root with Blender on the command path.

```powershell
$source = 'assets/source/blender/weapons/wpn_a1_vesper_field_01.blend'
$out = Join-Path ([System.IO.Path]::GetTempPath()) ('vesper-' + [guid]::NewGuid())
blender --factory-startup -b -t 1 --python-exit-code 1 -P tools/blender/cook_vesper_cell.py -- --source-blend $source --output-root $out
$glb = Join-Path $out 'assets/generated/studio/wpn_a1_vesper_field_01.glb'
python -B tools/assets/check_vesper_cell.py $glb
```

The recorded SHA-256 of the source is `a40921e09bcc2e4b0d0890da02dcb341e720e21b021b6c1f92686d39f51c5595`. The cooked GLB is `b19c2de40e6d3bcaa1c05fbad7c016cd5d5899201890d3f107d677a3b0a9f9b7`. Geometry edits that alter the near mesh require new reviewed index recipes. Pass `--generate-with` and a local meshoptimizer 1.1 shared library to the cook to regenerate them in the output directory.

## Native camera evidence

The four views and 270-frame recording come from a local integration of the actual Nysa-3 game camera, V8 gloves, and the native renderer that submits the cell independently. This package supplies the asset and cook. To play that sequence, the target game build must also integrate those runtime changes. The distinction matters when opening this standalone asset revision. [Four camera states](vesper-field-cell/four-states.jpg) · [1080p60 handling, 4.5 seconds](vesper-field-cell/handling.mp4)

The native sequence retained both submissions through 270 ticks. The cell moved for 48 ticks and reached 164.8 mm of origin travel. Holstering during entry, withdrawal and return restored the seated position in all three scripted runs. The local game executable had SHA-256 `012ecc6202f3fca1fa0b33f4865e7a6e27171e5a4425e9685c19831baf8760a0`.

The left ring finger's middle segment cleared the former cell-corner intersection across ten sampled poses. At full withdrawal the largest measured left fingertip gap was 2.15 mm at the middle finger. Near seating, the ring tip still entered the surface by roughly 0.73 mm. The right index and middle tips entered the control backing and palm swell by up to 3.46 mm and 2.41 mm. The lower-right receiver remains prominent in the ready frame. These measurements describe the sampled poses in the recorded integration.
