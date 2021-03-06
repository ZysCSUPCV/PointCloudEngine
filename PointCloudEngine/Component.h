#ifndef COMPONENT_H
#define COMPONENT_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    // Abstract class for all components that can be added to SceneObjects
    class Component
    {
    public:
        virtual void Initialize () = 0;
        virtual void Update () = 0;
        virtual void Draw () = 0;
        virtual void Release () = 0;

        // If this component is shared it should not be deleted if the game object is deleted
        bool shared = false;

        // Used to automatically initialize the component e.g. when it is created at runtime
        bool initialized = false;

		// Disabled components won't be updated or drawn
		bool enabled = true;

		// The scene object that this component is attached to
		SceneObject* sceneObject = NULL;
    };
}
#endif