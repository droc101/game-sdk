#version 460

in vec2 UV;
in vec3 NORMAL;
in vec4 COLOR;

out vec4 PIXEL;

uniform sampler2D TEXTURE;
uniform vec4 MATERIAL_COLOR;
uniform vec4 MOD_COLOR;
uniform bool SHADED;

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
    PIXEL = texture(TEXTURE, UV) * MATERIAL_COLOR * MOD_COLOR * COLOR;
    if (check_alpha_discard(PIXEL.a)) discard;
    PIXEL.a = 1.0;
    if (SHADED) {
        float shading = calculate_shading();
        PIXEL.rgb *= vec3(shading);
    }
}
