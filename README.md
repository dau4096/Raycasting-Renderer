# Raycasting Renderer
## _Branch: GPU_
___

## Overview
This project aims to recreate a DOOM style renderer using modern OpenGL functionality, within shaders. I have emphisis on adding features that make the project more technically or visually interesting, first and foremost.

The project has full key-rebinding support, options changing (both via `userConfig.xml`), custom stages (using the xml format present in `/stages/`, also allowing for a folder of the same name to be created to store model or texture data specific to this stage) and so on.

Textures must be in the PNG format, with a resolution of `128x128`. There can be a maximum of 64 textures in a stage at any time. Skyboxes are `512x256` textures, and, when `VIEW_VLOOK` is disabled, take up the full `Y` (vertical) space of the screen. `X` is determined on individual fragment direction in the `XY` plane (horizontal), from `-180` to `180` degrees.

`Displacements` can be auto-created by adding a `<model />` node in a stage xml file. This will load an OBJ file in the stage file's assets folder. It is not recommended to add many models, and exceeding 1000-2000 `Displacements` will adversely affect performance. These `Displacements` cannot be collided with, and do render faster than `Walls` or even `Visplanes` so can be used to create distant geometry cheaper.

___
## The CPU-Side
### _Flags_
There are 256 assignable flag index values. These are boolean values stored in a project-wide array, and can affect "Specials". "Specials" are `Walls` or `Visplanes` which can move, interact with the player, or so on. The `IOPtr` value on these objects in the stage XML file correspond to the relevant flag index. `extra` usually relates to distance to move depending on `type`, or amount to hurt player per tick for example.

### _Movement_
The player can;
- Slide by walking/running and then holding the crouch key.
- Double jump via pressing jump again while airborne
- Airstrafe by moving in a non-forward direction while airborne (similar to, but not the same as Quake/Source)

### _Types_
Walls;
- Perfectly vertical, 2D lines.
- Infinitely thin.
- Has constant height.
- Textured from 3D position.
- Expensive to render.

Visplanes;
- Perfectly horizontal, rectangular.
- Infinitely thin.
- Has constant height.
- Textured from 3D position

Displacements;
- 3D triangles.
- No physics collision.
- Cheap to render.
- Textured from barycentric UV.

Sprites;
- Always face camera.
- Always face light sources.
- Cylindrical collision. (or none)
- Textured from width/height values.

Lights;
- Point-sources.
- Constant colour.
- Can be dynamically enabled/disabled via flags.
- Expensive to render.

TextObjects;
- Sprite-like labels.
- Can only use up to 64 Alphanumeric characters per TO, plus a few extra symbols: `-.!?,'/:;&[]()^`
- Always fully lit.
- Very expensive to render, when numerous.

___
## The GPU-Side

### _raycast.comp_
A pre-processing step for `environment.frag`. Walls are vertical slices and so can be computed for every horizontal pixel, written to a buffer, then referenced later. This improves performance notably.


### _environment.frag_
This shader draws the stage from the perspective of the player, using line intersection and 3D rasterisation techniques. `Walls` are perfectly vertical, infinitely thin lines with a given height. `Visplanes` are perfectly horizontal planes that span between two 2D positions. These are textured based on physical position of each fragment to allow textures to line up between `Walls`/`Visplanes`.

The textures all use `GL_NEAREST` sampling, but make use of mipmapping. The calculation for when to use each mip level is based on a minimum distance to the first mip change, the surface's slope and the actual distance. The texturing blends between mip levels providing a smoother transition. Without mipmapping, glancing angles on surfaces creates a shimmering look as differing pixels are sampled when the view moves. This is non-ideal, so mipmapping was added. This has the added benefit of allowing for a texture quality option.

The world is drawn in a pseudo-3D manner. `X, Y` are in perspective, and `Z` (vertical) is orthographic. This mimics the DOOM style of perfectly vertical walls and notable inability to look directly upward/downward.


### _displacements3D.frag_
Rasterises and colours the displacements in 3D space, using `projection.vert` (which uses a similar system to sprite rendering.) This is drawn to a framebuffer including position and normals data.

![**[Image of Displacements before the projection shader]**](images/displacements-preVS.png "[Image of Displacements before the projection shader]")

_Displacements mesh in actual 3D space._

![**[Image of Displacements being warped in the view frustum]**](images/displacements-postVS.png "[Image of Displacements being warped in the view frustum]")

_Displacements after the projection shader, warped inside the view frustum._


### _displacements2D.frag_
This interprets the data from the frambuffer used in `displacements3D.frag` and writes it to the current frame texture given that pixel is closer than that already inside the frame. Also writes normals/position data for lighting if applicable.

![**[Image of Displacements overlaid on scene]**](images/displacements.png "[Image of Displacements overlaid on scene]")

_Displacements overlaid on the rest of the environment._


### _sprites.frag_
This shader simply draws every sprite. They can have transparency, and for lighting purposes are treated as a singular point at their centre with no normal (always facing light source).


### _lighting.comp_
This shader is responsible for the scene's lighting. The aforementioned shaders draw into;

![**[Image of renderedFrame]**](images/colourMap.png "[Image of renderedFrame]")

_An albedo map (`renderedFrame`)_

![**[Image of positionMap]**](images/positionMap.png "[Image of positionMap]")

_A map of positions (`positionMap`)_

![**[Image of normalMap]**](images/normalMap.png "[Image of normalMap]")

_And a map of normals (`normalMap`)_

![**[Image of lightMap]**](images/lightMap.png "[Image of lightMap]")

_To create the final lightmaps._

Using these three textures, it creates a lower resolution overlay of pixel lighting to be used later. This can contain lighting colours, shading, brightness and so on. The calculations utilise every light in the scene, the player's Headlamp (if enabled) and the sun (given as a direction and colour).

![**[Image of completed frame]**](images/finalFrame.png "[Image of completed frame]")

_The final result (produced later by `display.frag`) from the above maps._

The lighting map created is the only `GL_LINEAR` sampled `Image2D` in the entire project, as this allows the shadows to have softer edges.


### _interface.frag_
Responsible for the HUD, `interface.frag` works with `interface.vert` to create an orthographic 3D scene of HUD elements.


### _display.frag_
The only shader to have its own dedicated vertex shader, `display.frag` combines and shows the final frame. Using the albedo map, the lighting map and the UI texture, these are combined to create the final image visible onscreen. The shader also writes to another `Image2D`, as these "post-processing" style effects must appear in screenshots. `display.frag` also applies screen effects such as tinting when the player is hurt.

___
## The XML stage files
### _General_
These files contain the data to be loaded about a file. Textures are referred to by their filename (without the .png extension) and must be 128x128. Textures attempt to load from an internal default textureset (found in `src/textures-env/`) before falling back to the stage's asset folder. The stage's asset folder must be named in all capitals with the prefix `assets-`. For example, the `e1m1.xml` file has `assets-E1M1` containing textures it uses. Models (`.obj`) also must be in this folder. Old attributes are retained for the sake of compatability with older XMLs, or because older methods (like Visplane `start`/`end`) are convenient in certain circumstances. Types are always written as the text representation of the name. For example, `V_TELEPORT` would be written in its entirety, in all capitals. Boolean values accept "true", "false", "t", "f" in any variant of capitalisation.

### _Visplanes_
Visplanes are objects stored under the tag `<environment>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `start` | Attribute (Old) | 2D vector | 2D position to "start" the plane at. Only works when `end` is also present to make a rectangle. |
| `end` | Attribute (Old) | 2D vector | 2D position to "end" the plane at. Only works when `start` is also present to make a rectangle. |
| `vertices` | Child-Node | 2D vector | Node containing further attributes named v0 through v7 (2D vertex positions). See example for formatting. |
| `height` | Attribute | Number | The Z position to place the plane at. Planes are always horizontal so this is consistant accross the VP's surface. |
| `texture` | Attribute | Text | The texture file to use on the plane. |
| `type` | Attribute | Type | Type of plane. See the Types section (Visplane-specific) |
| `flag` | Attribute | Text | The name of the flag to be used (usually by non-`V_NORMAL` types.) |
| `extra` | Attribute | Number | Data used by non-`V_NORMAL` types. See the Types section (Visplane-specific). |
| `useWorldUVX` | Attribute | Boolean | Whether to texture based on physical X (Horizontal) position or accross the surface. (Currently non-functional, defaults to `TRUE`). |
| `useWorldUVY` | Attribute | Boolean | Whether to texture based on physical Y (Horizontal) position or accross the surface. (Currently non-functional, defaults to `TRUE`). |
| `textureScale` | Attribute | 2D vector | Texture scale. higher numbers make the texture larger. |
| `textureOffset` | Attribute | 2D vector | [0-1] range of texture offset. Is applied after texture scale. |
| `exitDirection` | Attribute | Number | Only applies when `type` is `V_TELEPORT`. Specifies player view direction in degrees when exiting this VP. `0.0` is +Y. |

Example;

`<visplane texture="floor" height="0.0" textureScale="2.0 2.0"> <vertices v0="3.0 5.5" v1="6.0 5.5" v2="6.0 6.0" />  </visplane>`

Triangular visplane at height 0, with double texture scale and 3 defined vertices. Taken from `e1m1.xml`.


### _Walls_
Walls are objects stored under the tag `<environment>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `start` | Attribute | 3D vector | 3D position to start the wall at. Z component is Z height of bottom of wall. |
| `end`	| Attribute | 3D vector | 3D position to end the wall at. Z component is Z height of top of wall. |
| `texture` | Attribute | Text | The texture file to use on the wall. |
| `altTexture` | Attribute | Text | The alternative texture file to use on the wall. Currently only used by `W_SWITCH` to show when it's flag is enabled. |
| `type` | Attribute | Type | Type of wall. See the Types section (Wall-specific) |
| `flag` | Attribute | Text | The name of the flag to be used (usually by non-`W_NORMAL` types.) |
| `extra` | Attribute | Number | Data used by non-`W_NORMAL` types. See the Types section (Wall-specific). |
| `useWorldUVX` | Attribute | Boolean | Whether to texture based on physical XY (Horizontal) position or accross the surface. |
| `useWorldUVY` | Attribute | Boolean | Whether to texture based on physical Z (Vertical) position or accross the surface. |
| `textureScale` | Attribute | 2D vector | Texture scale. higher numbers make the texture larger. |
| `textureOffset` | Attribute | 2D vector | [0-1] range of texture offset. Is applied after texture scale. |

Example;

`<wall start="-0.375 11.0 0.0" end="0.375 11.0 2.0" texture="metal2" useWorldUVX="FALSE" textureScale="2.0 4.0" type="W_DOORSWING" extra="2.0" />`

A swinging door which is 2 units tall, with double texture scale X and quadruple Y, which rotates by 2 radians when interacted with. Taken from `dev-quake.xml`.


### _Displacements_
Displacements are objects stored under the tag `<environment>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `aPos` | Attribute (Old) | 3D vector | Vertex 0's position. Only works when `vertices` is not present. |
| `bPos` | Attribute (Old) | 3D vector | Vertex 1's position. Only works when `vertices` is not present. |
| `cPos` | Attribute (Old) | 3D vector | Vertex 2's position. Only works when `vertices` is not present. |
| `vertices` | Child-Node | 3D vector | Node containing further attributes named v0 through v2 (3D vertex positions). See example for formatting. |
| `aUV` | Attribute (Old) | 2D vector | Vertex 0's texture coordinate. Only works when `uv` is not present. |
| `bUV` | Attribute (Old) | 2D vector | Vertex 1's texture coordinate. Only works when `uv` is not present. |
| `cUV` | Attribute (Old) | 2D vector | Vertex 2's texture coordinate. Only works when `uv` is not present. |
| `uv` | Child-Node | 2D vector | Node containing further attributes named uv0 through uv2 (2D texture coordinates). See example for formatting. |
| `texture` | Attribute | Text | The texture file to use on the displacement. |
| `type` | Attribute | Type | Type of displacement. See the Types section (Displacement-specific). Currently unused (defaults to `D_NORMAL`). |
| `flag` | Attribute | Text | The name of the flag to be used (usually by non-`D_NORMAL` types). Currently unused (defaults to _`nullptr`_). |
| `extra` | Attribute | Number | Data used by non-`D_NORMAL` types. See the Types section (Displacement-specific). Currently unused (defaults to `0.0`). |

Example;

`<displacement texture="planks2" textureOffset="0.5 0.5"> <vertices v0="-1.0 0.0 0.0" v1="1.0 0.0 0.0" v2="0.0 0.0 1.0" /> <uv uv0="0.0 0.0" uv1="1.0 0.0" uv2="1.0 1.0" /> </displacement>`

A displacement with 3 vertices (each with texture coordinate), a texture and an offset of half.


### _Sprites_
Sprites are objects stored under the tag `<objects>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `position` | Attribute | 3D vector | Where in 3D space the sprite should be. |
| `width` | Attribute | Number | Used for physics and rendering. Sprites are cylinders like the player, so width affects radius of the cylinder.
| `height` | Attribute | Number | Used for physics and rendering. Sprites are cylinders like the player, so height affects height of the cylinder.
| `texture` | Attribute | Text | The texture file to use on the sprite. Cannot be resized or shifted. |
| `type` | Attribute | Type | Type of sprite. See the Types section (Sprite-specific). Currently unused (defaults to `SPR_DECO`). |
| `collision` | Attribute | Boolean | Defines whether or not the sprite should collide with the player. |

Example;

`<sprite position="5.0 5.0 1.0" width="1.0" height="2.0" texture="piloten" collision="true" />`

A sprite at position (5, 5, 1) with a width and height of 1 and 2 respectively that can be collided with and has a texture. Taken from `dev-quake.xml`.


### _Lights_
Lights are objects stored under the tag `<objects>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `position` | Attribute | 3D vector | Where in 3D space the sprite should be. |
| `colour` | Attribute | 3D vector | What colour light should be emitted. |
| `intensity` | Attribute | Number | Defines the intensity and thus maximum range of the light.
| `flag` | Attribute | Text | The flag to be used to turn the light On/Off. Can be set to `ALWAYS` or `TRUE` to never turn off. |

Example;

`<light position="0.0 -2.0 3.0" colour="1.0 0.0 0.0" intensity="5.0" enabled="TRUE" />`

A light with position (0, -2, 3) which is red and has maximum range of 5 units. It is always enabled. Taken from `test.xml`.


### _TextObjects_
TextObjects are objects stored under the tag `<objects>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `position` | Attribute | 3D vector | Where in 3D space the sprite should be. |
| `text` | Attribute | Text | What text it should show. |
| `scale` | Attribute | Number | The rough scale of the text. 100 is the default. |

Example;

`<textObj text="Project by dau4096 on github." position="0.0 0.0 3.75" scale="100.0" />`

Text object that says "Project by dau4096 on github." with position (0, 0, 3.75) and default scale. Taken from `dev-quake.xml`.


### _Logic Gates_
Logic gates are objects stored under the tag `<logic>` have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `type` | Attribute | Type | Type of gate. See the Types section (Logic-specific). Currently unused (defaults to `G_PASSTHROUGH`). |
| `outFlag` | Attribute | Text | Where to store the output. |
| `inAFlag` | Attribute | Text | Input A's flag. |
| `inBFlag` | Attribute | Text | Input B's flag. |

Example;

`<gate type="G_NOT" inAFlag="stairsFlag" outFlag="notStairsFlag" />`

A logic gate that inverts "stairsFlag" and sets "notStairsFlag" to that value. Taken from `dev-quake.xml`.


### _Models_
Models are macros made of Displacements stored under the tag `<environment>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `position` | Attribute | 3D vector | Where the (0,0,0) point of the model should be. |
| `rotation` | Attribute | 3D vector | Rotation of the model (XYZ axis) in radians. |
| `scale` | Attribute | 3D vector | Scale of the model in XYZ space. |
| `file` | Attribute | Text | Filename of the model to use (without .obj extension) |
| `texture` | Attribute | Text | Texture to use on the model. Uses models' built in UV coordinates (A lack thereof is not corrected.). |

Example;

`<model position="0.0 0.0 0.75" rotation="0.0 0.785 0.0" file="LoPoly_FoodTray" texture="metal" />`

Loads the model "LoPoly_FoodTray.obj" from the stage's assets folder and places it at position (0, 0, 0.75) with a 45 degree rotation and the "metal" texture. Taken from `model-test.xml`.


### _Cuboids_
Cuboids are macros made of Walls and Visplanes stored under the tag `<environment>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `start` | Attribute | 3D vector | 3D position to start the cuboid at. |
| `end`	| Attribute | 3D vector | 3D position to end the cuboid at. |
| `sideTexture` | Attribute | Text | The texture file to use on the side faces. |
| `topTexture` | Attribute | Text | The texture file to use on the top face. |
| `bottomTexture` | Attribute | Text | The texture file to use on the bottom face. |
| `type` | Attribute | Type | Type of VPs/Walls are assigned via Cuboid Type. See the Types section for Cuboids. |
| `extra` | Attribute | Number | Data used by non-`C_NORMAL` types. |
| `flag` | Attribute | Text | The name of the flag to be used. |
| `useWorldUVX` | Attribute | Boolean | Whether to texture based on physical X (Horizontal) position or accross the surface. |
| `useWorldUVY` | Attribute | Boolean | Whether to texture based on physical Y (Horizontal) position or accross the surface. |
| `useWorldUVZ` | Attribute | Boolean | Whether to texture based on physical Z (Vertical) position or accross the surface. |
| `textureScale` | Attribute | 3D vector | Texture scale. higher numbers make the texture larger. |
| `textureOffset` | Attribute | 3D vector | [0-1] range of texture offset. Is applied after texture scale. |

Example;

`<cuboid start="-1.0 11.0 6.0" end="1.0 13.0 7.0" bottomTexture="metal2" sideTexture="metal" topTexture="metal2" />`

A cuboid made from 2 points and 3 seperate textures. Taken from `dev-quake.xml`.


### _Stairs_
Stairs are macros made of Walls and Visplanes. They slope along the longest edge and are stored under the tag `<environment>` and have the following attributes;
| Name | XMLtype | Datatype | Description |
| :---: | :---: | :---: | :--- |
| `start` | Attribute | 3D vector | 3D position to start the stairs at. |
| `end`	| Attribute | 3D vector | 3D position to end the stairs at. |
| `sideTexture` | Attribute | Text | The texture file to use on the side faces. |
| `stepsTexture` | Attribute | Text | The texture file to use on the top face. |
| `static` | Attribute | Boolean | Defines whether or not the stairs should be movable like DOOM93 stair builders. |
| `slowMovement` | Attribute | Boolean | Defines whether or not the stairs should move at the \_FAST or \_SLOW speed. Only used when `static` is `TRUE`. |
| `flag` | Attribute | Text | The name of the flag to be used to trigger the movement, if not `static`. |
| `hasEndWall` | Attribute | Boolean | Whether or not to have the tall end wall under the last step. |
| `hasSideWalls` | Attribute | Boolean | Whether or not to have the walls under the short edge of each step. |
| `hasConnectingWalls` | Attribute | Boolean | Whether or not to have the short walls between the long edges of each step. |
| `useWorldUVX` | Attribute | Boolean | Whether to texture based on physical X (Horizontal) position or accross the surface. |
| `useWorldUVY` | Attribute | Boolean | Whether to texture based on physical Y (Horizontal) position or accross the surface. |
| `useWorldUVZ` | Attribute | Boolean | Whether to texture based on physical Z (Vertical) position or accross the surface. |
| `textureScale` | Attribute | 3D vector | Texture scale. higher numbers make the texture larger. |
| `textureOffset` | Attribute | 3D vector | [0-1] range of texture offset. Is applied after texture scale. |


Example;

`<stairs start="6.0 -8.0 3.0" end="8.0 -1.0 0.0" static="FALSE" flag="notStairsFlag" hasEndWall="FALSE" sideTexture="brick" stepsTexture="planks2" textureScale="2.0 2.0 2.0" />`

Stairs made of 2 points that can move when a flag is triggered. It is textured and scaled. Taken from `dev-quake.xml`.

___
## Types
The types that can be used within XML files and their descriptions.
| Object | TypeName | Description |
| :---: | :---: | :--- |
| Visplane | `V_INVALID` | Used when a visplane is not valid. |
| Visplane | `V_NORMAL` | Used for a regular visplane. |
| Visplane | `V_TRIGGER` | Enables a flag when stepped on. |
| Visplane | `V_MOVEX_FAST` | Moves in the +X direction quickly. |
| Visplane | `V_MOVEX_SLOW` | Moves in the +X direction slowly. |
| Visplane | `V_MOVEY_FAST` | Moves in the +Y direction quickly. |
| Visplane | `V_MOVEY_SLOW` | Moves in the +Y direction slowly. |
| Visplane | `V_MOVEZ_FAST` | Moves in the +Z direction quickly. |
| Visplane | `V_MOVEZ_SLOW` | Moves in the +Z direction slowly. |
| Visplane | `V_HURT` | Hurts the player when stepped on. |
| Visplane | `V_PASSTHROUGH` | Has no physical collision but still renders. |
| Visplane | `V_NODRAW` | Has physical collision but does not render. |
| Visplane | `V_TELEPORT` | Teleports player to partner `V_TELEPORT` when stepped on. |
| Visplane | `V_CONVEY` | Moves player in (attribute) `direction`. |
| | | |
| Wall | `W_INVALID` | Used when a wall is not valid. |
| Wall | `W_NORMAL` | Used for a regular wall. |
| Wall | `W_TRIGGER` | Can be walked through, and enables a flag when that occurs. |
| Wall | `W_MOVED_FAST` | Moves in the direction of (start -> end) quickly. |
| Wall | `W_MOVED_SLOW` | Moves in the direction of (start -> end) slowly. |
| Wall | `W_MOVEN_FAST` | Moves in the direction of the wall's normal quickly. |
| Wall | `W_MOVEN_SLOW` | Moves in the direction of the wall's normal slowly. |
| Wall | `W_MOVEZ_FAST` | Moves in the +Z direction quickly. |
| Wall | `W_MOVEZ_SLOW` | Moves in the +Z direction slowly. |
| Wall | `W_SWITCH` | Enables a flag when interacted with. |
| Wall | `W_PASSTHROUGH` | Has no physical collision but still renders. |
| Wall | `W_NODRAW` | Has physical collision but does not render. |
| Wall | `W_DOORZ` | Moves up when interacted with, stays there for 2s, then moves down. |
| Wall | `W_DOORSWING` | Rotates around its start point when interacted with, stays there for 2s, then returns to initial position. |
| | | |
| Displacement | `D_INVALID` | Used when a displacement is not valid. |
| Displacement | `D_NORMAL` | Used for a regular displacement. |
| | | |
| Sprite | `SPR_INVALID` | Used when a sprite is not valid. |
| Sprite | `SPR_DECO` | Used for a regular sprite. |
| Sprite | `SPR_LIGHT` | Used to mark a light. Acts like `SPR_DECO`. |
| | | |
| LogicGate | `G_INVALID` | Used when a gate is not valid. |
| LogicGate | `G_PASSTHROUGH` | Sets output to input. |
| LogicGate | `G_AND` | Sets output to logical AND of A and B. |
| LogicGate | `G_OR` | Sets output to logical OR of A and B. |
| LogicGate | `G_NOT` | Sets output to logical NOT of A. |
| LogicGate | `G_XOR` | Sets output to logical XOR of A and B. |
| LogicGate | `G_LATCH` | A enables, B disables. Outputs state. |
| LogicGate | `G_PULSE` | Outputs when A goes from `FALSE` to `TRUE`. (1 tick) |
| LogicGate | `G_TOGGLE` | A swaps state. Outputs state. |
| | | |
| Cuboid | `C_INVALID` | Used when a cuboid is not valid. |
| Cuboid | `C_NORMAL` | Used for a regular cuboid. |
| Cuboid | `C_MOVEX_FAST` | Moves in the +X direction quickly. |
| Cuboid | `C_MOVEX_SLOW` | Moves in the +X direction slowly. |
| Cuboid | `C_MOVEY_FAST` | Moves in the +Y direction quickly. |
| Cuboid | `C_MOVEY_SLOW` | Moves in the +Y direction slowly. |
| Cuboid | `C_MOVEZ_FAST` | Moves in the +Z direction quickly. |
| Cuboid | `C_MOVEZ_SLOW` | Moves in the +Z direction slowly. |
| Cuboid | `C_PASSTHROUGH` | Has no physical collision but still renders. |
| Cuboid | `C_NODRAW` | Has physical collision but does not render. |
| | | |