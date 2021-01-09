/*
 * =====================================================================================
 *
 *       Filename:  BtOgrePG.h
 *
 *    Description:  The part of BtOgre that handles information transfer from Bullet to
 *                  Ogre (like updating graphics object positions).
 *
 *        Version:  1.0
 *        Created:  27/12/2008 03:40:56 AM
 *
 *         Author:  Nikhilesh (nikki)
 *
 * =====================================================================================
 */

#ifndef BtOgreGP_H_
#define BtOgreGP_H_

#include "btBulletDynamicsCommon.h"

#include <OgreMatrix4.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreBone.h>
#include <OgreSceneNode.h>

#include "BtOgreExtras.h"

namespace BtOgre {

//A MotionState is Bullet's way of informing you about updates to an object.
//Pass this MotionState to a btRigidBody to have your SceneNode updated automaticaly.
class RigidBodyState : public btMotionState
{
    protected:
        btTransform mGraphicsWorldTrans;
        btTransform mCenterOfMassOffset;
        Ogre::Matrix4 mParentTrans;

        Ogre::SceneNode *mSceneNode;
        Ogre::Bone *mBone;

    public:
        // NOTE: Made the constructor and variable names as per btDefaultMotionState
        //       (makes following the code somewhat easier)
        //       The key difference is that the "user pointer" is Ogre::SceneNode
        //       and we don't keep startTrans around (doesn't seem to be ever used anyway?)
        RigidBodyState(Ogre::SceneNode *node, Ogre::Bone *bone, const Ogre::Matrix4& parentTrans,
                const btTransform& startTrans = btTransform::getIdentity(),
                const btTransform& centerOfMassOffset = btTransform::getIdentity())
            : mParentTrans(parentTrans),
              mGraphicsWorldTrans(startTrans),
              mCenterOfMassOffset(centerOfMassOffset),
              mSceneNode(node),
              mBone(bone)
        {
        }
#if 0
        RigidBodyState(Ogre::SceneNode *node)
            : mGraphicsWorldTrans(((node != NULL) ? BtOgre::Convert::toBullet(node->getOrientation()) : btQuaternion(0,0,0,1)),
                         ((node != NULL) ? BtOgre::Convert::toBullet(node->getPosition())    : btVector3(0,0,0))),
              mCenterOfMassOffset(btTransform::getIdentity()),
              mSceneNode(node)
        {
        }
#endif
        // called by btRigidBody::setupRigidBody during construction to set its m_worldTransform,
        // making it ignore btRigidBodyConstructionInfo::m_startWorldTransform
        //
        // also called by btRigidBody::predictIntegratedTransform
        virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const
        {
            // return the previously saved mGraphicsWorldTrans from setWorldTransform
            centerOfMassWorldTrans = mGraphicsWorldTrans * mCenterOfMassOffset.inverse();
        }

        // called by btDiscreteDynamicsWorld::synchronizeSingleMotionState
        virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans);

        void setNode(Ogre::SceneNode *node)
        {
            mSceneNode = node;
        }
};

//Softbody-Ogre connection goes here!

}

#endif
