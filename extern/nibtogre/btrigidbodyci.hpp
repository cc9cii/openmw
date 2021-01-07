/*
  Copyright (C) 2019-2021 cc9cii

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
#ifndef NIBTOGRE_BTRIGIDBODYCI_H
#define NIBTOGRE_BTRIGIDBODYCI_H

#include <memory>
#include <vector>
#include <map>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include <OgreResource.h>
#include <OgreMatrix4.h>

#include "niobject.hpp" // NiAVObjectRef

namespace Ogre
{
    class ResourceManager;
    class ManualResourceLoader;
}

namespace NiBtOgre
{
    class bhkSerializable;

    class BtRigidBodyCI: public Ogre::Resource
    {
        BtRigidBodyCI();
        BtRigidBodyCI(const BtRigidBodyCI& other);
        BtRigidBodyCI& operator=(const BtRigidBodyCI& other);

    protected:
        void loadImpl();
        void unloadImpl();

    public:
        // FIXME: shouldn't really be public, and we need to combine these into a struct or something
        //
        //      target NiNode ref     NiNode world transform
        //              |                   |
        //              v                   v
        std::map<NiAVObjectRef, std::pair<Ogre::Matrix4, btCollisionShape *> > mBtCollisionShapeMap; // used by RigidBody
        std::map<NiAVObjectRef, std::string> mTargetNames; // for doors and activators
        std::map<NiAVObjectRef, float> mMass;
        std::vector<bhkSerializable*> mConstraints;
        std::map<NiAVObjectRef, bhkSerializableRef> mRigidBodies;

        BtRigidBodyCI(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader);
        ~BtRigidBodyCI();
    };
}

#endif // NIBTOGRE_BTRIGIDBODYCI_H
