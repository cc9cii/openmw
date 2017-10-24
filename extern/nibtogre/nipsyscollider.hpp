/*
  Copyright (C) 2015-2017 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#ifndef NIBTOGRE_NIPSYSCOLLIDER_H
#define NIBTOGRE_NIPSYSCOLLIDER_H

#include <OgreVector3.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiPSysCollider
//     NiPSysPlanarCollider
//     NiPSysSphericalCollider
namespace NiBtOgre
{
    class NiStream;
    class Header;

    class NiNode;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysCollider : public NiObject
    {
    public:
        float mBounce;
        bool mSpawnOnCollide;
        bool mDieOnCollide;
        NiPSysSpawnModifierRef mSpawnModifierIndex;
        NiObject   *mParent; // Ptr
        NiObjectRef mNextColliderIndex;
        NiNode     *mColliderObject; // Ptr

        NiPSysCollider(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysPlanarCollider : public NiPSysCollider
    {
    public:
        float mWidth;
        float mHeight;
        Ogre::Vector3 mXAxis;
        Ogre::Vector3 mYAxis;

        NiPSysPlanarCollider(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class NiPSysSphericalCollider : public NiPSysCollider
    {
    public:
        float mRadius;

        NiPSysSphericalCollider(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIPSYSCOLLIDER_H
