#version 330

uniform float spacing;
uniform int plane;
uniform mat4 matrix;

#define PLANE_XZ 0
#define PLANE_XY 1
#define PLANE_YZ 2

#define LEVEL_HALF_SIZE 512
#define LEVEL_SIZE (LEVEL_HALF_SIZE * 2)

out vec3 lineColor;

vec4 toWorld(vec2 gridVertex, int plane) {
    if (plane == PLANE_XZ) {
        return vec4(gridVertex.x, 0.0, gridVertex.y, 1.0);
    } else if (plane == PLANE_XY) {
        return vec4(gridVertex.x, gridVertex.y, 0.0, 1.0);
    } else { // PLANE_YZ
        return vec4(0.0, gridVertex.x, gridVertex.y, 1.0);
    }
}

void main() {
    int instancesPerAxis = int(LEVEL_SIZE / spacing);
    float axisPosition = -LEVEL_HALF_SIZE + (spacing * float(gl_InstanceID % instancesPerAxis));
    float offAxisSign = gl_VertexID == 0 ? -1 : 1;
    if (gl_InstanceID > instancesPerAxis) {
        gl_Position = matrix * toWorld(vec2(axisPosition, LEVEL_HALF_SIZE * offAxisSign), plane);
    } else {
        gl_Position = matrix * toWorld(vec2(LEVEL_HALF_SIZE * offAxisSign, axisPosition), plane);
    }
    if (axisPosition == floor(axisPosition)) {
        if (int(abs(axisPosition)) % 16 == 0) {
            lineColor = vec3(0.5,0,0.5);
        } else if (int(abs(axisPosition)) % 8 == 0) {
            lineColor = vec3(0,0.5,0.5);
        } else {
            lineColor = vec3(0.2);
        }
    } else {
        lineColor = vec3(0.1);
    }
}
