#version 330

out mediump vec4 COLOR;

uniform vec4 lineColor;

void main()
{
    COLOR = lineColor;
    float fade_alpha = COLOR.a;
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    float fade = clamp(fade_alpha, 0.0, 1.0);
    if (fade < 0.001 || fade < fract(magic.z * fract(dot(gl_FragCoord.xy, magic.xy)))) {
        discard;
    }
    COLOR.a = 1.0;
}
