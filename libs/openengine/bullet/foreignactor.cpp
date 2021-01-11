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
#include <extern/nibtogre/bhkrefobject.hpp>

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
                = btRigidBody::btRigidBodyConstructionInfo(0.f, // NOTE: 0 mass
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
            body->mMass = 7*ci->mMass[iter->first]; // for enabling ragdoll later
            body->mMotionState = state;

            // FIXME: copied from Bullet example
            body->setDamping(btScalar(0.05), btScalar(0.85));
            body->setDeactivationTime(btScalar(0.8));
            body->setSleepingThresholds(btScalar(1.6), btScalar(2.5));

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

        // store constraints for later removal/delete
        // NOTE: constraints are not enabled until ragdoll is enabled
        for (std::size_t i = 0; i < ci->mConstraints.size(); ++i)
        {
            if (NiBtOgre::bhkConstraint* bhkConst = dynamic_cast<NiBtOgre::bhkConstraint*>(ci->mConstraints[i]))
            {
                // assumed that there are always exactly 2
                if (bhkConst->mEntities.size() != 2)
                    throw std::logic_error("Too many bhkEntities for a constraint");

                std::map<NiBtOgre::bhkEntity*, btRigidBody*> bodies;
                for (std::size_t j = 0; j < 2; ++j)
                {
                    int32_t aRef = bhkConst->mEntities[j]->selfRef();
                    std::map<std::int32_t, RigidBody*>::const_iterator cit = rigidBodyMap.find(aRef);
                    if (cit == rigidBodyMap.end())
                        throw std::runtime_error("cannot find RigidBody for constraints");

                    bodies[bhkConst->mEntities[j]] = cit->second;
                }

                btTypedConstraint* constraint = ci->mConstraints[i]->buildConstraint(bodies);
                if (constraint)
                {
                    parentBody->mConstraints.push_back(constraint);
                    // NOTE: constraints are not added to the world here; they will be when ragdoll is enabled
                }
            }
        }

        mForeignBody = parentBody;
    }

    ForeignActor::~ForeignActor()
    {
        // NOTE: btMotionState is deleted by RigidBody dtor

        std::vector<btTypedConstraint*>::const_iterator cit = mForeignBody->mConstraints.begin();
        for (; cit != mForeignBody->mConstraints.end(); ++cit)
        {
            mEngine->mDynamicsWorld->removeConstraint(*cit);

            delete *cit;
        }

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

    bool ForeignActor::enableRagdoll()
    {
        std::cout << "ForeignActor::enableRagdoll" << std::endl;

        // FIXME: need to remember bone transform before removing and adding the rigid bodies

        btVector3 localInertia(0.f, 0.f, 0.f);
        if (mForeignBody->mMass)
            mForeignBody->getCollisionShape()->calculateLocalInertia(mForeignBody->mMass, localInertia);

        mEngine->mDynamicsWorld->removeRigidBody(mForeignBody);

        mForeignBody->setMassProps(mForeignBody->mMass, localInertia);
        mForeignBody->setMotionState(mForeignBody->mMotionState);
        mForeignBody->mMotionState = nullptr;

        // FIXME: mask settings
        mEngine->mDynamicsWorld->addRigidBody(
            mForeignBody,CollisionType_World|CollisionType_Raycasting,CollisionType_World|CollisionType_Raycasting|CollisionType_Actor|CollisionType_HeightMap);

        //mForeignBody->applyGravity(); // FIXME: doesn't seem to do anything?

        std::map<std::string, RigidBody*>::iterator it = mForeignBody->mChildren.begin();
        for (; it != mForeignBody->mChildren.end(); ++it)
        {
            localInertia = btVector3(0.f, 0.f, 0.f);
            if (it->second->mMass)
                it->second->getCollisionShape()->calculateLocalInertia(it->second->mMass, localInertia);

            mEngine->mDynamicsWorld->removeRigidBody(it->second);

            it->second->setMassProps(it->second->mMass, localInertia);
            it->second->setMotionState(it->second->mMotionState);
            it->second->mMotionState = nullptr;

            // FIXME: mask settings
            mEngine->mDynamicsWorld->addRigidBody(
                it->second,CollisionType_World|CollisionType_Raycasting,CollisionType_World|CollisionType_Raycasting|CollisionType_Actor|CollisionType_HeightMap);

            //it->second->applyGravity(); // FIXME: doesn't seem to do anything?
        }

        // enable constraints
        std::vector<btTypedConstraint*>::const_iterator cit = mForeignBody->mConstraints.begin();
        for (; cit != mForeignBody->mConstraints.end(); ++cit)
        {
            mEngine->mDynamicsWorld->addConstraint(*cit, /*disable collision between linked bodies*/true);
        }

        return true;
    }
}
}
