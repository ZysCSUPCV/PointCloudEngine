#ifndef IRENDERER_H
#define IRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class IRenderer
    {
    public:
		virtual void SetLighting(const bool& useLighting) = 0;
        virtual void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize) = 0;
    };
}
#endif
