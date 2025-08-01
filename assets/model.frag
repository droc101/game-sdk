#version 330

#define DISPLAY_MODE_COLORED 0
#define DISPLAY_MODE_COLORED_SHADED 1
#define DISPLAY_MODE_TEXTURED 2
#define DISPLAY_MODE_TEXTURED_SHADED 3
#define DISPLAY_MODE_UV 4
#define DISPLAY_MODE_NORMAL 5

in vec2 UV;
in vec3 NORMAL;
in vec4 COLOR;

out vec4 PIXEL;

uniform sampler2D ALBEDO_TEXTURE;
uniform vec3 ALBEDO;

uniform int displayMode;

void main()
{
    if (displayMode == DISPLAY_MODE_COLORED || displayMode == DISPLAY_MODE_COLORED_SHADED) {
        PIXEL = vec4(ALBEDO, 1.0) * COLOR;
        if (PIXEL.a < 0.5) discard;
        if (displayMode == DISPLAY_MODE_COLORED_SHADED) {
            vec3 light_dir = normalize(vec3(0.0, 0.0, 1.0));
            float shading = dot(normalize(NORMAL), light_dir);
            shading = (shading + 1.0) * 0.5;
            shading = pow(shading, 1.5);
            shading = mix(0.2, 1.0, shading);
            PIXEL.rgb *= vec3(shading);
        }
    } else if (displayMode == DISPLAY_MODE_TEXTURED || displayMode == DISPLAY_MODE_TEXTURED_SHADED) {
        PIXEL = texture(ALBEDO_TEXTURE, UV) * vec4(ALBEDO, 1.0) * COLOR;;
        if (PIXEL.a < 0.5) discard;
        if (displayMode == DISPLAY_MODE_TEXTURED_SHADED) {
            vec3 light_dir = normalize(vec3(0.0, 0.0, 1.0));
            float shading = dot(normalize(NORMAL), light_dir);
            shading = (shading + 1.0) * 0.5;
            shading = pow(shading, 1.5);
            shading = mix(0.2, 1.0, shading);
            PIXEL.rgb *= vec3(shading);
        }
    } else if (displayMode == DISPLAY_MODE_UV) {
        PIXEL = vec4(UV, 0.0, 1.0);
    } else if (displayMode == DISPLAY_MODE_NORMAL) {
        PIXEL = vec4(NORMAL, 1.0);
    }
    

    if (!gl_FrontFacing) {
        ivec2 pixel = ivec2(gl_FragCoord.xy);
        if ((pixel.x + pixel.y) % 5 != 0) discard;
    }
}
