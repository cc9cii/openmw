#include "skeleton.hpp"

#include <OgreSkeletonManager.h>
#include <OgreResource.h>
#include <OgreSkeleton.h>
#include <OgreBone.h>

#include <components/nif/node.hpp>
#include <components/nifcache/nifcache.hpp>
#include <components/misc/stringops.hpp>

namespace NifOgre
{
// Nif::Node (NiAVObject) has trafo
// - so, need to ignore records that are not based on NiAVObject?
//   (not building bones for NiGeometry nodes anyway, only focus on NiNode nodes and
//   their children)
//
// NiGeometry
//   NiParticles
//     NiAutoNormalParticles <-- ok*
//     NiParticleMeshes      x-- not supported
//     NiParticleSystem                       <- TBD
//       BSStripParticleSystem                <- TBD
//       NiMeshParticleSystem x-- not supported
//     NiRotatingParticles   <-- ok*
//   NiTriBasedGeom
//     BSLODTriShape                          <- TBD
//     NiClod                x-- not supported
//     NiLines               x-- not supported
//     NiTriShape            <-- ok*
//       BSSegmentedTriShape x-- not supported
//       NiScreenElements    x-- not supported
//     NiTriStrips           <-- ok*
//
// NiNode
//   AvoidNode
//   BSBlastNode                              <- TBD
//   BSDamageStage                            <- TBD
//   BSDebrisNode            <-- not supported
//   BSFadeNode              <-- ok*
//   BSLeafAnimNode                           <- TBD
//   BSMasterParticleSystem  <-- not supported
//   BSMultiBoundNode                         <- TBD
//   BSOrderedNode                            <- TBD
//   BSTreeNode                               <- TBD
//   BSValueNode                              <- TBD
//   FxWidget
//     FxButton
//     FxRadioButton         <-- not suppported
//   NiBSAnimationNode       <-- ok*
//   NiBSParticleNode                        <-- TBD
//   NiBillboardNode         <-- ok*
//   NiBone                  <-- not supported
//   NiRoom                  <-- not supported
//   NiRoomGroup             <-- not supported
//   NiSortAdjustNode        <-- not supported
//   NiSwitchNode                            <-- TBD
//     NiLODNode             <-- not supported
//   RootCollisionNode       <-- ok*
//
// NiPSParticleSystem
//   NiPSMeshParticleSystem
//
// NiPortal
//
// -------------------------------------------------------------------
//
// some options for TES4:
//  - existance of NiBSBoneLODController (how to add the branches later in buildBones()?)
//  - look for name "Bip01" (possibly slow?)
//  - nodes in node groups of NiBSBoneLODController
//  - all children of NiBSBoneLODController target (prob. Bip01)
//
// FIXME:
// For ragdolls the bones should correspond to NiNodes containing bhkRigidBody or bhkRigidBodyT.
// A btRigidBody with btCollisionShape will then control the bone using btMotionState.

// Build a bone tree given an Ogre::Skeleton and a root node by recursively calling itself
//
// 1. create a bone (use node's name, FIXME not sure what happens if it exists already)
// 2. add the bone to parent if recursing (otherwise it is the root bone)
// 3. add bone handle to mNifToOgreHandleMap (node->recIndex is the key)
// 4. set orientation/position/scale based on the node and set as 'binding pose'
// 5. check node and controllers types (FIXME: not sure what this is meant to achieve)
// 6. recurse into children if 'node' is a NiNode
void NIFSkeletonLoader::buildBones (Ogre::Skeleton *skel, const Nif::Node *node, Ogre::Bone *parent)
{
    // Don't check here since needSkeleton() has already checked
    // FIXME: It seems that some objects require bones and/or transformation or will be in
    // wrong places.  These need to be handled elsewhere other than skeleton loader.
    if (0)//!ninode || node->recType == Nif::RC_BSFadeNode) // FIXME experiment
    {
        // check NiGeometry, NiPSParticleSystem and NiPortal are ignored
        const Nif::NiGeometry         *nigeom   = dynamic_cast<const Nif::NiGeometry*>(node);
        //const Nif::NiPSParticleSystem *nipsys   = dynamic_cast<const Nif::NiPSParticleSystem*>(node);
        //const Nif::NiPortal           *niportal = dynamic_cast<const Nif::NiPortal*>(node);
        if (!nigeom/* && !nipsys && !niportal*/)
            warn("Unhandled "+node->recName+" "+node->name+" in "+skel->getName());

        return;
    }
    // FIXME: sometimes a bone is created under BSFadeNode but none of the children have bones...
    //        currently resolved by ignoring BSFadeNode in NifOgre::NIFSkeletonLoader::needSkeleton()
    Ogre::Bone *bone;
    if (node->name.empty())
    {
        // HACK: use " " instead of empty name, otherwise Ogre will replace it with an auto-generated
        // name in SkeletonInstance::cloneBoneAndChildren.
        static const char* emptyname = " ";
        if (!skel->hasBone(emptyname))
            bone = skel->createBone(emptyname);
        else
            bone = skel->createBone();
    }
    else
    {
        if(!skel->hasBone(node->name))
            bone = skel->createBone(node->name);
        else
            bone = skel->createBone();
    }

    if(parent) parent->addChild(bone);
    mNifToOgreHandleMap[node->recIndex] = bone->getHandle();

    bone->setOrientation(node->trafo.rotation);
    bone->setPosition(node->trafo.pos);
    bone->setScale(Ogre::Vector3(node->trafo.scale));
    bone->setBindingPose();

    if(!(node->recType == Nif::RC_NiNode || /* Nothing special; children traversed below */
         node->recType == Nif::RC_RootCollisionNode || /* handled in nifbullet (hopefully) */
         node->recType == Nif::RC_NiTriShape || /* Handled in the mesh loader */
         node->recType == Nif::RC_NiBSAnimationNode || /* Handled in the object loader */
         node->recType == Nif::RC_NiBillboardNode || /* Handled in the object loader */
         node->recType == Nif::RC_NiBSParticleNode ||
         node->recType == Nif::RC_NiCamera ||
         node->recType == Nif::RC_NiAutoNormalParticles ||
         node->recType == Nif::RC_NiRotatingParticles ||
         node->recType == Nif::RC_NiParticleSystem ||      // <-- should be ignored (NiGeometry)
         node->recType == Nif::RC_BSStripParticleSystem || // <-- should be ignored (NiGeometry)
         node->recType == Nif::RC_BSFadeNode || // should be the same as NiNode
         node->recType == Nif::RC_NiTriStrips              // <-- should be ignored (NiGeometry)
         ))
        warn("Unhandled "+node->recName+" "+node->name+" in "+skel->getName());

    Nif::ControllerPtr ctrl = node->controller;
    while(!ctrl.empty())
    {
        if(!(ctrl->recType == Nif::RC_NiParticleSystemController ||
             ctrl->recType == Nif::RC_NiBSPArrayController ||
             ctrl->recType == Nif::RC_NiVisController ||
             ctrl->recType == Nif::RC_NiUVController ||
             ctrl->recType == Nif::RC_NiKeyframeController ||
             ctrl->recType == Nif::RC_NiGeomMorpherController ||
             ctrl->recType == Nif::RC_NiTransformController || // NiTransformController replaces NiKeyframeController
             // FIXME: implement below ones in NifOgre::NIFObjectLoader::createNodeControllers()
             // for now suppress warning
             ctrl->recType == Nif::RC_NiBSBoneLODController ||
             ctrl->recType == Nif::RC_bhkBlendController ||
             ctrl->recType == Nif::RC_NiControllerManager ||
             ctrl->recType == Nif::RC_NiMultiTargetTransformController
             ))
            warn("Unhandled "+ctrl->recName+" from node "+node->name+" in "+skel->getName());
        ctrl = ctrl->next;
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &children = ninode->children;
        for(size_t i = 0;i < children.length();i++)
        {
            if(!children[i].empty())
                buildBones(skel, children[i].getPtr(), bone);
        }
    }
    // FIXME: see notes above
    //else
       //std::cout << "Unhandled "<<node->recName<<" "<<node->name<<" in "<<skel->getName()<<std::endl;
}

// callback class for the 3rd parameter in Ogre::SkeletonManager::create()
// which is called by createSkeleton()
void NIFSkeletonLoader::loadResource(Ogre::Resource *resource)
{
    Ogre::Skeleton *skel = dynamic_cast<Ogre::Skeleton*>(resource);
    OgreAssert(skel, "Attempting to load a skeleton into a non-skeleton resource!");

    // should have been loaded already by NifOgre::NIFObjectLoader::load
    Nif::NIFFilePtr nif(Nif::Cache::getInstance().load(skel->getName()));
    nif->parse(); // FIXME: testing
    const Nif::Node *node = static_cast<const Nif::Node*>(nif->getRoot(0));

    try {
        buildBones(skel, node);
    }
    catch(std::exception &e) {
        std::cerr<< "Exception while loading "<<skel->getName() <<std::endl;
        std::cerr<< e.what() <<std::endl;
        return;
    }
}


// Conditions seem to be that:
//
// - boneTrafo exists, or
// - at least one controller in the chain is NiKeyFrameController (and is active?), or
// - node name is "AttachLight" or "ArrowBone", or
// - one of the child node requires a skeleton, or
// - the tree is not limited to NiNode, RootCollisionNode, NiTriShape or NiTriStrips only
//
// - ?? what are the remaining node types?
//
bool NIFSkeletonLoader::needSkeleton(const Nif::Node *node)
{
    /* We need to be a little aggressive here, since some NIFs have a crap-ton
     * of nodes and Ogre only supports 256 bones. We will skip a skeleton if:
     * There are no bones used for skinning, there are no keyframe controllers, there
     * are no nodes named "AttachLight" or "ArrowBone", and the tree consists of NiNode,
     * NiTriShape, and RootCollisionNode types only.
     */
    if(node->boneTrafo)
        return true;

    // TES3 checks for NiKeyframeController, TES4 checks for NiBSBoneLODController
    if(!node->controller.empty())
    {
        Nif::ControllerPtr ctrl = node->controller;
        do {
            if((ctrl->recType == Nif::RC_NiKeyframeController || Nif::RC_NiBSBoneLODController)
                    && ((ctrl->flags & Nif::NiNode::ControllerFlag_Active) != 0))
                return true;
        } while(!(ctrl=ctrl->next).empty());
    }

    // FIXME: Not sure why "AttachLight" implies a skeleton is needed
    //        Possibley because MWRender::Animation::addExtraLight() uses attachObjectToBone()?
    if (node->name.find("AttachLight") != std::string::npos || node->name == "ArrowBone")
        return true;

    if(node->recType == Nif::RC_NiNode ||
            node->recType == Nif::RC_BSFadeNode ||
            node->recType == Nif::RC_BSValueNode ||
            node->recType == Nif::RC_NiSwitchNode || // probably has NiNode children
            node->recType == Nif::RC_NiBillboardNode || // NiNode that faces the camera
            node->recType == Nif::RC_RootCollisionNode)
    {
        const Nif::NiNode *ninode = static_cast<const Nif::NiNode*>(node);
        const Nif::NodeList &children = ninode->children;
        for(size_t i = 0;i < children.length();i++)
        {
            if(!children[i].empty())
            {
                if(needSkeleton(children[i].getPtr()))
                    return true;
            }
        }
        return false;
    }

    // check if have skin, else no need for a skeleton on this branch (TES3)
    // TES4 skeleton.nif nodes do not have skins, need another way to identify (see above)
    if(node->recType == Nif::RC_NiTriShape || node->recType == Nif::RC_NiTriStrips ||
       node->recType == Nif::RC_BSLODTriShape)
    {
        const Nif::NiGeometry *nigeom = static_cast<const Nif::NiGeometry*>(node);
        if (nigeom->skin.empty())
            return false;
        else
            return true;
    }

    // FIXME: hack BSValueNode is probably for positioning?
    // FIXME: this logic does not work since NiTriShape can have NiSkinInstance and bones
    //if(node->recType == Nif::RC_NiTriShape || node->recType == Nif::RC_NiTriStrips ||
       //node->recType == Nif::RC_BSValueNode || node->recType == Nif::RC_BSLODTriShape)
        //return false;

    // FIXME
    std::cout << "unexpected skel " << node->recName << ", " << node->name << std::endl;
    return true;
}

// New strategy for ragdoll: Jan 2017
//
// All ragdolls are to be implemented using skeletons.  Now, there isn't a strong
// justifications for this, but the gut feeling says that the majority of the ragdolls have
// skeletons anyway (npc's and creatures).
//
// This will allow tag points to be created based on node offset rather than the vertices
// having them baked in.
//
// It is not clear at this point if there will be any performance impact.  If it turns out to
// be the case, we can revisit this decision at that time.
//
// The benefit, I hope, will be a simpler implementation (i.e. no special cases).  Having said
// that, some ragdolls only "activate" when the creature/npc is dead - otherwise they are
// driven by animation.  Others are always under physics control (e.g. scale, lamp, practice
// target).
//
// Another (minor) benefit is to be able to keep the existing implementation of attaching
// lights to bones.
//
// Having decided to use skeleton/bones, we have to worry about scaling.  Not necessarily
// non-uniform scaling, but it may be a good opportunity to get that going at the same time.
// The only trouble is that I don't really understand how that's meant to work.  Also, it is
// unclear if Ogre 1.10 should be abandoned in favour of 2.0 (or even 2.1) implementation of
// skeleton.

// 1. check if a sekeleton should be built
// 2. if so create a skeleton and return its pointer (or a nullptr if not built)
Ogre::SkeletonPtr NIFSkeletonLoader::createSkeleton(const std::string &name,
                                                    const std::string &group, const Nif::Node *node)
{
    bool forceskel = false;
    std::string::size_type extpos = name.rfind('.');
    if(extpos != std::string::npos && name.compare(extpos, name.size()-extpos, ".nif") == 0)
    {
        Ogre::ResourceGroupManager &resMgr = Ogre::ResourceGroupManager::getSingleton();
        forceskel = resMgr.resourceExistsInAnyGroup(name.substr(0, extpos)+".kf");
    }

    if(forceskel || needSkeleton(node))
    {
        // skeleton will be loaded when loadResource() is called back
        Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
        return skelMgr.create(name, group, true, &sLoaders[name]);
    }

    return Ogre::SkeletonPtr();
}

// Looks up an Ogre Bone handle ID from a NIF's record index. Should only be
// used when the bone name is insufficient as this is a relatively slow lookup
int NIFSkeletonLoader::lookupOgreBoneHandle(const std::string &nifname, int idx)
{
    LoaderMap::const_iterator loader = sLoaders.find(nifname);
    if(loader != sLoaders.end())
    {
        std::map<int,int>::const_iterator entry = loader->second.mNifToOgreHandleMap.find(idx);
        if(entry != loader->second.mNifToOgreHandleMap.end())
            return entry->second;
    }

    // some nifs may not have bones
    return -1;
    //throw std::runtime_error("Invalid NIF record lookup ("
            //+nifname+", index "+Ogre::StringConverter::toString(idx)+")");
}

NIFSkeletonLoader::LoaderMap NIFSkeletonLoader::sLoaders;

}
