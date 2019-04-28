#ifndef STRUCTURES_H
#define STRUCTURES_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct Color16
    {
        // 6 bits red, 6 bits green, 4 bits blue
        unsigned short data;

        Color16()
        {
            data = 0;
        }

        Color16(byte red, byte green, byte blue)
        {
            byte r = round(63 * (red / 255.0f));
            byte g = round(63 * (green / 255.0f));
            byte b = round(15 * (blue / 255.0f));

            data = r << 10;
            data = data | g << 4;
            data = data | b;
        }
    };

    struct PolarNormal
    {
        // Compact representation of a normal with polar coordinates using inclination theta and azimuth phi
        // When theta=0 and phi=0 this represents an empty normal (0, 0, 0)
        // [0, pi] therefore 0=0, 255=pi
        byte theta;
        // [-pi, pi] therefore 0=-pi, 255=pi
        byte phi;

        PolarNormal()
        {
            theta = 0;
            phi = 0;
        }

        PolarNormal(Vector3 normal)
        {
            normal.Normalize();

            theta = 255.0f * (acos(normal.z) / XM_PI);
            phi = 127.5f + 127.5f * (atan2f(normal.y, normal.x) / XM_PI);

            if (theta == 0 && phi == 0)
            {
                // Set phi to another value than 0 to avoid representing the empty normal (0, 0, 0)
                // This is okay since phi has no inpact on the saved normal anyways
                phi = 128;
            }
        }

        Vector3 ToVector3()
        {
            if (theta == 0 && phi == 0)
            {
                return Vector3(0, 0, 0);
            }

            float t = XM_PI * (theta / 255.0f);
            float p = XM_PI * ((phi / 127.5f) - 1.0f);

            return Vector3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
        }
    };

    struct Vertex
    {
        // Stores the .ply file vertices
        Vector3 position;
        Vector3 normal;
        byte color[3];
    };

	struct OctreeNodeProperties
	{
		// Mask where the lowest 8 bit store one bit for each of the children: 1 when it exists, 0 when it doesn't
		// The shader can only parse 32bit values -> combine to UINT (8 bit mask, 3 * 8 bit weights) with the weights
		byte childrenMask;

		// Each weight represents the percentage of points assigned to this cluster (0=0%, 255=100%)
		// One weight can be omitted because the sum of all weights is always 100%
		byte weights[3];

		// The different cluster mean normals and colors in object space calculated by k-means algorithm with k=4
		PolarNormal normals[4];
		Color16 colors[4];
	};

    struct OctreeNodeVertex
    {
        // Bounding volume cube position
        Vector3 position;

		// The properties of the node
		OctreeNodeProperties properties;

        // Width of the whole cube
        float size;
    };

    // Stores all the data that is needed to create octree nodes
    struct OctreeNodeCreationEntry
    {
        UINT nodesIndex;
		UINT childrenIndex;
        std::vector<Vertex> vertices;
        Vector3 position;
        float size;
        int depth;
    };

	// Stores all the data that is needed to traverse the octree
	struct OctreeNodeTraversalEntry
	{
		UINT index;
		Vector3 position;
		float size;
		int depth;
	};

	// Same constant buffers as in hlsl file, keep packing rules in mind
	struct OctreeConstantBuffer
	{
		Matrix World;
		Matrix View;
		Matrix Projection;
		Matrix WorldInverseTranspose;
		Vector3 cameraPosition;
		float padding0;
		Vector3 localCameraPosition;
		float padding1;
		Vector4 localViewFrustum[8];
		float fovAngleY;
		float splatSize;
		float samplingRate;
		float overlapFactor;
		int level;

		// Compute shader data
		UINT inputCount;

		float padding2;
		float padding3;
	};
}

#endif