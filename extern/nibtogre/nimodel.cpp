/*
  Copyright (C) 2015-2020 cc9cii

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
#include "nimodel.hpp"

#include <memory>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <boost/algorithm/string.hpp>

#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreSkeletonManager.h>
#include <OgreSkeleton.h>

#include <extern/fglib/fgtri.hpp>

#include "niobject.hpp"
#include "bhkrefobject.hpp" // bhkConstraint
#include "btogreinst.hpp"
#include "ninode.hpp"
#include "nisequence.hpp"
#include "nigeometry.hpp" // for getVertices()
#include "nitimecontroller.hpp" // NiMultiTargetTransformController

// "nif" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// This name is required later for Ogre resource managers such as MeshManager.
// The file is opened by mNiStream::mStream.
//
// FIXME: there could be duplicates b/w TES3 and TES4/5
NiBtOgre::NiModel::NiModel(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                           const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                           const Ogre::String& nif/*, bool showEditorMarkers*/)
    : Resource(creator, name, handle, group, isManual, loader)
    , mGroup(group)
    , mCurrIndex(-1) // WARN: for debugging only, may be removed in future
    , mModelName(name)
    , mNif(nif)
    , mRootNode(nullptr)
    , mBuildData(*this)
    , mShowEditorMarkers(false/*showEditorMarkers*/)
{
    mNiObjects.clear();
    mSkeleton.reset();
}

NiBtOgre::NiModel::~NiModel()
{
    //unload(); // FIXME: causes exceptions trying to notify the creator (NiModelManager)?
}

void NiBtOgre::NiModel::prepareImpl()
{
#if defined(__GNUC__) && __GNUC__ < 8
    mNiStream = std::unique_ptr<NiBtOgre::NiStream>(new NiStream(mNif)); // WARN: may throw
    mNiHeader = std::unique_ptr<NiBtOgre::NiHeader>(new NiHeader(mNiStream.get()));
#else
    mNiStream = std::make_unique<NiBtOgre::NiStream>(mNif); // WARN: may throw
    mNiHeader = std::make_unique<NiBtOgre::NiHeader>(mNiStream.get());
#endif
}

void NiBtOgre::NiModel::unprepareImpl()
{
    mNiHeader.reset();
    mNiStream.reset();
}

void NiBtOgre::NiModel::unloadImpl()
{
    for (std::size_t i = 0; i < mMeshes.size(); ++i)
        mMeshes[i].first.reset();

    mSkeleton.reset();
}

void NiBtOgre::NiModel::preLoadImpl() // for manual loading
{
    prepareImpl();
}

void NiBtOgre::NiModel::preUnloadImpl() // for manual loading
{
    unprepareImpl();
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::NiModel::loadImpl()
{
    createNiObjects();

    findBoneNodes(true); // not really needed, but this allows NiNode::addBones() to search for names
    // objects being built may require the skeleton to exist, so load it now
    // e.g. NiMultiTargetTransformController::build(), NiTriBasedGeom::buildSubMesh()
    buildSkeleton(true/*load*/);

    //if (blockType(0) == "NiTriShape")
        //return; // FIXME: morroblivion\environment\bittercoast\bcscum03.nif

    createMesh();
    buildModel();
}

void NiBtOgre::NiModel::createNiObjects()
{
    if (!mNiObjects.empty())
        return; // don't create the objects again

    //if (mModelName.find("smeltermarker") != std::string::npos)/*fxambwatersalmon02b*/
        //std::cout << mModelName << std::endl;
    //if (getName().find("vgeardoor01") != std::string::npos)
        //std::cout << "door" << std::endl;

    mNiObjects.resize(mNiHeader->numBlocks());
    if (mNiStream->nifVer() >= 0x0a000100) // from 10.0.1.0
    {
        std::uint32_t numBlocks = mNiHeader->numBlocks(); // make a copy since it can increase inside the loop
        for (std::uint32_t i = 0; i < numBlocks; ++i)
        {
            mCurrIndex = i; // FIXME: debugging only

            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            mNiObjects[i] = NiObject::create(mNiHeader->blockType(i), i, mNiStream.get(), *this, mBuildData);
        }
    }
    else
    {
        for (std::uint32_t i = 0; i < mNiHeader->numBlocks(); ++i)
        {
            mCurrIndex = i; // FIXME: debugging only
#if 0
            std::string blockName = mNiStream->readString();
            if (blockName == "RootCollisionNode")
                std::cout << name << " : " << "RootCollisionNode" << std::endl;
            if (blockName == "AvoidNode")
                std::cout << name << " : " << "AvoidNode" << std::endl;
            //std::cout << name << " : " << "BoundingBox" << std::endl;
            mNiObjects[i] = NiObject::create(blockName, i, mNiStream.get(), *this, mBuildData);
#else
            // For TES3, the object type string is read first to determine the type.
            mNiObjects[i] = NiObject::create(mNiStream->readString(), i, mNiStream.get(), *this, mBuildData);
#endif
        }
    }

    // TODO: should assert that the first object, i.e. mNiObjects[0], is either a NiNode (TES3/TES4)
    //       or BSFadeNode (TES5)

    // read the footer to check for root nodes
    std::uint32_t numRoots = 0;
    mNiStream->read(numRoots);

    mRoots.resize(numRoots);
    for (std::uint32_t i = 0; i < numRoots; ++i)
        mNiStream->read(mRoots.at(i));

    if (numRoots == 0)
        throw std::runtime_error(mModelName + " has no roots");
    else if (numRoots > 1) // FIXME: debugging only, to find out which NIF has multiple roots
        //throw std::runtime_error(mModelName + " has too many roots");
        // Creatures\Bear\forward.kf had 3 roots
        std::cout << "NOTE: " << mModelName << " has " << numRoots << " numRoots." << std::endl;

    if (numRoots == 1)
    {
        std::string type = blockType(mRoots[0]);
        if (type == "NiTriStrips" || type == "NiTriShape")
        {
            // HACK: assume we added a dummy NiNode
            std::size_t i = 0;
            for (; i < mNiObjects.size(); ++i)
                if (blockType(std::uint32_t(i)) == "NiNode")
                    break;

            if (mCurrIndex < int(mNiObjects.size()-1))
                mCurrIndex = int(mNiObjects.size()-1); // cheat

            mRoots[0] = std::uint32_t(i);
        }

        mRootNode = getRef<NiNode>(mRoots[0]);
    }
}

// find the bones, if any (i.e. prepare for the skeleton loader)
void NiBtOgre::NiModel::findBoneNodes(bool buildObjectPalette, size_t rootIndex)
{
    if (!mBuildData.needsSkeletonBuilt()) // TODO: should we allow skeleton with a single bone?
        return;

    const std::vector<NiNodeRef>& leafIndices = mBuildData.getBoneTreeLeafIndices();

    // find the bones
    int32_t leafIndex;
    NiNode *leafNode;
    mBoneRootNode = getRef<NiNode>(mRoots[rootIndex]);
    for (std::size_t i = 0; i < leafIndices.size(); ++i)
    {
        leafIndex = leafIndices[i];
        std::string nodeType = blockType(leafIndex);
        // TES4 dungeons\ayleidruins\exterior\arwellgrate01.nif has a NiBillboardNode target
        if (nodeType != "NiNode" && nodeType != "NiBillboardNode" && nodeType != "BSFadeNode")
            continue; // FIXME: morroblivion\flora\bushes\corkbulb01anim.nif, index 0x20

        leafNode = getRef<NiNode>(leafIndex);
        NiNodeRef boneRoot = leafNode->findBones(mRoots[rootIndex]); // populate NiNode::mChildBoneNodes

        // is skeleton root is different to scene root?  (-1 means we stopped midway so ignore)
        if (boneRoot != -1 && boneRoot != (std::int32_t)rootIndex)
            mBoneRootNode = getRef<NiNode>(boneRoot);

        if (buildObjectPalette)
            mObjectPalette[leafNode->getName()] = leafIndex;  // note: overwritten if same leaf node
    }
}

void NiBtOgre::NiModel::setSkinTexture(const std::string& texture)
{
    // see if we built the meshes already
    if (!mMeshes.empty())
        throw std::logic_error("NiModel: too late to set the texture to build the mesh");

    // do it for all meshes just in case - unfortunately can't predict which ones will need it
    std::map<NiNodeRef, NiNode*>::iterator iter;
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
        iter->second->setSkinTexture(texture);
}

// There are several types of NIFs that have more than one Mesh/Entity:
//
// 1. Skinned with a Skeleton
//    e.g. Clutter\UpperClass\UpperClassDisplayCasePurple01.NIF
//
//    (NOTE: some skinned NIFs do not have more than one mesh/entity
//     e.g. clothes\robemage\m\robemage_m.gnd)
//
// 2. Ragdoll
//    These require an additional Ogre::SceneNode per Entity.
//
//    e.g. Architecture\Cathedral\Crypt\CathedralCryptLight02.NIF
//    e.g. Clutter\FightersGuild\TargetHeavy01.NIF
//
// 3. More than one mesh/entity for no particular reason
//    Must remember to move all the related entities for any havok objects.
//
//    e.g. Weapons\Steel\Longsword.NIF
//
// FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
// Or group should come from the classes, e.g. static, misc, furniture, etc
void NiBtOgre::NiModel::createMesh(bool isMorphed, Ogre::SkeletonPtr suppliedSkeleton)
{
    Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();
    NiMeshLoader& meshLoader = NiModelManager::getSingleton().meshLoader();

    // make the mesh names unique
    //
    // * the same NIF can be used with different externally supplied skeletons
    // * morphed vertices and textures of the same NIF can be used for many NPCs
    std::string suppliedSkelName = "";
    if (suppliedSkeleton)
        suppliedSkelName = boost::to_lower_copy(suppliedSkeleton->getName());

    std::string modelName;
    if (isMorphed || !suppliedSkeleton)
        modelName = getName(); // WARN: morphed must have Npc::mEditorId + "_" in getName()
    else
        modelName = suppliedSkelName + "_" + getName(); // getName() should return NIF name

    // iterate through the mesh build map
    //
    // NOTE: If the model/object is static, we only need one child scenenode from the
    // basenode.  Else we need one for each NiNode that has a mesh (and collision shape?).
    // FIXME: how to do this?
    std::map<NiNodeRef, NiNode*>::iterator iter;
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
    {
        // skeleton name: only used for skinned meshes (same mesh used for different skeletons)
        //   e.g. Armor\Thief\M\Boots.NIF, "COW "Tamriel" 5 11" (TES4)
        //   ShadySam (Breton, skeleton.nif) & DeadArgonianAgent (Argonian, SkeletonBeast.NIF)
        //
        // The model name and parent node name are concatenated for use with Ogre::MeshManager
        // without triggering exeptions due to duplicates.
        // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif%ICWallTower01
        //
        // NiNode index: some NIF files have the NiNode name for different nodes in the same NIF
        //   e.g. Architecture\Solitude\SolitudeBase.nif (TES5)
        //
        // NiNode name: technically not required but useful for debugging
        //
        // FIXME: failsafe - check if mParent->getNodeName() returns blank, in which case use block number?
        // FIXME: consider the use of a hash (possibly the same as BSA) + block number for performance
        std::string meshName = modelName +
                               "#" + std::to_string(iter->second->selfRef()) + // node index
                               "%" + iter->second->getName();            // node name

        Ogre::MeshPtr mesh = meshManager.getByName(/*boost::algorithm::to_lower_copy(meshName)*/meshName, mGroup);
        if (!mesh)
            mesh = meshLoader.createMesh(meshName, mGroup, this, iter->second->selfRef(), suppliedSkelName);

        if (suppliedSkeleton)
            mesh->setSkeletonName(suppliedSkelName);
        else if (mBuildData.isSkeletonTES4() && mSkeleton) // FIXME: FO3, etc, but not yet tested
            mesh->setSkeletonName(mSkeleton->getName()); // Storm Atronach skeleton.nif has its own mesh
        else if (mSkeleton && getName() == "meshes\\characters\\_male\\skeleton.nif") // FIXME: FO3
            mesh->setSkeletonName(mSkeleton->getName());
#if 0
        // FIXME: testing VGearDoor01.NIF
        if (mSkeleton && !mesh->hasSkeleton() && getName().find("geardoor") != std::string::npos)
        {
            mesh->setSkeletonName(getName());
        }
#endif
        mesh->setAutoBuildEdgeLists(false);

        mMeshes.push_back(std::make_pair(mesh, iter->second));
    }

    // Create a dummy mesh if the root node does not have a mesh (and hence an Entity) to support
    // the current implementation of MWRender::Animation, since it requires an entity at the root
    // of the skeleton model. (see mObjectRoot->mSkelBase i.e. NifOgre::ObjectScene::mSkelBase)
    //
    // Most Skeleton.nif doesn't have any NiTriBasedGeom, and even Storm Atronach's skeleton
    // doesn't have one at the root node.
    //
    // e.g. Characters\_Male\Skeleton.nif (no mesh)
    // e.g. Creatures\StormAtronach\Skeleton.nif (also has normal mesh)
    //
    if (mBuildData.isSkeletonTES4() // FIXME: may also be required for others, but not yet tested
        || ((getName().find("skeleton.nif") != std::string::npos) && // FIXME: FO3
            //getName() == mNif) // HACK: FO3 avoid being triggerd by skinned model with skeleton name in front
            getRef<NiObjectNET>(getRootIndex())->hasIntegerExtraData("SkeletonID")) // FO3 triggers too often on its own :-(
        || (mSkeleton)// && !mBuildData.getAnimNodesMap().empty()) // FIXME testing new door code
        )
    {
        // FIXME: despite the name, maybe this should be "Scene Root" or "BSFadeNode" instead?
        // mModelName != "meshes\\morroblivion\\creatures\\wildlife\\kagouti\\skeleton.nif") // FIXME
#if 0
        NiNode * rootNode = getRootNode(); // WARN: maybe nullptr if there are multiple roots!
#else
        NiNode * rootNode = getSkeletonRoot();
#endif
        NiNodeRef rootIndex = rootNode->selfRef();

        // only if it has not been built already
        if (mBuildData.mMeshBuildList.find(rootIndex) == mBuildData.mMeshBuildList.end())
        {
            std::string meshName = getName() + "#0%" + rootNode->getName();
            Ogre::MeshPtr mesh = meshManager.getByName(meshName, "General");
            if (!mesh)
                mesh = meshLoader.createMesh(meshName, mGroup, this, rootIndex);

            if (suppliedSkeleton)
                mesh->setSkeletonName(suppliedSkelName);
            else if (mSkeleton)
                mesh->setSkeletonName(mSkeleton->getName());

            mMeshes.push_back(std::make_pair(mesh, rootNode));
        }
    }
}

// for building body parts
void NiBtOgre::NiModel::buildSkinnedModel(Ogre::SkeletonPtr suppliedSkeleton)
{
    if (mSkeleton && suppliedSkeleton)
        throw std::logic_error("NiModel: overwriting its own skeleton");

    // for building body parts use the supplied skeleton
    // but don't overwrite an existing one (for Storm Atronach)
    if (!mSkeleton)
        mSkeleton = suppliedSkeleton;

    mNiObjects[getRootIndex()]->build(&mBuildData); // FIXME: what to do with other roots?
}

// build the skeleton and node controllers
void NiBtOgre::NiModel::buildModel()
{
    mNiObjects[getRootIndex()]->build(&mBuildData); // FIXME: what to do with other roots?
}

// NOTE: 'model' should be updated each time a weapon (e.g. bow) is equipped or unequipped
//       since that will affect the object palette.  But the object palette in its current form
//       returns object index based on a text string name.  The indices from another model
//       (e.g.  bow) won't be useful.  i.e. we need a different solution.
//       TODO: check if the object names can clash
void NiBtOgre::NiModel::buildAnimation(Ogre::Entity *skelBase, NiModelPtr model,
        std::multimap<float, std::string>& textKeys,
        std::vector<Ogre::Controller<Ogre::Real> >& controllers,
        NiModel *skeleton, NiModel *bow)
{
    getRef<NiControllerSequence>(getRootIndex())->build(skelBase, model, textKeys, controllers, *skeleton, skeleton->getObjectPalette());
}

// for activator animations (and maybe doors, too)
void NiBtOgre::NiModel::buildAnimation(Ogre::Entity *skelBase, NiModelPtr model,
        std::multimap<float, std::string>& textKeys,
        std::vector<Ogre::Controller<Ogre::Real> >& controllers,
        NiModel *skeleton, NiControllerSequence *animation)
{
    animation->build(skelBase, model, textKeys, controllers, *skeleton, skeleton->getObjectPalette());
}

void NiBtOgre::NiModel::getControllerSequenceMap(std::map<std::string, NiControllerSequence*>& result) const
{
    for (std::size_t i = 0; i < mNiObjects.size(); ++i)
    {
        // assumed there is only one; if not, the first one is used
        if (mNiHeader->blockType(i) == "NiControllerManager")
        {
            // get the map from the NiControllerManager
            getRef<NiControllerManager>(i)->getControllerSequenceMap(result);
        }
    }

    return; // FIXME: throw?
}

#if 0
void NiBtOgre::NiModel::createCollisionshapes()
{
    const std::map<std::int32_t, /*std::pair<std::string,*/ int32_t/*>*/ >& rigidBodies = nimodel->getBhkRigidBodyMap();
    std::map<std::int32_t, /*std::pair<std::string, */int32_t/*>*/ >::const_iterator iter(rigidBodies.begin());
    for (; iter != rigidBodies.end(); ++iter)
    {
        //if (iter->second/*.second*/ == -1)
            //continue;  // e.g. fire/firetorchlargesmoke.nif%DamageSphere
        // FIXME: check for phantom

        std::int32_t bhkRef = iter->second/*.second*/;
        bhkSerializable *bhk = nimodel->getRef<bhkSerializable>(bhkRef);
        std::int32_t targetRef = iter->first;
        NiAVObject *target = nimodel->getRef<NiAVObject>(targetRef);

        mTargetNames[targetRef] = nimodel->indexToString(target->getNameIndex());

        // expectation is that each target has only one bhkRigidBody
        if (mBtCollisionShapeMap.find(targetRef) != mBtCollisionShapeMap.end())
            throw std::logic_error("target name collision "+nimodel->indexToString(targetRef));


        // get the bullet shape with the target as a parameter
        // TODO: cloning pre-pade shape (e.g. bhkRigidBody via unique_ptr) may be faster?
        mBtCollisionShapeMap[targetRef] = std::make_pair(target->getWorldTransform(), bhk->getShape(*target));
    }
}
#endif
void NiBtOgre::NiModel::buildSkeleton(bool load)
{
    if (mSkeleton)
        return; // we may already have an externally supplied skeleton

    if (mBuildData.needsSkeletonBuilt())
    {
        mSkeleton = Ogre::SkeletonManager::getSingleton().getByName(getName(), mGroup);
        if (!mSkeleton)
        {
            NiSkeletonLoader& skeletonLoader = NiModelManager::getSingleton().skeletonLoader();
            mSkeleton = skeletonLoader.createSkeleton(getName(), mGroup, this);
        }

        if (load)
            mSkeleton->load();
    }
}

// for non-skinned parts
std::string NiBtOgre::NiModel::getTargetBone() const
{
    NiBtOgre::NiNode *rootNode = getRef<NiBtOgre::NiNode>(getRootIndex());
    return rootNode->getStringExtraData("Prn");
}

const std::vector<Ogre::Vector3>& NiBtOgre::NiModel::fgVertices() const
{
    return getUniqueNiTriBasedGeom()->getVertices(false/*morphed*/);
}

const std::vector<Ogre::Vector3>& NiBtOgre::NiModel::fgVerticesFO3(bool hat) const
{
    return getNiTriBasedGeom(hat)->getVertices(false/*morphed*/);
}

std::vector<Ogre::Vector3>& NiBtOgre::NiModel::fgMorphVertices()
{
    return getUniqueNiTriBasedGeom()->mMorphVertices;
}

std::vector<Ogre::Vector3>& NiBtOgre::NiModel::fgMorphVerticesFO3(bool hat)
{
    return getNiTriBasedGeom(hat)->mMorphVertices;
}

void NiBtOgre::NiModel::useFgMorphVertices()
{
    getUniqueNiTriBasedGeom()->mUseMorphed = true;
}

void NiBtOgre::NiModel::buildFgPoses(const FgLib::FgTri *tri, bool rotate)
{
    NiTriBasedGeom *subMesh = getUniqueNiTriBasedGeom(); // head models, etc, should all be unique

    std::vector<std::pair<Ogre::MeshPtr, NiNode*> >::iterator it = mMeshes.begin();
    for (; it != mMeshes.end(); ++it)
    {
        if (it->second == subMesh->mParent)
        {
            subMesh->buildFgPoses(it->first.get(), tri, rotate);
            return;
        }
    }
}

// WARN: returns the vertices from the first NiTriBasedGeom child of the root NiNode
//       (this can be used to get around the lack of TRI files for certain NIF models)
NiBtOgre::NiTriBasedGeom *NiBtOgre::NiModel::getUniqueNiTriBasedGeom() const
{
    if (mNiObjects.empty())
        throw std::logic_error("NiModel attempting to retrieve an object that is not yet built.");

    NiNode *ninode = getRef<NiNode>(getRootIndex());

    return ninode->getUniqueSubMeshChild();
}

NiBtOgre::NiTriBasedGeom *NiBtOgre::NiModel::getNiTriBasedGeom(bool hat) const
{
    if (mNiObjects.empty())
        throw std::logic_error("NiModel attempting to retrieve an object that is not yet built.");

    NiNode *ninode = getRef<NiNode>(getRootIndex());

    return ninode->getSubMeshChildFO3(hat);
}

void NiBtOgre::NiModel::fillSkinIndices(std::map<std::string, std::vector<std::size_t> >& skinIndicesMap) const
{
    std::vector<std::size_t> skinIndices;

    std::map<NiNodeRef, NiNode*>::const_iterator iter;
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
    {
        skinIndices.clear();
        iter->second->getSkinIndices(skinIndices);
        if (!skinIndices.empty())
            skinIndicesMap.insert(std::make_pair(iter->second->getName(), skinIndices));
    }
}

void NiBtOgre::NiModel::fillDismemberParts(std::map<std::int32_t, std::vector<std::vector<std::uint16_t> > >& dismemberMap) const
{
    std::vector<std::vector<std::uint16_t> > bodyParts;

    std::map<NiNodeRef, NiNode*>::const_iterator iter;
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
    {
        bodyParts.clear();
        iter->second->getDismemberParts(bodyParts);
        if (!bodyParts.empty())
            dismemberMap.insert(std::make_pair(iter->first, bodyParts));
    }
}

NiBtOgre::NiNode *NiBtOgre::NiModel::getSkeletonRoot()
{
    if (!mBoneRootNode)
    {
        // FIXME: not even sure why we're building skeletons for skinned models which uses
        // external supplied skeleton; anyway these don't have "UPB"
        if (mBuildData.isSkinnedModel()) // does it have an NiSkinInstance?
        {
            mBoneRootNode = getRef<NiNode>(mBuildData.getSkeletonRoot()); // use the one from NiSkinInstance

            return mBoneRootNode;
        }
    }

    return mBoneRootNode;
}

const Ogre::Quaternion NiBtOgre::NiModel::getBaseRotation() const
{
    return getRootNode()->getLocalRotation();
}

std::uint32_t NiBtOgre::NiModel::getRootIndex() const
{
    //if (getNumRootNodes() > 1) // FIXME: bear\forward.kf
        //throw std::logic_error("NiNode parent map: multiple parents");

    return mRoots[0];
}

const NiBtOgre::NiMultiTargetTransformController *NiBtOgre::NiModel::getNiMultiTargetTransformController() const
{
    for (std::size_t i = 0; i < mNiObjects.size(); ++i)
    {
        // assumed there is only one; if not, the first one is returned
        if (mNiHeader->blockType(i) == "NiMultiTargetTransformController")
            return getRef<NiMultiTargetTransformController>(i);
    }

    return nullptr;
}

void NiBtOgre::BuildData::setNiNodeParent(NiAVObjectRef child, NiNode *parent)
{
    //if (child == -1) // already checked in NiNode before calling this method
        //return;

    std::map<NiAVObjectRef, NiNode*>::iterator lb = mParentNiNodeMap.lower_bound(child);

    if (lb != mParentNiNodeMap.end() && !(mParentNiNodeMap.key_comp()(child, lb->first)))
    {
        if (lb->second != parent)
            throw std::logic_error("NiNode parent map: multiple parents");
        // else the same entry already there for some reason, ignore for now
    }
    else
        mParentNiNodeMap.insert(lb, std::make_pair(child, parent)); // None found, create one
}

NiBtOgre::NiNode *NiBtOgre::BuildData::getNiNodeParent(NiAVObjectRef child) const
{
    std::map<NiAVObjectRef, NiNode*>::const_iterator it = mParentNiNodeMap.find(child);
    if (it != mParentNiNodeMap.cend())
    {
        return it->second;
    }
    else
    {
        //throw std::logic_error("NiNode parent map: parent not found");
        std::cerr << mModel.getName() << " : NiNode parent not found - " << child << std::endl;
        // FIXME: it turns out that some parents have higher index than children (i.e. occurs
        // later in the file) - to fix this properly quite a bit a change will be required
        // - although it might be possible to post process before mParent and transforms are
        // required
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 128
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 130
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 202
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 215
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 217
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 132
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 134
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 208
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 218
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 220
        return mModel.getRef<NiNode>(0);
    }
}

void NiBtOgre::BuildData::addAnimBoneName(const std::string& anim, const std::string& bone)
{
    std::map<std::string, std::vector<std::string> >::iterator lb
        = mAnimNodesMap.lower_bound(anim);

    if (lb != mAnimNodesMap.end() && !(mAnimNodesMap.key_comp()(anim, lb->first)))
    {
        lb->second.push_back(bone);
    }
    else // None found, create one
    {
        mAnimNodesMap.insert(lb, std::make_pair(anim, std::vector<std::string> { bone }));
    }
}

#if 0
void NiBtOgre::BuildData::addNewSkelLeafIndex(NiNodeRef leaf)
{
    if (std::find(mBoneTreeLeafIndices.begin(), mBoneTreeLeafIndices.end(), leaf) == mBoneTreeLeafIndices.end())
        mBoneTreeLeafIndices.push_back(leaf);
}

bool NiBtOgre::BuildData::hasBoneLeaf(NiNodeRef leaf) const
{
     return std::find(mBoneTreeLeafIndices.begin(), mBoneTreeLeafIndices.end(), leaf) != mBoneTreeLeafIndices.end();
}
#endif
