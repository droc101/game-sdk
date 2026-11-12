#version 460

in vec3 POSITION;
in vec3 NORMAL;

layout (location = 0) out vec4 OBJECT_BUFFER;
layout (location = 1) out vec4 POSITION_BUFFER;
layout (location = 2) out vec4 NORMAL_BUFFER;

uniform uint OBJECT_TYPE;
uniform uint OBJECT_INDEX;
uniform uint OBJECT_SUB_INDEX;

void main()
{
    OBJECT_BUFFER.xyz = vec3(
        uintBitsToFloat(OBJECT_TYPE),
        uintBitsToFloat(OBJECT_INDEX),
        uintBitsToFloat(OBJECT_SUB_INDEX)
    );
    POSITION_BUFFER.xyz = POSITION;
    NORMAL_BUFFER.xyz = NORMAL;
}
