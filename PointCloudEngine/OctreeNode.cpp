#include "OctreeNode.h"

PointCloudEngine::OctreeNode::OctreeNode(const std::vector<Vertex> &vertices, const Vector3 &center, const float &size, const int &depth)
{
    size_t vertexCount = vertices.size();
    
    if (vertexCount == 0)
    {
        ErrorMessage(L"Cannot create Octree Node from empty vertices!", L"CreateNode", __FILEW__, __LINE__);
        return;
    }

    // The octree is generated by fitting the vertices into a cube at the center position
    // Then this cube is splitted into 8 smaller child cubes along the center
    // For each child cube the octree generation is repeated
    // Assign node values given by the parent
    nodeVertex.size = size;
    nodeVertex.position = center;

    // Sum up all the visibility factors to create weighted averages
    float visibilityFactorSums[6] = { 0, 0, 0, 0, 0, 0 };

    // Initialize average colors and variables
    Vector3 averageNormals[6] = { Vector3::Zero, Vector3::Zero, Vector3::Zero, Vector3::Zero, Vector3::Zero, Vector3::Zero };
    double averageReds[6] = { 0, 0, 0, 0, 0, 0 };
    double averageGreens[6] = { 0, 0, 0, 0, 0, 0 };
    double averageBlues[6] = { 0, 0, 0, 0, 0, 0 };

    // Calculate view dependent colors and normals for this node
    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        Vertex v = *it;

        // Average visible color and normal from all 6 view directions
        for (int i = 0; i < 6; i++)
        {
            Vector3 viewDirection = Octree::viewDirections[i];
            viewDirection.Normalize();

            // Calculate visibility of this vertex from the view direction (0 if not visible, 1 if directly orthogonal to view direction)
            float visibilityFactor = v.normal.Dot(-viewDirection);

            if (visibilityFactor > 0)
            {
                // Sum up visible normals
                averageNormals[i] += visibilityFactor * v.normal;

                // Sum up visible colors
                averageReds[i] += visibilityFactor * v.color[0];
                averageGreens[i] += visibilityFactor * v.color[1];
                averageBlues[i] += visibilityFactor * v.color[2];

                // Divide sums by this value in the end
                visibilityFactorSums[i] += visibilityFactor;
            }
        }
    }

    // Divide all the weighted sums in order to get the weighted average
    for (int i = 0; i < 6; i++)
    {
        if (visibilityFactorSums[i] > 0)
        {
            averageNormals[i] /= visibilityFactorSums[i];
        
            averageReds[i] /= visibilityFactorSums[i];
            averageGreens[i] /= visibilityFactorSums[i];
            averageBlues[i] /= visibilityFactorSums[i];

            nodeVertex.normals[i] = PolarNormal(averageNormals[i]);
            nodeVertex.colors[i] = Color16(averageReds[i], averageGreens[i], averageBlues[i]);
        }
        else
        {
            // Make sure that this normal and color is ignored in the weighting process using empty normal and color
            nodeVertex.normals[i] = PolarNormal();
            nodeVertex.colors[i] = Color16();
        }
    }

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
    float childExtend = 0.25f * nodeVertex.size;

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
                children[i] = new OctreeNode(childVertices[i], childCenters[i], size / 2.0f, depth - 1);
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

std::vector<OctreeNodeVertex> PointCloudEngine::OctreeNode::GetVertices(const Vector3 &localCameraPosition, const float &splatSize)
{
    // TODO: View frustum culling, Visibility culling (normals)
    // Only return a vertex if its projected size is smaller than the passed size or it is a leaf node
    std::vector<OctreeNodeVertex> octreeVertices;
    float distanceToCamera = Vector3::Distance(localCameraPosition, nodeVertex.position);

    // Scale the splat size by the fov and camera distance (Result: size at that distance in world space)
    float splatSizeWorld = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;

    if ((nodeVertex.size < splatSizeWorld) || IsLeafNode())
    {
        // Make sure that e.g. single point nodes with size 0 are drawn as well
        if (nodeVertex.size < FLT_EPSILON)
        {
            // Set the size temporarily to the splat size in world space to make sure that this node is visible
            OctreeNodeVertex tmp = nodeVertex;
            tmp.size = splatSizeWorld;

            octreeVertices.push_back(tmp);
        }
        else
        {
            octreeVertices.push_back(nodeVertex);
        }
    }
    else
    {
        // Traverse the whole octree and add child vertices
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<OctreeNodeVertex> childOctreeVertices = children[i]->GetVertices(localCameraPosition, splatSize);
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
        octreeVertices.push_back(nodeVertex);
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
