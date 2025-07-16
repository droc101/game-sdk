//
// Created by droc101 on 7/15/2025.
//

#ifndef GLDEBUG_H
#define GLDEBUG_H

#include <GL/glew.h>

class GLDebug {

    public:
        static void GL_DebugMessageCallback(GLenum source,
                             GLenum type,
                             GLuint id,
                             GLenum severity,
                             GLsizei /*length*/,
                             const GLchar *msg,
                             const void * /*data*/);

};



#endif //GLDEBUG_H
