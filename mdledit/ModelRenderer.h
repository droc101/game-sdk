//
// Created by droc101 on 6/27/25.
//

#ifndef MODELRENDERER_H
#define MODELRENDERER_H

#include <map>
#include <string>
#include <GLES3/gl3.h>
#include <libassets/ModelAsset.h>
#include <glm/glm.hpp>

class ModelRenderer {
    public:
        ModelRenderer() = delete;

        static void Init();

        static void Destroy();

        static void Render();

        static void LoadModel(const char* filepath);

        static void UnloadModel();

        static const ModelAsset* GetModel();

        static void UpdateView(float pitchDeg, float yawDeg, float distance);

        static void UpdateViewRel(float pitchDeg, float yawDeg, float distance);

        static void ResizeWindow(GLsizei w, GLsizei h);

        static int *GetLOD();

        static int *GetSkin();

    private:
        inline static ModelAsset model;

        struct GLModelLod
        {
            GLuint vao;
            GLuint vbo;
            std::vector<GLuint> ebos;
        };

        inline static std::vector<GLModelLod> lods = std::vector<GLModelLod>();
        inline static std::map<std::string, GLuint> texture_buffers = std::map<std::string, GLuint>();

        inline static GLuint program;

        inline static float pitch = 0.0f;
        inline static float yaw = 0.0f;
        inline static float distance = 1.0f;
        inline static glm::mat4 projection;

        inline static int lod = 0;
        inline static int skin = 0;

        inline static GLsizei windowWidth = 800;
        inline static GLsizei windowHeight = 600;
        inline static float windowAspect = 4.0 / 3.0;

        static GLuint CreateShader(const char* filename, GLenum type);

        static GLuint CreateTexture(const char *filename);

        static void UpdateMatrix();
};



#endif //MODELRENDERER_H
