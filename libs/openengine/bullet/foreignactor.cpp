#include "foreignactor.hpp"

#include <iostream> // FIXME: temp testing

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <OgreSceneManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/misc/stringops.hpp>

#include <extern/nibtogre/nimodel.hpp>
#include <extern/nibtogre/btrigidbodycimanager.hpp> // BtRigidBodyCIPtr
#include <extern/nibtogre/btrigidbodyci.hpp>

#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

namespace OEngine {
namespace Physic
{

    ForeignActor::ForeignActor(const std::string& name, const std::string& model, const Ogre::Entity& skelBase,
            PhysicEngine *engine, const Ogre::Vector3& position, const Ogre::Quaternion& rotation, float scale)
    : PhysicActor(name, model, engine, position, rotation, scale)
    {
        PhysicActor::setForeign();

        std::string sid = (boost::format("%07.3f") % scale).str(); // FIXME: we don't need this
        std::string lowerMesh = model+sid;
        Misc::StringUtils::lowerCaseInPlace(lowerMesh);

        Ogre::SkeletonInstance *skelInst = nullptr;
        if (&skelBase)
            skelInst = skelBase.getSkeleton();

        std::map<std::int32_t, RigidBody*> rigidBodyMap; // lookup for constraints

        BtRigidBodyCIPtr ci
            = NiBtOgre::BtRigidBodyCIManager::getSingleton().getOrLoadByName(lowerMesh, "General");

        int numBodies = 0; // keep track of Rigid Bodies with the same 'name'
        RigidBody *parentBody;
        std::map<std::int32_t, std::pair<Ogre::Matrix4, btCollisionShape*> >::const_iterator iter;
        for (iter = ci->mBtCollisionShapeMap.begin(); iter != ci->mBtCollisionShapeMap.end(); ++iter)
        {
            btCollisionShape *collisionShape = iter->second.second;
            if (!collisionShape)
                continue; // phantom

            // FIXME: not sure what the correct havok scaling for mass might be
            collisionShape->setLocalScaling(btVector3(scale, scale, scale));
            btRigidBody::btRigidBodyConstructionInfo CI
                = btRigidBody::btRigidBodyConstructionInfo(0*7*ci->mMass[iter->first], // NOTE: 0 mass
                    0/*btMotionState**/,
                    collisionShape,
                    btVector3(0.f, 0.f, 0.f)); // local inertia

            //CI.m_localInertia.setZero();
            //CI.m_collisionShape->calculateLocalInertia(CI.m_mass, CI.m_localInertia);
            //CI.m_localInertia /= 10;

            // world transform of SceneNode
            Ogre::Matrix4 sceneNodeTrans;
            sceneNodeTrans.makeTransform(position, Ogre::Vector3(scale), rotation);

            // world transform of target NiNode
            const Ogre::Matrix4& targetTrans = iter->second.first;

            // combine then convert to bt format
            Ogre::Matrix4 m = sceneNodeTrans * targetTrans;
            Ogre::Vector3 p = m.getTrans();
            Ogre::Quaternion q = m.extractQuaternion();
            btTransform startTrans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(p.x, p.y, p.z));

            //Ogre::SceneNode *childNode = nodeMap.find(iter->first)->second;

            Ogre::Bone* bone = nullptr;
            if (skelInst && skelInst->hasBone(ci->mTargetNames[iter->first]))
                bone = skelInst->getBone(ci->mTargetNames[iter->first]);

            // NOTE: dtor of RigidBody deletes RigidBodyState
            BtOgre::RigidBodyState *state
                = new BtOgre::RigidBodyState(nullptr/*childNode*/, bone, sceneNodeTrans, startTrans);
            //CI.m_motionState = state; // NOTE: not enabled until ragdoll

            // NOTE: 'name' should be the same for collision detection/raycast
            RigidBody *body = new RigidBody(CI, name);
            body->mPlaceable = false;
            body->mLocalTransform = iter->second.first; // needed for rotateObject() and moveObject()
            body->mIsForeign = true;

#if 1
            if (body->getCollisionShape()->getUserIndex() == 4) // useFullTransform
                // NOTE: effectively does nothing since scaledBoxTranslation is ZERO and boxRotation is IDENTITY
                //adjustRigidBody(body, position, rotation, Ogre::Vector3(0.f) * scale, Ogre::Quaternion::IDENTITY);
                std::cout << "user index 4 " << ci->mTargetNames[iter->first] << std::endl;
            else
#endif
            {
                body->setWorldTransform(startTrans); // prob. unnecessary since using btMotionState

                body->mBindingPosition = btVector3(p.x, p.y, p.z);
                body->mBindingOrientation = btQuaternion(q.x, q.y, q.z, q.w);
                body->mTargetName = ci->mTargetNames[iter->first];
            }

            // keep pointers around to delete later
            if (numBodies == 0)
                parentBody = body;
            else
                parentBody->mChildren.insert(std::make_pair(ci->mTargetNames[iter->first], body));

            // iter->first is NiNode, need to get the rigidbody
            // FIXME: really shouldn't be doing all this lookup here
            std::map<NiBtOgre::NiAVObjectRef, NiBtOgre::bhkSerializableRef>::const_iterator bit
                = ci->mRigidBodies.find(iter->first);

            if (bit == ci->mRigidBodies.end())
                throw std::runtime_error("NiNode for a RigidBody not found");

            rigidBodyMap[bit->second] = body; // for constraints below

            engine->mDynamicsWorld->addRigidBody(
                    body,CollisionType_World|CollisionType_Raycasting,CollisionType_World|CollisionType_Raycasting|CollisionType_Actor|CollisionType_HeightMap);
            //if (numBodies == 0)
                //engine->mCollisionObjectMap[name] = body; // register only the parent

            ++numBodies;
        }

        // FIXME: constraints go here

        mForeignBody = parentBody;
    }

    ForeignActor::~ForeignActor()
    {
        if(mForeignBody)
        {
            std::map<std::string, RigidBody*>::iterator it = mForeignBody->mChildren.begin();
            for (; it != mForeignBody->mChildren.end(); ++it)
                mEngine->mDynamicsWorld->removeRigidBody(it->second);

            mEngine->mDynamicsWorld->removeRigidBody(mForeignBody);

            std::map<std::string, RigidBody*>::iterator it2 = mForeignBody->mChildren.begin();
            for (; it2 != mForeignBody->mChildren.end(); ++it2)
                delete it2->second;

            delete mForeignBody;
        }
    }

    void ForeignActor::setPosition(const Ogre::Vector3 &pos)
    {
        // FIXME
        //std::cout << "ForeignActor::setPosition " << pos << std::endl;
    }

    void ForeignActor::setRotation (const Ogre::Quaternion& rotation)
    {
        // FIXME
        //std::cout << "ForeignActor::setRotation " << rotation << std::endl;
    }

    btCollisionObject *ForeignActor::getRigidBody() const
    {
        return mForeignBody;
    }
}
}
