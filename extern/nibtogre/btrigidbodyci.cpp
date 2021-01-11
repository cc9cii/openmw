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
#include "btrigidbodyci.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <OgreResourceManager.h>

#include "nimodel.hpp"
#include "nimodelmanager.hpp"
#include "ninode.hpp"
#include "bhkrefobject.hpp"

namespace
{
    void deleteShape(btCollisionShape* shape)
    {
        if(shape)
        {
            if(shape->isCompound())
            {
                btCompoundShape* compoundShape = static_cast<btCompoundShape*>(shape);
                for(int i = 0; i < compoundShape->getNumChildShapes() ;++i)
                {
                    deleteShape(compoundShape->getChildShape(i));
                }
            }
            delete shape;
        }
    }
}

NiBtOgre::BtRigidBodyCI::BtRigidBodyCI(Ogre::ResourceManager *creator, const Ogre::String& name,
        Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader)
    : Resource(creator, name, handle, group, isManual, loader)
{
}

NiBtOgre::BtRigidBodyCI::~BtRigidBodyCI()
{
    std::map<std::int32_t, std::pair<Ogre::Matrix4, btCollisionShape*> >::iterator iter;
    for (iter = mBtCollisionShapeMap.begin(); iter != mBtCollisionShapeMap.end(); ++iter)
    {
        deleteShape(iter->second.second);
    }
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::BtRigidBodyCI::loadImpl()
{
    std::string modelName = Ogre::Resource::getName(); // remove scale from the name (see -7 below)

    //if (modelName.find("keleton") != std::string::npos)
    //if (modelName.find("robeuc") != std::string::npos)
        //std::cout << modelName << std::endl;

    NiModelPtr nimodel
        = NiBtOgre::NiModelManager::getSingleton().getByName(modelName.substr(0, modelName.length()-7), getGroup());

    if (!nimodel) // shouldn't happen, since we need the Entities created already
        throw std::runtime_error("BtRigidBodyCI: NiModel not loaded");

    const std::vector<NiNodeRef>&  ctlrTargets = nimodel->buildData().getControllerTargets();


    // bhkRigidBodyMap is populated by bhkNiCollisionObject ctor
    // In most cases the target is an NiNode which is also the target of a controller.
    // However, sometimes the target node may be a descendant of an animation controller
    // target.  In such cases we need to build the collision shape using the transform from the
    // root node of the animation (i.e. one with a controller).
    //
    // e.g. BenirusDoor01.NIF (COC AnvilBenirusManorBasement)
    //
    // block 241 is a rigid body that is associated with NiNode block 238 (chunk02),
    // but the root of the animation is NiNode block 225 (gearA) which is controlled by an
    // NiTransformInterpolator in block 35
    //
    // It may be possible to find the animation root node in the ctor of bhkNiCollisoinObject,
    // but that will require making an assumption that all the required blocks have been
    // loaded.  It will be safer to do that after the model has been built.
    //
    // For rendering NiNode::isDynamicMesh() is used for finding the root animation node.  It
    // should be possile to use the same here.  Similarly, NiNode::getTransform() can be used
    // to get the required transforms.

    //           target NiAVObject ref               bhkSerializable ref (e.g. bhkRigidBody)
    //                   |                                    |
    //                   v                                    v
    const std::map<NiAVObjectRef, bhkSerializableRef>& rigidBodies = nimodel->getBhkRigidBodyMap();
    std::map<NiAVObjectRef, bhkSerializableRef>::const_iterator iter(rigidBodies.begin());
    for (; iter != rigidBodies.end(); ++iter)
    {
        //if (iter->second == -1)
            //continue;  // e.g. fire/firetorchlargesmoke.nif%DamageSphere
        // FIXME: check for phantom

        bhkSerializableRef bhkRef = iter->second/*.second*/;
        bhkSerializable *bhk = nimodel->getRef<bhkSerializable>(bhkRef);

        if (bhkRigidBody* body = dynamic_cast<bhkRigidBody*>(bhk))
        {
            mMass[iter->first] = body->mMass;
            mRigidBodies[iter->first] = iter->second;

            // make note of any constraints
            for (std::size_t i = 0; i < body->mConstraints.size(); ++i)
            {
                if (body->mConstraints[i] != -1)
                {
                    bhkSerializable *constraint
                        = nimodel->getRef<bhkSerializable>(body->mConstraints[i]);
                    mConstraints.push_back(constraint);
                }
            }
        }
        else
            mMass[iter->first] = 0.f;

        NiAVObjectRef targetRef = iter->first;
        NiAVObject *target = nimodel->getRef<NiAVObject>(targetRef);

        //if (nimodel->indexToString(target->getNameIndex()) == "sewerChannelGate01b")
        //if (nimodel->indexToString(target->getNameIndex()) == "Chunk01")
            //std::cout << nimodel->getName() << std::endl;

        // FIXME: should confirm that targetRef is indeed NiNode (throw?)
        NiNode *targetNode = nimodel->getRef<NiNode>(targetRef);
        NiNodeRef controlledNodeRef = 0;
        bool dynamic = targetNode->isDynamicMesh(&controlledNodeRef);

        // the mTargetNames map value is the bone name - this allows different targetRef key to
        // be used for the same anim bone
        NiNode *controlledNode = nullptr;
        if (dynamic)
        {
            controlledNode = nimodel->getRef<NiNode>(controlledNodeRef);
            mTargetNames[targetRef] = controlledNode->getName();
        }
        else
            mTargetNames[targetRef] = nimodel->indexToString(target->getNameIndex());

        // expectation is that each target has only one bhkRigidBody
        if (mBtCollisionShapeMap.find(targetRef) != mBtCollisionShapeMap.end())
            throw std::logic_error("rigidbody target collision "+nimodel->indexToString(targetRef));

        // get the bullet shape with the target as a parameter
        // TODO: cloning pre-pade shape (e.g. bhkRigidBody via unique_ptr) may be faster?
        // FIXME: there is an unused map of the same name in NiModel (confusing, should delete)
        if (dynamic)
        {
            mBtCollisionShapeMap[targetRef]
                = std::make_pair(controlledNode->getWorldTransform(), bhk->getShape(*target, controlledNode));
            //                                    ^
            //                                    |
            //                 this should be the world transform of the
            //                 animation root node since it is used by
            //                 PhysicEngine::createAndAdjustRigidBody()
        }
        else if (nimodel->buildData().havokEnabled()) // WARN: only reliable in TES4
        {
            mBtCollisionShapeMap[targetRef]
                = std::make_pair(targetNode->getWorldTransform(), bhk->getShape(*target, targetNode));
        }
        else if (nimodel->buildData().isSkeletonTES4()) // prob. not necessary since skeletons have havok
        {
            mBtCollisionShapeMap[targetRef]
                = std::make_pair(targetNode->getLocalTransform(), bhk->getShape(*target, targetNode));
        }
        else
        {
            mBtCollisionShapeMap[targetRef]
                = std::make_pair(targetNode->getWorldTransform(), bhk->getShape(*target, nullptr));
        }
    }
}

void NiBtOgre::BtRigidBodyCI::unloadImpl()
{
}
