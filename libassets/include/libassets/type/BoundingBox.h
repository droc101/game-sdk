//
// Created by droc101 on 8/20/25.
//

#pragma once
#include <array>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/type/ModelVertex.h>
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
        explicit BoundingBox(std::array<float, 3> extents);

        /**
         * Create a new bounding box with the given origin and extents
         * @param origin The origin of the box
         * @param extents The extents of the box
         */
        BoundingBox(std::array<float, 3> origin, std::array<float, 3> extents);

        /**
         * Create a bounding box of the given vertices
         * @param verts The vertices to create a box for
         */
        explicit BoundingBox(const std::vector<ModelVertex> &verts);

        /**
         * Create a bounding box of the given vertices
         * @param verts The vertices to create a box for
         */
        explicit BoundingBox(const std::vector<std::array<float, 3>> &verts);

        /**
         * Create a bounding box from a DataReader
         * @param reader The DataReader to read from
         */
        explicit BoundingBox(DataReader &reader);

        /**
         * Get the 8 corners of the bounding box
         */
        [[nodiscard]] std::array<std::array<float, 3>, 8> GetPoints() const;

        /**
         * Get the 8 corners of the bounding box in a single array, useful for rendering
         */
        [[nodiscard]] std::array<float, 24> GetPointsFlat() const;

        /**
         * Write the bounding box to a DataWriter
         * @param writer The DataWriter to write to
         */
        void Write(DataWriter &writer) const;

        std::array<float, 3> origin{};
        std::array<float, 3> extents = {0.5, 0.5, 0.5};
};
