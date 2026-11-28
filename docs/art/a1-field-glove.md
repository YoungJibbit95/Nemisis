# A1 field glove and grip poses

This editable field glove has 37 deform joints, eight mesh batches and seven animation roles. The field grip actions articulate the right index and middle fingers and the left thumb and ring finger in their individual bend planes. The palm and finger surface uses the project's own human hand mesh as a fitted leather last. The side panels and sleeves belong to this glove. The source metadata records the geometry reference and its hash.

## Recreate the delivered GLB

Use Blender 5.1.0 and its bundled glTF exporter 5.1.18. The material images are embedded in the editable scene. The export script applies the authored local contact data from the adjacent JSON file. The existing five movement actions and both equipment-contact actions can be inspected in Blender's Action Editor.

```powershell
$source = 'assets/source/blender/characters/chr_a1_fp_arms_01.blend'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ('a1-field-glove-' + [guid]::NewGuid() + '.glb')
blender --factory-startup -b $source --disable-autoexec -t 1 --python-exit-code 1 -P tools/blender/recook_a1_field_glove.py -- $output
Get-FileHash -Algorithm SHA256 $output
```

The portable source and recooked GLB have SHA-256 hashes `4ec1d852a9d9f09032f582bb5e25a3b3d8f31e6c797d5faedb743c932f12aeab` and `8e20b1ad974c5ac9a5715840cf203c22a19e8094565f3bf776973023d426efec`. A Blender skin audit found zero unweighted vertices, zero vertices over four influences and normalized weights. It flagged left/right roll differences on the asymmetric grip rig.

## Measured in the game integration

The close-ups use the actual deformed triangles, materials and UVs recorded at the same reload phase, lit with a fixed inspection camera. The lower row combines this glove with the separate runtime forearm-roll change in the local Nysa-3 integration. That runtime change is required to reproduce the wrist motion. [Matched left and right hands](a1-field-glove/native-reload-surfaces.jpg)

During peak withdrawal, the right wrist's axial rotation relative to the forearm fell from 35.73° to 11.35°, and the left wrist's magnitude fell from 44.09° to 5.49°. Across the entire 270-tick game capture the worst distal roll was 14.33° on the right and 14.20° on the left. The largest frame-to-frame distal changes were 7.93° and 9.06° respectively.

Across ten game poses, right index and middle fingertip vertices previously penetrated the field equipment by up to 3.46 and 2.41 mm. The revised fingertips clear the interiors in the per-component solid check. At the surface, three and six triangle pairs still cross in the ready pose. The proximal right index reaches 1.38 mm into the grip. The left thumb reaches 1.11 mm at return and the left ring fingertip reaches 0.50 mm at seating. The existing sleeve intersection at the right shoulder fixture during sprint remains 15.05 mm. Return to ready is still quick, with a peak 36.64° change in orientation of the entire left forearm across one tick.

## Paired runtime source

[Game source delta for both wrist rigs](a1-field-glove/integrated-hand-rigs.patch). It applies to the integrated game source checkpoint `24859e89411ff88bcfafa4aa2f5e0a6fd8433abe`. The playable checkpoint including these changes is `903c8e80c1b3ae40170169a5458b101acd17dc08`, using the engine source checkpoint `43424c114f455a9610f84817a19a0eb904872fd5`. Both game and engine checkpoint branches are preserved in the development workspace. The patch was checked against the exact base tree with Git. The public main line needs its upstream runtime and host dependencies integrated before this source delta can run there.

The A1 forearm absorbs axial rotation continuously when the pose crosses a half turn. On the Project M4 the two weighted forearm segments share roll, the wrist keeps the palm aligned, and the fingers distribute curl over successive joints. The solver measures the remaining palm error in the actual affinely transformed rig. The magazine exchange follows the sampled chest with the same final attachment for rendering and hand placement. The separate regression exercises nonlinear wrist orientation, flexion, fingertip displacement and the existing contact thresholds.

[Native M4 exchange after the rig correction](a1-field-glove/m4-exchange.jpg). Across hip, stride, reload and ADS, maximum axial wrist roll fell from 179.91° to 10.29°. The 386-tick production input sequence remained within 0.996 mm of the intended hand contacts. Right wrist flexion still reached 55.7° in hip carry. The left sleeve showed a dark fold and the hip readability check remains open.
