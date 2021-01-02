#include "foreignactivatoranimation.hpp"

#include <iostream> // FIXME: testing only

#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreMesh.h>
#include <OgreTagPoint.h>

#include <extern/nibtogre/ninode.hpp>
#include <extern/nibtogre/nimodelmanager.hpp> // NiModelPtr

namespace
{
    // See NiModel::createMesh() on the mesh name encoding.  This function relies on the bone
    // name being the same as the mesh's parent NiNode name.
    std::string extractBoneNameFromEntity(Ogre::Entity* entity)
    {
        std::string meshName = entity->getMesh()->getName();
        std::size_t pos = meshName.find_last_of('%');
        return meshName.substr(pos+1);
    }

    std::string getAnimRootBoneName(NiBtOgre::NiModel* model, NiBtOgre::NiNodeRef nodeRef)
    {
        if (NiBtOgre::NiNode *node = model->getRef<NiBtOgre::NiNode>(nodeRef))
        {
            if (NiBtOgre::NiNode *animRootNode = node->getAnimRoot())
            {
                return animRootNode->getName();
            }
        }

        return "";
    }
}

namespace MWRender
{
    // Unlike NPC animations, activator animations are built into the NIF model.
    // The NiModel has its own skeleton rather than using externally supplied skeleton in NPC
    // animations.  So the high level build steps are:
    //
    // 1. load the NIF Model
    // 2. build the skeleton and animation controllers
    //
    //    Loading of the NIF Model (which also builds the skeleton) is done by
    //    Animation::setObjectRoot() which is called from ObjectAnimation ctor.
    //
    //    The animation controllers are normally built during the NIF Model loading.
    //    However it is difficult (impossible?) to ensure the correct skeleton to be
    //    used for the controllers.  So we build the controllers afterwards
    //    similar to the way NPC animations are done using external skeleton model.
    //
    //    Also, building the controllers after the model is built allows NiModelPtr
    //    to be kept in the controllers (else the transform data, which can be potentially
    //    large, will need to be copied to the controllers).
    //
    //    Note that the ".kf" files used in NPC animations start with the ControllerSequence
    //    block.  But for the activator animations we need to examine NiControllerManager to
    //    find out which ones to build. See addActivatorAnimSource() where we call
    //    NiModel::buildAnimation() with a slightly different function signature.
    //
    // 3. build the meshes and link to the skeleton
    //
    //    This step is also done during the NIF Model build.  For Activators using
    //    controllers to animate attached Ogre::Entity there would be no need to link
    //    to any skeletons since we'll be attaching the entities to either the base node's
    //    skeleton (i.e. mSkelBase) or to the SceneNode if the entity is static.
    //
    // 4. instantiate the entities
    // 5. build the animations to work with MWRender::Animation
    //
    // Some examples:
    //
    // Dungeons\Caves\Traps\CTrapLog01.NIF (SCPT 00051AC2)
    // Dungeons\Caves\Triggers\CTrigTripwire01.NIF
    // Dungeons\Sewers\SewerWheel01.NIF
    // Dungeons\Sewers\sewerChannelGate01.NIF
    //
    ForeignActivatorAnimation::ForeignActivatorAnimation(const MWWorld::Ptr &ptr, const std::string& modelName)
        : ObjectAnimation(ptr, modelName)
    {
        if (modelName.empty())
            return;

        if (!mObjectRoot->mSkelBase)
            return;

        //if (mObjectRoot->mForeignObj->mModel->getName().find("geardoor") == std::string::npos)
            //return;

        Ogre::SkeletonInstance *skelinst = mObjectRoot->mSkelBase->getSkeleton();
        if (!skelinst)
            return; // FIXME: can we have an activator without a skeleton?

        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);

        NiBtOgre::NiModel *model = mObjectRoot->mForeignObj->mModel.get();
        const std::map<NiBtOgre::NiNodeRef, Ogre::Entity*>& entityMap
            = mObjectRoot->mForeignObj->mEntities;

        std::map<NiBtOgre::NiNodeRef, Ogre::Entity*>::const_iterator cit = entityMap.begin();
        for (; cit != entityMap.end(); ++cit)
        {
            if (cit->second == mSkelBase)
                continue; // avoid infinite loop

            if (cit->second->isAttached())
                cit->second->detachFromParent();

            std::string boneName = extractBoneNameFromEntity(cit->second);

            if (!skelinst->hasBone(boneName))
            {
                boneName = getAnimRootBoneName(model, cit->first);
                if (!skelinst->hasBone(boneName))
                {
                    // entity must not be animated, attach to the scene node
                    mInsert->attachObject(cit->second);
                    continue;
                }
            }

            // NOTE: bone binding poistions
            //
            // The vertices of the meshes are expected to be built with transforms starting
            // from the nodes/bones to which they will be attached (excluding any local
            // transforms of the nodes/bones themselves).  This way when the bones are moved by
            // the controllers the attached entities will be rendered in their proper positions.
            //
            // It should be possible to include the nodes' local transforms then take them away
            // when attaching the entities.  However, calculating the correct transform gets
            // complicated especially if the entity is being attached to an ancestor bone.
#if 0
            // FIXME: doesn't work as expected, see SewerWheel01 (COC "Dark04Sewers02")
            Ogre::Bone* bone = mSkelBase->getSkeleton()->getBone(boneName);
            //tag =
            mSkelBase->attachObjectToBone(boneName, cit->second,
            bone->_getBindingPoseInverseOrientation(), bone->_getBindingPoseInversePosition());
#else
            mSkelBase->attachObjectToBone(boneName, cit->second);
#endif
        }

        addActivatorAnimSource();
    }

    ForeignActivatorAnimation::~ForeignActivatorAnimation()
    {
    }

    void ForeignActivatorAnimation::addActivatorAnimSource()
    {
        NiModelPtr animModel = mObjectRoot->mForeignObj->mModel;

        // find the ControllerSequences to build
        std::map<std::string, NiBtOgre::NiControllerSequence*> controllerSequences;
        animModel->getControllerSequenceMap(controllerSequences);

        // build each one
        std::map<std::string, NiBtOgre::NiControllerSequence*>::const_iterator cit
            = controllerSequences.begin();

        for (; cit != controllerSequences.end(); ++cit)
        {
            Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
            std::vector<Ogre::Controller<Ogre::Real> > controllers;

            animModel->buildAnimation(mSkelBase, animModel, animSource->mTextKeys,
                    controllers, animModel.get(), cit->second);

            // now setup MWRender::Animation for playing/running the animations
            mAnimSources.push_back(animSource);

            std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animSource->mControllers;
            for (size_t i = 0; i < controllers.size(); i++)
            {
                NifOgre::NodeTargetValue<Ogre::Real> *dstval;
                dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[i].getDestination().get());

                size_t grp = 0;
                if (!mAccumRoot && (dstval->getNode()->getName().find("NonAccum") != std::string::npos))
                {
                    mNonAccumRoot = dstval->getNode();
                    mAccumRoot = mNonAccumRoot->getParent();
                }

                controllers[i].setSource(mAnimationTimePtr[grp]);
                grpctrls[grp].push_back(controllers[i]);
            }
        }
    }

    void ForeignActivatorAnimation::play(const std::string &groupname, int priority, int groups, bool autodisable,
            float speedmult, const std::string &start, const std::string &stop,
            float startpoint, size_t loops, bool loopfallback)
    {
        // play any associated vertex animations as well
        // e.g. Dungeons\Caves\Triggers\CTrigTripwire01.NIF
        if (mObjectRoot->mForeignObj && mObjectRoot->mForeignObj->mVertexAnimEntities.size() > 0)
        {
            for (unsigned int i = 0; i < mObjectRoot->mForeignObj->mVertexAnimEntities.size(); ++i)
            {
                if (!mObjectRoot->mForeignObj->mVertexAnimEntities[i]->hasAnimationState(groupname))
                    continue;

                Ogre::AnimationState *anim
                    = mObjectRoot->mForeignObj->mVertexAnimEntities[i]->getAnimationState(groupname);
                if (anim)
                {
                    anim->setEnabled(true);
                    anim->setLoop(false); // FIXME: should be based on the NIF setting
                }
            }
        }

        Animation::play(groupname, priority, groups, autodisable, speedmult,
                        start, stop, startpoint, loops, loopfallback);
    }

    Ogre::Vector3 ForeignActivatorAnimation::runAnimation(float duration)
    {
        return Animation::runAnimation(duration);
    }
}
