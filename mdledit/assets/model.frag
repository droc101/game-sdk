#version 320 es

precision mediump float;

layout (location = 4) in vec2 UV;
layout (location = 5) in vec3 NORMAL;

out mediump vec4 COLOR;

uniform sampler2D ALBEDO_TEXTURE;
uniform vec3 ALBEDO;

void main()
{
    COLOR = texture(ALBEDO_TEXTURE, UV) * vec4(ALBEDO, 1.0);
    if (COLOR.a < 0.5) discard;
    vec3 light_dir = normalize(vec3(0.2, 0.0, 0.8));
    float shading = dot(NORMAL, light_dir);
    shading = shading == 1.0 ? 1.0 : 1.0 - pow(2.0, -5.0 * shading);
    shading = max(0.6, shading);
    COLOR.rgb *= vec3(shading);
}
