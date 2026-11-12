#version 460

#define DISPLAY_MODE_COLORED 0
#define DISPLAY_MODE_COLORED_SHADED 1
#define DISPLAY_MODE_TEXTURED 2
#define DISPLAY_MODE_TEXTURED_SHADED 3
#define DISPLAY_MODE_UV 4
#define DISPLAY_MODE_NORMAL 5

in vec2 UV;
in vec3 NORMAL;
in vec4 COLOR;

out vec4 FRAG_COLOR;

uniform sampler2D ALBEDO_TEXTURE;
uniform vec4 ALBEDO;

uniform int DISPLAY_MODE;

float calculate_shading() {
    vec3 light_dir = normalize(vec3(0.0, -1.0, 1.0));
    float shading = 1 - pow(2, 10 * dot(normalize(NORMAL), light_dir));
    shading = max(0.6, shading);
    return shading;
}

bool check_alpha_discard(float fade_alpha) {
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    float fade = clamp(fade_alpha, 0.0, 1.0);
    return fade < 0.001 || fade < fract(magic.z * fract(dot(gl_FragCoord.xy, magic.xy)));
}

void main()
{
    if (DISPLAY_MODE == DISPLAY_MODE_COLORED || DISPLAY_MODE == DISPLAY_MODE_COLORED_SHADED) {
        FRAG_COLOR = ALBEDO * COLOR;
        if (check_alpha_discard(FRAG_COLOR.a)) discard;
        FRAG_COLOR.a = 1.0;
        if (DISPLAY_MODE == DISPLAY_MODE_COLORED_SHADED) {
            float shading = calculate_shading();
            FRAG_COLOR.rgb *= vec3(shading);
        }
    } else if (DISPLAY_MODE == DISPLAY_MODE_TEXTURED || DISPLAY_MODE == DISPLAY_MODE_TEXTURED_SHADED) {
        FRAG_COLOR = texture(ALBEDO_TEXTURE, UV) * ALBEDO * COLOR;
        if (check_alpha_discard(FRAG_COLOR.a)) discard;
        FRAG_COLOR.a = 1.0;
        if (DISPLAY_MODE == DISPLAY_MODE_TEXTURED_SHADED) {
            float shading = calculate_shading();
            FRAG_COLOR.rgb *= vec3(shading);
        }
    } else if (DISPLAY_MODE == DISPLAY_MODE_UV) {
        FRAG_COLOR = vec4(UV, 0.0, 1.0);
    } else if (DISPLAY_MODE == DISPLAY_MODE_NORMAL) {
        FRAG_COLOR = vec4(NORMAL, 1.0);
    }
    

    if (!gl_FrontFacing) {
        ivec2 pixel = ivec2(gl_FragCoord.xy);
        if ((pixel.x + pixel.y) % 5 != 0) discard;
    }
}
