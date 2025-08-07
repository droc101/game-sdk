//
// Created by droc101 on 7/15/2025.
//

#pragma once

#include <GL/glew.h>

class GLDebug
{
    public:
        static void GL_DebugMessageCallback(GLenum source,
                                            GLenum type,
                                            GLuint id,
                                            GLenum severity,
                                            GLsizei /*length*/,
                                            const GLchar *msg,
                                            const void * /*data*/);
};
