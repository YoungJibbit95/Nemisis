# Private match and live renderer inspection

The integrated Nysa test started a 2v2 match from the main menu. One local player shared the match with three independently posed bots. The host recorded 38 bot hits through tick 960. Escape returned to the main menu with zero active sessions and zero remote mesh handles.

| World normals and host hit volumes | Scene settings restored |
| --- | --- |
| ![Nysa showing the mesh normal shader, active console controls and the host hit volumes on a remote character](console.jpg) | ![The Nysa scene after resetting material, lighting and shadow controls](restored.jpg) |

F2 opens the console and releases the pointer. Arrow keys and Enter also operate the controls. Live hit outlines use the host's four floor-relative damage volumes and standing or crouched pose. The head, torso and two legs are distinct. Practice-range targets use their actual collision shapes. For this sample the test changed the mesh shader, disabled material textures, directional shadows and local lights, enabled GPU pass timers, then restored scene settings. The game continued simulating throughout the sequence.

The Vulkan capture sampled 92 images at 1920 × 1080 on an RTX 3070 Ti. The private desktop audit checked the input desktop 196 times and observed zero foreground violations. The isolated game executable SHA-256 was `1d17f1a2d0335465a98587296ad1715b9ba8002f9603534806bd81d58e496ffc`. Its local source pair includes changes beyond game base `e1d8801bb249bacb3afd0c8377d1bc08f4be481c` and engine base `fbbbfb3a18e945c28c2dc0bc7c5a8d8f8d541f56`. The images document that paired build. Individual release PRs qualify their own exact game, engine and map.

In a separate repeatable 45-second CPU exercise, three bots covered 132–203 m on Nysa and 164–227 m on Helion. The local player respawned and was reacquired on both maps. The combined four-player simulation took 0.184 ms per tick at p99 on Nysa and 0.073 ms on Helion in one isolated run. These numbers measure simulation cost. The private test does not establish a 165 Hz display result.
