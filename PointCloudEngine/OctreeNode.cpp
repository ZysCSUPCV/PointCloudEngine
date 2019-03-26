#include "OctreeNode.h"

PointCloudEngine::OctreeNode::OctreeNode(const std::vector<Vertex> &vertices, const Vector3 &center, OctreeNode *parent, const int &depth)
{
    size_t size = vertices.size();
    
    if (size == 0)
    {
        ErrorMessage(L"Cannot create Octree Node from empty vertices!", L"CreateNode", __FILEW__, __LINE__);
        return;
    }

    // The octree is generated by fitting the vertices into a cube at the center position
    // Then this cube is splitted into 8 smaller child cubes along the center
    // For each child cube the octree generation is repeated
    this->parent = parent;

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
        Vertex v = *it;

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
    std::vector<Vertex> childVertices[8];

    // Fit each vertex into its corresponding child cube
    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        Vertex v = *it;

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

    // Correlates to the assigned child vertices
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
    if (depth > 0)
    {
        for (int i = 0; i < 8; i++)
        {
            if (childVertices[i].size() > 0)
            {
                children[i] = new OctreeNode(childVertices[i], childCenters[i], this, depth - 1);
            }
        }
    }
}

PointCloudEngine::OctreeNode::~OctreeNode()
{
    // Delete children
    for (int i = 0; i < 8; i++)
    {
        SafeDelete(children[i]);
    }
}

std::vector<OctreeNodeVertex> PointCloudEngine::OctreeNode::GetVertices(const Vector3 &localCameraPosition, const float &size)
{
    // TODO: View frustum culling, Visibility culling (normals)
    // Only return a vertex if its projected size is smaller than the passed size or it is a leaf node
    std::vector<OctreeNodeVertex> octreeVertices;
    float distanceToCamera = Vector3::Distance(localCameraPosition, octreeVertex.position);

    // Scale the size by the fov and camera distance (how big is the size at that distance in world space?)
    float worldSize = size * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;

    if ((octreeVertex.size < worldSize) || IsLeafNode())
    {
        // Make sure that e.g. nodes with size 0 are drawn as well
        OctreeNodeVertex tmp = octreeVertex;
        tmp.size = worldSize;

        octreeVertices.push_back(tmp);
    }
    else
    {
        // Traverse the whole octree and add child vertices
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<OctreeNodeVertex> childOctreeVertices = children[i]->GetVertices(localCameraPosition, size);
                octreeVertices.insert(octreeVertices.end(), childOctreeVertices.begin(), childOctreeVertices.end());
            }
        }
    }

    return octreeVertices;
}

std::vector<OctreeNodeVertex> PointCloudEngine::OctreeNode::GetVerticesAtLevel(const int &level)
{
    std::vector<OctreeNodeVertex> octreeVertices;

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
                std::vector<OctreeNodeVertex> childOctreeVertices = children[i]->GetVerticesAtLevel(level - 1);
                octreeVertices.insert(octreeVertices.end(), childOctreeVertices.begin(), childOctreeVertices.end());
            }
        }
    }

    return octreeVertices;
}

bool PointCloudEngine::OctreeNode::IsLeafNode()
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
