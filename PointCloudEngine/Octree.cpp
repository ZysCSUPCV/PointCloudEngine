#include "Octree.h"

PointCloudEngine::Octree::Octree(std::vector<PointCloudVertex> vertices, Vector3 center)
{
    // The octree is generated by fitting the vertices into a cube at the center position
    // Then this cube is splitted into 8 smaller child cubes along the center
    // For each child cube the octree generation is repeated
    size_t size = vertices.size();

    if (size == 0)
    {
        ErrorMessage(L"Cannot create Octree node from empty vertices!", L"CreateNode", __FILEW__, __LINE__);
    }

    // Initialize average color
    double averageRed = 0;
    double averageGreen = 0;
    double averageBlue = 0;
    double averageAlpha = 0;
    double factor = size;

    // Initialize extends of the cube (the maximal distance from the center in each axis)
    Vector3 extends = Vector3::Zero;

    // TODO: Normal

    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        PointCloudVertex v = *it;

        extends.x = max(extends.x, abs(center.x - v.position.x));
        extends.y = max(extends.y, abs(center.y - v.position.y));
        extends.z = max(extends.z, abs(center.z - v.position.z));

        averageRed += v.red / factor;
        averageGreen += v.green / factor;
        averageBlue += v.blue / factor;
        averageAlpha += v.alpha / factor;
    }

    // Save the bounding cube properties
    // Only the root node could compute its center with minPosition + 0.5f * (maxPosition - minPosition) to improve spatial fit
    // But usually this is not an issue because the vertices should be oriented around the origin in object space anyways
    octreeVertex.position = center;
    octreeVertex.size = max(max(extends.x, extends.y), extends.z);

    // Save average color
    octreeVertex.red = round(averageRed);
    octreeVertex.green = round(averageGreen);
    octreeVertex.blue = round(averageBlue);
    octreeVertex.alpha = round(averageAlpha);

    // Split and create children vertices
    std::vector<PointCloudVertex> childVertices[8];

    // Fit each vertex into its corresponding child cube
    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        PointCloudVertex v = *it;

        if (v.position.x > center.x)
        {
            if (v.position.y > center.y)
            {
                if (v.position.z > center.z)
                {
                    childVertices[0].push_back(v);
                }
                else
                {
                    childVertices[1].push_back(v);
                }
            }
            else
            {
                if (v.position.z > center.z)
                {
                    childVertices[2].push_back(v);
                }
                else
                {
                    childVertices[3].push_back(v);
                }
            }
        }
        else
        {
            if (v.position.y > center.y)
            {
                if (v.position.z > center.z)
                {
                    childVertices[4].push_back(v);
                }
                else
                {
                    childVertices[5].push_back(v);
                }
            }
            else
            {
                if (v.position.z > center.z)
                {
                    childVertices[6].push_back(v);
                }
                else
                {
                    childVertices[7].push_back(v);
                }
            }
        }
    }

    // Assign the centers for each child cube
    float childExtend = 0.5f * octreeVertex.size;

    Vector3 childCenters[8] =
    {
        center + Vector3(childExtend, childExtend, childExtend),
        center + Vector3(childExtend, childExtend, -childExtend),
        center + Vector3(childExtend, -childExtend, childExtend),
        center + Vector3(childExtend, -childExtend, -childExtend),
        center + Vector3(-childExtend, childExtend, childExtend),
        center + Vector3(-childExtend, childExtend, -childExtend),
        center + Vector3(-childExtend, -childExtend, childExtend),
        center + Vector3(-childExtend, -childExtend, -childExtend)
    };

    // Only subdivide further if the size is above the minimum size
    if (octreeVertex.size > octreeVertexMinSize)
    {
        for (int i = 0; i < 8; i++)
        {
            if (childVertices[i].size() > 0)
            {
                children[i] = new Octree(childVertices[i], childCenters[i]);
                children[i]->parent = this;
            }
        }
    }
    else
    {
        octreeVertex.size = octreeVertexMinSize;
    }
}

PointCloudEngine::Octree::~Octree()
{
    // Delete children
    for (int i = 0; i < 8; i++)
    {
        SafeDelete(children[i]);
    }
}

std::vector<OctreeVertex> PointCloudEngine::Octree::GetOctreeVertices(Vector3 localCameraPosition, float size)
{
    // TODO: View frustum culling, Visibility culling (normals)
    // Only return a vertex if its projected size is smaller than the passed size or it is a leaf node
    std::vector<OctreeVertex> octreeVertices;
    float distanceToCamera = Vector3::Distance(localCameraPosition, octreeVertex.position);

    // Scale the size by the fov and camera distance
    float worldSize = size * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;

    if ((octreeVertex.size < worldSize) || IsLeafNode())
    {
        octreeVertices.push_back(octreeVertex);
    }
    else
    {
        // Traverse the whole octree and add child vertices
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<OctreeVertex> childOctreeVertices = children[i]->GetOctreeVertices(localCameraPosition, size);
                octreeVertices.insert(octreeVertices.end(), childOctreeVertices.begin(), childOctreeVertices.end());
            }
        }
    }

    return octreeVertices;
}

std::vector<OctreeVertex> PointCloudEngine::Octree::GetOctreeVerticesAtLevel(int level)
{
    std::vector<OctreeVertex> octreeVertices;

    if (level == 0)
    {
        octreeVertices.push_back(octreeVertex);
    }
    else if (level > 0)
    {
        // Traverse the whole octree and add child vertices
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<OctreeVertex> childOctreeVertices = children[i]->GetOctreeVerticesAtLevel(level - 1);
                octreeVertices.insert(octreeVertices.end(), childOctreeVertices.begin(), childOctreeVertices.end());
            }
        }
    }

    return octreeVertices;
}

bool PointCloudEngine::Octree::IsLeafNode()
{
    for (int i = 0; i < 8; i++)
    {
        if (children[i] != NULL)
        {
            return false;
        }
    }

    return true;
}
