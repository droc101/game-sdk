//
// Created by droc101 on 8/20/25.
//

#pragma once

#include <array>
#include <concepts>
#include <cstdio>
#include <libassets/type/ModelVertex.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <limits>
#include <vector>

/**
 * This is basically an AABB, but in-engine it is not axis-locked, so it isn't called AABB here to avoid confusion.
 */
class BoundingBox
{
    public:
        /**
         * Creates a default 1x1x1 bbox at the origin
         */
        BoundingBox() = default;

        /**
         * Create a new bounding box at the origin (0,0,0) with the given extents
         * @param extents The extents (half-size) of the bounding box
         */
        explicit BoundingBox(glm::vec3 extents);

        /**
         * Create a new bounding box with the given origin and extents
         * @param origin The origin of the box
         * @param extents The extents of the box
         */
        BoundingBox(glm::vec3 origin, glm::vec3 extents);

        /**
         * Create a bounding box of the given vertices
         * @param verts The vertices to create a box for
         */
        template<typename T> requires(std::same_as<T, ModelVertex> || std::same_as<T, glm::vec3>)
        explicit BoundingBox(const std::vector<T> &verts)
        {
            if (verts.empty())
            {
                printf("WARN: Tried to create AABB with 0 points!");
                return;
            }
            glm::vec3 minPoint = {
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
            };
            glm::vec3 maxPoint = {
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
            };

            for (const T &vert: verts)
            {
                glm::vec3 point;
                if constexpr (std::same_as<T, ModelVertex>)
                {
                    point = vert.position;
                } else
                {
                    point = vert;
                }

                if (point.x < minPoint.x)
                {
                    minPoint.x = point.x;
                }
                if (point.x > maxPoint.x)
                {
                    maxPoint.x = point.x;
                }

                if (point.y < minPoint.y)
                {
                    minPoint.y = point.y;
                }
                if (point.y > maxPoint.y)
                {
                    maxPoint.y = point.y;
                }

                if (point.z < minPoint.z)
                {
                    minPoint.z = point.z;
                }
                if (point.z > maxPoint.z)
                {
                    maxPoint.z = point.z;
                }
            }

            origin = (minPoint + maxPoint) * 0.5f;
            extents = (maxPoint - minPoint) * 0.5f;
        }

        /**
         * Create a bounding box from a DataReader
         * @param reader The DataReader to read from
         */
        explicit BoundingBox(DataReader &reader);

        /**
         * Get the 8 corners of the bounding box
         */
        [[nodiscard]] std::array<glm::vec3, 8> GetPoints() const;

        /**
         * Get the 8 corners of the bounding box in a single array, useful for rendering
         */
        [[nodiscard]] std::array<float, 24> GetPointsFlat() const;

        /**
         * Write the bounding box to a DataWriter
         * @param writer The DataWriter to write to
         */
        void Write(DataWriter &writer) const;

        glm::vec3 origin{};
        glm::vec3 extents = {0.5, 0.5, 0.5};
};
