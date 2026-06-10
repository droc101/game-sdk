//
// Created by droc101 on 8/30/25.
//

#pragma once

#include <assimp/mesh.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class ConvexHull
{
    public:
        /**
         * Create a ConvexHull from an OBJ file
         * @param objPath The path to the OBJ file
         * @param status Error code output
         */
        ConvexHull(const std::string &objPath, Error::ErrorCode &status);
        /**
         * Read a ConvexHull from a DataReader
         */
        explicit ConvexHull(DataReader &reader);
        /**
         * Create a ConvexHull from an Assimp aiMesh
         */
        explicit ConvexHull(const aiMesh *mesh);

        /**
         * Write this ConvexHull to a DataWriter
         */
        void Write(DataWriter &writer) const;

        /**
         * Get the points in this ConvexHull
         */
        std::vector<glm::vec3> &GetPoints();

        /**
         * Get a flat array of points for rendering
         */
        [[nodiscard]] std::vector<float> GetPointsForRender() const;

        /**
         * Import multiple ConvexHulls from a single OBJ file
         * @param path The OBJ file to import from
         * @param output Where to store the ConvexHulls=
         */
        [[nodiscard]] static Error::ErrorCode ImportMultiple(const std::string &path, std::vector<ConvexHull> &output);

    private:
        glm::vec3 offset{};
        std::vector<glm::vec3> points{};

        void CalculateOffset();
};
