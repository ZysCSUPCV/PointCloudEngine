#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeNode
    {
    public:
        OctreeNode (std::vector<Vertex> vertices, Vector3 center, OctreeNode *parent);
        ~OctreeNode();

        std::vector<OctreeNodeVertex> GetVertices(Vector3 localCameraPosition, float size);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(int level);

    private:
        bool IsLeafNode();

        // Reducing this value heavily decreases the octree node count and therefore also the memory requirements
        // TODO: Should be set individually for each ply model
        const float octreeNodeMinSize = 0.0001f;

        OctreeNode *parent = NULL;
        OctreeNode *children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        OctreeNodeVertex octreeVertex;
    };
}

#endif