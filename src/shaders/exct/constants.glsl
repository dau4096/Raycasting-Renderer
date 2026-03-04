/* constants.glsl */
//Generic graphics-wide shader constants.


//////////////// Constants ////////////////
#define INF 0xFFFFFF
#define EPSILON 1e-4f
#define EPSILON_ALT 1e-3f
#define MIN_WALL_DIST 0.125f
#define INVALIDdv2 dvec2(INF, INF)
#define INVALIDv2 vec2(INF, INF)
#define INVALIDv3 vec3(INF, INF, INF)
#define INVALIDv4 vec4(INF, INF, INF, INF)
#define NORMAL_UP vec3(0.0f, 0.0f, 1.0f)

//Types
#define T_NONE     0x0
#define T_WALL     0x1
#define T_VISPLANE 0x2
//////////////// Constants ////////////////



//////////////// Config stuff ////////////////
//Mip-mapping;
//Debugging for mipmapping
#define MIPMAP_FORCE_LEVEL_ENABLED false
#define MIPMAP_FORCE_LEVEL_VALUE 0.0f
#define MIPMAP_DEBUG false

//Blending between variable numbers of mipmap levels.
#define MIPMAP_BLEND_ENABLED true
#define MIPMAP_LEVELS 7.0f
#define MIPMAP_MIN_DISTANCE 5.0f


//Lighting;
//Any alpha above this value contributes to lighting FBO components.
#define LIGHTING_THRESHOLD_ALPHA 0.75
//////////////// Config stuff ////////////////