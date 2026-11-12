#version 460

uniform float SPACING;
uniform int PLANE;
uniform mat4 VIEW_MATRIX;

#define PLANE_XZ 0
#define PLANE_XY 1
#define PLANE_YZ 2

#define LEVEL_HALF_SIZE 8192
#define LEVEL_SIZE (LEVEL_HALF_SIZE * 2)

out vec3 LINE_COLOR;

vec4 toWorld(vec2 gridVertex, int PLANE) {
    if (PLANE == PLANE_XZ) {
        return vec4(gridVertex.x, 0.0, gridVertex.y, 1.0);
    } else if (PLANE == PLANE_XY) {
        return vec4(gridVertex.x, gridVertex.y, 0.0, 1.0);
    } else { // PLANE_YZ
        return vec4(0.0, gridVertex.x, gridVertex.y, 1.0);
    }
}

void main() {
    int instancesPerAxis = int(LEVEL_SIZE / SPACING);
    float axisPosition = -LEVEL_HALF_SIZE + (SPACING * float(gl_InstanceID % instancesPerAxis));
    float offAxisSign = gl_VertexID == 0 ? -1 : 1;
    if (gl_InstanceID > instancesPerAxis) {
        gl_Position = VIEW_MATRIX * toWorld(vec2(axisPosition, LEVEL_HALF_SIZE * offAxisSign), PLANE);
    } else {
        gl_Position = VIEW_MATRIX * toWorld(vec2(LEVEL_HALF_SIZE * offAxisSign, axisPosition), PLANE);
    }
    if (axisPosition == floor(axisPosition)) {
        if (int(abs(axisPosition)) % 128 == 0) {
            LINE_COLOR = vec3(0.5,0,0.5);
        } else if (int(abs(axisPosition)) % 64 == 0) {
            LINE_COLOR = vec3(0,0.5,0.5);
        } else {
            LINE_COLOR = vec3(0.2);
        }
    } else {
        LINE_COLOR = vec3(0.1);
    }
}
