//
// Created by droc101 on 7/15/2025.
//

#include "GLDebug.h"
#include <cstdio>
#include <GL/glew.h>
#include <string>

// #define BREAK_ON_ERROR

void GLDebug::GL_DebugMessageCallback(const GLenum source,
                                      const GLenum type,
                                      const GLuint id,
                                      const GLenum severity,
                                      GLsizei /*length*/,
                                      const GLchar *msg,
                                      const void * /*data*/)
{
    std::string sourceString;
    std::string typeString;
    std::string severityString;

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return; // shut up
    }

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            sourceString = "API";
            break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            sourceString = "WINDOW SYSTEM";
            break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            sourceString = "SHADER COMPILER";
            break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
            sourceString = "THIRD PARTY";
            break;

        case GL_DEBUG_SOURCE_APPLICATION:
            sourceString = "APPLICATION";
            break;

        case GL_DEBUG_SOURCE_OTHER:
        default:
            sourceString = "UNKNOWN";
            break;
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
            typeString = "ERROR";
            break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeString = "DEPRECATED BEHAVIOR";
            break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeString = "UNDEFINED BEHAVIOR";
            break;

        case GL_DEBUG_TYPE_PORTABILITY:
            typeString = "PORTABILITY";
            break;

        case GL_DEBUG_TYPE_PERFORMANCE:
            typeString = "PERFORMANCE";
            break;

        case GL_DEBUG_TYPE_OTHER:
            typeString = "OTHER";
            break;

        case GL_DEBUG_TYPE_MARKER:
            typeString = "MARKER";
            break;

        default:
            typeString = "UNKNOWN";
            break;
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            severityString = "HIGH";
            break;

        case GL_DEBUG_SEVERITY_MEDIUM:
            severityString = "MEDIUM";
            break;

        case GL_DEBUG_SEVERITY_LOW:
            severityString = "LOW";
            break;

        // ReSharper disable once CppDFAUnreachableCode
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severityString = "NOTIFICATION";
            break;

        default:
            severityString = "UNKNOWN";
            break;
    }

    printf("%d: %s of %s severity, raised from %s: %s\n",
           id,
           typeString.c_str(),
           severityString.c_str(),
           sourceString.c_str(),
           msg);
    fflush(stdout);

#ifdef BREAK_ON_ERROR
    // If you hit this "breakpoint", an OpenGL error has been printed to the console,
    // and the corresponding GL call should be on the call stack.
    asm("nop"); // you can also breakpoint this
#ifdef WIN32
    __debugbreak();
#else
    raise(SIGABRT);
#endif
#endif
}
