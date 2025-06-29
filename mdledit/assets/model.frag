#version 320 es

#define DISPLAY_MODE_TEXTURED 0
#define DISPLAY_MODE_SHADED 1
#define DISPLAY_MODE_UV 2
#define DISPLAY_MODE_NORMAL 3

precision mediump float;

layout (location = 4) in vec2 UV;
layout (location = 5) in vec3 NORMAL;

out mediump vec4 COLOR;

uniform sampler2D ALBEDO_TEXTURE;
uniform vec3 ALBEDO;

uniform int displayMode;

void main()
{
    if (displayMode == DISPLAY_MODE_TEXTURED || displayMode == DISPLAY_MODE_SHADED) {
        COLOR = texture(ALBEDO_TEXTURE, UV) * vec4(ALBEDO, 1.0);
        if (COLOR.a < 0.5) discard;
        if (displayMode == DISPLAY_MODE_SHADED) {
            vec3 light_dir = normalize(vec3(0.2, 0.0, 0.8));
            float shading = dot(NORMAL, light_dir);
            shading = shading == 1.0 ? 1.0 : 1.0 - pow(2.0, -5.0 * shading);
            shading = max(0.6, shading);
            COLOR.rgb *= vec3(shading);
        }
    } else if (displayMode == DISPLAY_MODE_UV) {
        COLOR = vec4(UV, 0.0, 1.0);
    } else if (displayMode == DISPLAY_MODE_NORMAL) {
        COLOR = vec4(NORMAL, 1.0);
    }
    

    if (!gl_FrontFacing) {
        ivec2 pixel = ivec2(gl_FragCoord.xy);
        if ((pixel.x + pixel.y) % 5 != 0) discard;
    }
}
