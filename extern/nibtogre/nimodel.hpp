/*
  Copyright (C) 2015-2021 cc9cii

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
#ifndef NIBTOGRE_NIMODEL_H
#define NIBTOGRE_NIMODEL_H

#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <memory>

#include <btBulletCollisionCommon.h>

#include <OgreResource.h>
#include <OgreController.h>
#include <OgreQuaternion.h>

#include "nistream.hpp"
#include "niheader.hpp"
#include "niobject.hpp"
#include "nimodelmanager.hpp"

namespace Ogre
{
    class ResourceManager;
    class ManualResourceLoader;
    class SceneNode;
    class Skeleton;
    class SkeletonInstance;
}

namespace FgLib
{
    class FgTri;
}

namespace NiBtOgre
{
    class NiObject;
    struct NiTriBasedGeom;
    struct BtOgreInst;
    class NiModel;
    class NiNode;
    class NiControllerSequence;
    class NiMultiTargetTransformController;

    enum BuildFlags {
        Flag_EnableHavok         = 0x0001,
        Flag_EnableCollision     = 0x0002, // TES4 only?
        Flag_IsSkeleton          = 0x0004, // TES4 only?
        Flag_EnableAnimation     = 0x0008,
        Flag_FlameNodesPresent   = 0x0010, // TES4 only?
        Flag_EditorMarkerPresent = 0x0020,
        Flag_IgnoreEditorMarker  = 0x0040,
        Flag_HasSkin             = 0x0080,
        Flag_None                = 0xFFFF
    };

    struct BuildData
    {
    private:
        const NiModel& mModel;

        //       child index    parent
        //            |            |
        //            v            v
        std::map<NiAVObjectRef, NiNode*> mParentNiNodeMap;

        // During construction various NiObjects may indicate that it has bones.
        // These are then used as the starting points for NiNode::findBones which recursively
        // traverses till a skeleton root is found - the main objective is to filter out any
        // NiNodes that are not needed as bones (to minimise the number of bones).
        std::vector<NiNodeRef> mBoneTreeLeafIndices; // tempoarily used to find the bones

        //      block index of NiGeomMorpherController
        //               |       index of controlled block (corresponds to a NiMorphData::Morph)
        //               |                          |
        //               v                          v
        //std::map<NiTimeControllerRef, std::vector<int> > mGeomMorpherControllerMap;

        std::vector<NiNodeRef> mControllerTargetIndices; // for detecting dynamic collision shapes

        // Populated by NiControllerSequence (or NiMultiTargetTransformController).
        // The targets are usually NiNode or NiTriBasedGeom.
        // TODO: an example of NiTriBasedGeom target (it might be for vertex anim only)
        //
        //       animation name           NiAVObject target (bone) name
        //            |                        |
        //            v                        v
        std::map<std::string, std::vector<std::string> > mAnimNodesMap;

        NiNodeRef mSkeletonRoot; // a copy from NiSkinInstance

    public:

        inline NiNodeRef getSkeletonRoot() const { return mSkeletonRoot; }
        inline void setSkeletonRoot(NiNodeRef nodeRef) { mSkeletonRoot = nodeRef; }
        inline bool isSkinnedModel() const { return mSkeletonRoot != -1; }

        //bool mFlameNodesPresent;
        //bool mEditorMarkerPresent;

        int32_t mBuildFlags; // mainly from BSX flags
        inline bool havokEnabled()          const { return (mBuildFlags & Flag_EnableHavok)         != 0; }
        inline bool animEnabled()           const { return (mBuildFlags & Flag_EnableAnimation)     != 0; }
        inline bool isSkeletonTES4()        const { return (mBuildFlags & Flag_IsSkeleton)          != 0; }
        inline bool isSkeletonTES5()        const { return (mBuildFlags & Flag_IsSkeleton)          != 0; }
        inline bool flameNodesPresentTES4() const { return (mBuildFlags & Flag_FlameNodesPresent)   != 0; }
        inline bool editorMarkerPresent()   const { return (mBuildFlags & Flag_EditorMarkerPresent) != 0; }

        // Helper methods to get access to the parent NiNode.
        // WARN: Fragile - relies on the parent blocks in a NIF being present before the child blocks
        //       which isn't always true (e.g. TES4 architecture\arena\arenaspectatorm01.nif)
        // TODO: Save these special cases then post proces them before the children accesses mParent
        NiNode *getNiNodeParent(NiAVObjectRef child) const; // used by NiNode and NiGeometry ctor
        void setNiNodeParent(NiAVObjectRef child, NiNode *parent); // called by NiNode ctor

        std::map<NiNodeRef, NiNode*> mMeshBuildList;

        // WARN: Adds without checking if it exists already.
        //       NiSkinInstance - bone refs
        //       NiNode - flame nodes, attach light
        //       NiKeyframeController - target refs
        //       NiTimeController - initial target ref
        //       NiMultiTargetTransformController - extra target refs
        //       (NiTriBasedGeom - hack for testing animation of sub-mesh)
        inline void addBoneTreeLeafIndex(NiNodeRef leaf) { mBoneTreeLeafIndices.push_back(leaf); }
        inline bool needsSkeletonBuilt() const { return mBoneTreeLeafIndices.size() > 1; }
        inline const std::vector<NiNodeRef>& getBoneTreeLeafIndices() const { return mBoneTreeLeafIndices; }

        // only adds if none found
        //void addNewSkelLeafIndex(NiNodeRef leaf); // FIXME: not used?
        //bool hasBoneLeaf(NiNodeRef leaf) const;   // FIXME: not used?

        // called by NiTimeController and NiMultiTargetTransformController
        // TODO: this results in a subset of mBoneTreeLeafIndices - is there a way to combine?
        inline void addControllerTargetIndex(NiNodeRef target) { mControllerTargetIndices.push_back(target); }
        inline const std::vector<NiNodeRef>&  getControllerTargets() const { return mControllerTargetIndices; }

        // called by NiMultiTargetTransformController::build() and NiControllerSequence::buildFO3()
        // TODO: seems too similar to ControllerTargetIndices except keyed by anim name?
        void addAnimBoneName(const std::string& anim, const std::string& bone);

        inline bool hasNodeAnimation() const { return !mAnimNodesMap.empty(); }
        inline const std::map<std::string, std::vector<std::string> >&
            getAnimNodesMap() const { return mAnimNodesMap; }

        // The btCollisionShape for btRigidBody corresponds to an Ogre::Entity whose Ogre::SceneNode
        // may be controlled for Ragdoll animations.  So we just really need the NiModel name,
        // NiNode name (and maybe the NiNode block index) to be able to load the required info.
        //
        // Once the NiModel is "built", all the required info are ready to be simply collected
        // and used.
        //
        //      index = parent NiNode's block index
        //        |
        //        |                  name  = concatenation of model, "%" and parent NiNode name
        //        |                    |
        //        v                    v
        std::map<NiNodeRef, /*std::pair<std::string,*/ int32_t/*>*/ > mBhkRigidBodyMap;
        //std::vector<std::pair<bhkConstraint*, bhkEntity*> > mBhkConstraints;
        bool mHasBhkConstraint;
        inline bool hasBhkConstraint() const { return mHasBhkConstraint; }

        // FIXME: these needs to be vectors (there can be multiple) and the flame node editor
        // id needs to be extracted (without '@#N' where N is a number)
        std::vector<NiNode*> mFlameNodes;
        std::vector<NiNode*> mAttachLights;

        std::multimap<float, std::string> mTextKeys;
        std::vector<Ogre::Controller<Ogre::Real> > mControllers;

        int mFlags; // some global properties

        BuildData(const NiModel& model)
            : mModel(model), mSkeletonRoot(-1)
            , /*mFlameNodesPresent(false), mEditorMarkerPresent(false) ,*/ mBuildFlags(0)
            , mHasBhkConstraint(false)
            , mFlags(0)
        {
        }
    };

    //
    //   +--& NiModel o--- NiStream
    //   |    o o o o        o o
    //   |    | | | |        | +-- NIF version numbers (local copy)
    //   |    | | | |        *
    //   |    | | | +----- Header
    //   |    | | |         o o
    //   |    | | |         | +--- NIF version numbers
    //   |    | | |         +----- vector<string>
    //   |    | | |
    //   |    | | +--- ObjectPalatte
    //   |    | |
    //   |    | +--- BuildData &--+   (access during construction)
    //   |    |                   o
    //   |    +------- vector<NiObject*>
    //   |                      o
    //   +----------------------+
    //
    // NOTE: NiStream has a pointer to Header for the appending long strings (TES3/TES4)
    //       NiObject has a reference to NiModel for getting strings and other NiObject Ptrs/Refs
    //
    //       BuildData is passed to NiObject to capture useful data during construction
    //       (this reduces the need to scan the objects later)
    //
    //       FIXME: skeleton should be here along with ObjectPalatte?
    //
    //       ObjectPalatte is used by another NiModel to build animation based on targets
    //       (bones) in this NiModel
    //
    class NiModel : public Ogre::Resource
    {
        std::unique_ptr<NiStream> mNiStream; // NOTE: the initialisation order of NiStream and NiHeader
        std::unique_ptr<NiHeader> mNiHeader;

        const std::string mGroup;  // Ogre group

        std::vector<std::unique_ptr<NiObject> > mNiObjects;
        std::vector<std::uint32_t> mRoots;

        int mCurrIndex; // FIXME: for debugging Ptr

        std::string mModelName;
        std::string mNif; // store NIF path for prepareImpl (may be different to mModelName)

        NiNode *mRootNode; // convenience copy (NOTE: only valid if there is only one)
        NiNode *mBoneRootNode; // convenience copy

        BuildData mBuildData;
        Ogre::SkeletonPtr mSkeleton;
        std::vector<std::pair<Ogre::MeshPtr, NiNode*> > mMeshes;

        //      target NiNode ref     NiNode world transform
        //              |                   |
        //              v                   v
        std::map<std::int32_t, std::pair<Ogre::Matrix4, btCollisionShape *> > mBtCollisionShapeMap;

        bool mShowEditorMarkers; // usually only for OpenCS

        // for skeleton.nif, etc; key is the node name
        std::map<std::string, NiAVObjectRef> mObjectPalette;

        // default, copy and assignment not allowed
        NiModel();
        NiModel(const NiModel& other);
        NiModel& operator=(const NiModel& other);

    protected:
        virtual void prepareImpl();
        virtual void unprepareImpl();
        virtual void loadImpl();   // called by Ogre::Resource::load()
        virtual void unloadImpl(); // called by Ogre::Resource::unload()
        virtual void preLoadImpl();   // called by Ogre::Resource::load()
        virtual void preUnloadImpl(); // called by Ogre::Resource::unload()

    public:
        // The parameter 'nif' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        NiModel(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                const Ogre::String& nif/*, bool showEditorMarkers=false*/);
        ~NiModel();

        const std::string& getOgreGroup() const { return mGroup; }
        const std::string& getName() const { return mModelName; }

        template<class T>
        inline T *getRef(std::int32_t index) const {
#if 0
            try {
                return static_cast<T*>(mNiObjects[index].get());
            }
            catch (...) {
                return nullptr;
            }
#else
            if (index >= 0 && (index > mCurrIndex)) // FIXME: for debugging Ptr
                throw std::runtime_error("Ptr");

            return (index < 0) ? nullptr : static_cast<T*>(mNiObjects[index].get());
#endif
        }

        // returns NiObject type name
        const std::string& blockType(std::uint32_t index) const {
            return mNiHeader->blockType(index);
        }

        // needed for the NIF version and strings (TES5)
        inline std::uint32_t nifVer() const { return mNiHeader->nifVer(); }

        inline const std::string& indexToString(std::int32_t index) const {
            return mNiHeader->indexToString(index);
        }

        inline std::int32_t searchStrings(const std::string& str) const {
            return mNiHeader->searchStrings(str);
        }

        inline bool hideEditorMarkers() const { return !mShowEditorMarkers; }

        NiNode *getSkeletonRoot();          // returns nullptr if none found

        inline const NiNode *getRootNode() const { return mRootNode; }; // returns the root NiNode of the model
        std::uint32_t getRootIndex() const; // WARN: will throw if there are more than one
        inline std::size_t getNumRootNodes() const { return mRoots.size(); }

        typedef std::int32_t NiNodeRef;
        const std::map<NiNodeRef, /*std::pair<std::string,*/ int32_t/*>*/ >&
            getBhkRigidBodyMap() const { return mBuildData.mBhkRigidBodyMap; }

        const std::multimap<float, std::string>& getTextKeys() const { return mBuildData.mTextKeys; }
        const std::vector<Ogre::Controller<Ogre::Real> >& getControllers() const { return mBuildData.mControllers; }

        //const std::map<NiNodeRef, int32_t>& getBhkRigidBodyMap() const { return mBuildData.mBhkRigidBocyMap; }

        void buildSkinnedModel(Ogre::SkeletonPtr skeleton = Ogre::SkeletonPtr());

        void buildModel();

        //void createCollisionshapes();

        void buildAnimation(Ogre::Entity *skelBase, NiModelPtr model,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<Ogre::Real> >& controllers,
                NiModel *skeleton,
                NiModel *bow = nullptr);

        void buildAnimation(Ogre::Entity *skelBase, NiModelPtr model,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<Ogre::Real> >& controllers,
                NiModel *skeleton,
                NiControllerSequence *animation);

        void getControllerSequenceMap(std::map<std::string, NiControllerSequence*>& result) const;

        const std::map<std::string, NiAVObjectRef>& getObjectPalette() const { return mObjectPalette; }

        Ogre::SkeletonPtr getSkeleton() const { return mSkeleton; }
        inline bool hasSkeleton() const { return mSkeleton.get() != nullptr; }
        void buildSkeleton(bool load = false);

        inline const std::vector<std::pair<Ogre::MeshPtr, NiNode*> >& getMeshes() const { return mMeshes; }

        void createNiObjects();

        // will throw if built the mesh already
        void setSkinTexture(const std::string& texture);

        // for replacing visible skin textures
        // mesh name and sub-mesh indices
        std::map<std::string, std::vector<std::size_t> > mSkinIndices;

        // fills a map of NiTriBasedGeom name and visible skin sub-mesh indices
        void fillSkinIndices(std::map<std::string, std::vector<std::size_t> >&) const;

        void fillDismemberParts(std::map<std::int32_t, std::vector<std::vector<std::uint16_t> > >&) const;

        // supply skeleton for skinned objects
        void createMesh(bool isMorphed = false, Ogre::SkeletonPtr skeleton = Ogre::SkeletonPtr());

        inline const BuildData& buildData() const { return mBuildData; }

        // used to generate a FaceGen TRI file, returns the vertices of the first NiTriBasedGeom
        const std::vector<Ogre::Vector3>& fgVertices() const;
        const std::vector<Ogre::Vector3>& fgVerticesFO3(bool hat) const;

        // used by FgSam to populate the morphed vertices
        std::vector<Ogre::Vector3>& fgMorphVertices();
        std::vector<Ogre::Vector3>& fgMorphVerticesFO3(bool hat);

        void useFgMorphVertices(); // used by FgSam to indicate vertices have been morphed

        void buildFgPoses(const FgLib::FgTri *tri, bool rotate = false);

        std::string getTargetBone() const;

        void findBoneNodes(bool buildObjectPalette = false, std::size_t rootIndex = 0);

        const Ogre::Quaternion getBaseRotation() const;

        template<class T>
        T *insertDummyBlock(const std::string& blockType); // for landscape LOD meshes
        inline std::uint32_t addString(const std::string& str) { return mNiHeader->addString(str); }

        const NiMultiTargetTransformController *getNiMultiTargetTransformController() const;

    private:

        // access to NiGeometryData for generating a FaceGen TRI file or to populate morphed vertices
        // WARN: may throw
        NiTriBasedGeom *getUniqueNiTriBasedGeom() const;
        NiTriBasedGeom *getNiTriBasedGeom(bool hat) const;
    };

    template<typename T>
    T *NiModel::insertDummyBlock(const std::string& blockType)
    {
        // do this before adding the new block to get the right index
        std::uint32_t nextBlock = mNiHeader->numBlocks();

        // update NiHeader (mBlockTypes, mBlockTypeIndex, mStrings)
        mNiHeader->addBlockType(blockType);

        // insert the dummy
        mNiObjects.resize(nextBlock+1);
        mNiObjects.at(nextBlock) = NiObject::create(blockType, nextBlock, nullptr, *this, mBuildData);

        return static_cast<T*>(mNiObjects[nextBlock].get());
    }
}

#endif // NIBTOGRE_NIMODEL_H
