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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#ifndef NIBTOGRE_NINODE_H
#define NIBTOGRE_NINODE_H

#include <OgreVector4.h>

#include "niavobject.hpp"

namespace Ogre
{
    class Skeleton;
    class Bone;
    class Mesh;
    class Vector3;
}

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiAVObject
//         NiNode
//             AvoidNode <------------------ /* typedef NiNode */
//             BSBlastNode
//             BSRangeNode
//             BSDebrisNode
//             BSDamageStage
//             BSFadeNode <----------------- /* typedef NiNode */
//             BSLeafAnimNode <------------- /* typedef NiNode */
//             BSMultiBoundNode
//             BSOrderedNode
//             BSTreeNode
//             BSValueNode
//             NiBillboardNode
//             NiBSAnimationNode <--------- /* typedef NiNode */
//             NiBSParticleNode <---------- /* typedef NiNode */
//             NiSwitchNode
//             RootCollisionNode <--------- /* typedef NiNode */
namespace NiBtOgre
{
    class NiStream;
    struct NiTriBasedGeom;

    class NiNode : public NiAVObject
    {
        std::string mNodeName; // cached here since used frequently
        NiNode *mParent;       // cached here since used frequently
        BuildData& mData;
        NiNode *mAnimRoot; // FIXME: used for children of animated nodes

        std::string mSkinTexture;

        std::vector<NiTriBasedGeom*> mSubMeshChildren;

        //void buildTES3(Ogre::SceneNode *sceneNode, BtOgreInst *inst, NiObject *parentNiNode = nullptr);

        NiNodeRef findBones(const NiNodeRef skeletonRoot, const NiNodeRef childNode);

        bool isBSBone(const NiNode *node) const;
        std::string getBoneLOD(const NiNode& node) const;

    protected:
        std::vector<NiAVObjectRef>      mChildren;
        std::vector<NiDynamicEffectRef> mEffects;
        std::vector<NiNodeRef> mChildBoneNodes; // protected for NiSwitchNode

    public:
        NiNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~NiNode() {};

        // It seems that for TES4 only NiNodes/NiBillboardNode are root nodes?
        virtual void build(BuildData *data, NiObject *parentNiNode = nullptr);

        // For NiGeometry children (e.g. NiTriStrips)
        virtual const std::string& getName() const { return mNodeName; }
        const NiNode& getParentNiNode() const { return *mParent; } // not used?

        //
        virtual void registerSubMesh(NiTriBasedGeom* geom);
        virtual void buildMesh(Ogre::Mesh* mesh);

        //
        NiNodeRef findBones(std::int32_t rootIndex);

        void addBones(Ogre::Skeleton *skeleton,
                Ogre::Bone *parentBone, std::map<std::uint32_t, std::uint16_t>& indexToHandle);
        void addAllBones(Ogre::Skeleton *skeleton, Ogre::Bone *parentBone);
        void buildObjectPalette(std::map<std::string, NiAVObjectRef>& objectPalette, bool first = false);

        const std::vector<NiAVObjectRef>& getNiNodeChildren() const { return mChildren; }

        NiTriBasedGeom *getUniqueSubMeshChild(); // WARN: may throw
        NiTriBasedGeom *getSubMeshChildFO3(bool hat); // WARN: may throw

        std::size_t getNumSubMeshChildren() const { return mSubMeshChildren.size(); }

        void setSkinTexture(const std::string& texture) { mSkinTexture = texture; }
        inline const std::string& getSkinTexture() const { return mSkinTexture; }

        void getSkinIndices(std::vector<std::size_t>& skinIndices) const;
        void getDismemberParts(std::vector<std::vector<std::uint16_t> >& bodyParts) const;

        // an attempt to fix the head/ear/eyes rotation issue
        const Ogre::Quaternion getLocalRotation() const { return Ogre::Quaternion(NiAVObject::mRotation); }

        bool isDynamicMesh(NiNodeRef *nodeRef) const;
        const void getTransform(NiNodeRef nodeRef, Ogre::Matrix4& transform, bool nodeOffset = false) const;
        NiNode *getAnimRoot() const { return mAnimRoot; }
        void setAnimRoot(NiNodeRef nodeRef);
    };

    typedef NiNode AvoidNode;

    // Seen in NIF version 20.2.0.7
    class BSBlastNode : public NiNode
    {
        char mUnknown1;
        short mUnknown2;

    public:
        BSBlastNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSBlastNode() {};

        //void build(NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSDamageStage : public NiNode
    {
        char mUnknown1;
        std::int16_t mUnknown2;

    public:
        BSDamageStage(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSDamageStage() {};

        //void build(NiObject *parentNiNode = nullptr);
    };

    class BSRangeNode : public NiNode
    {
        std::uint8_t mMin;
        std::uint8_t mMax;
        std::uint8_t mCurrent;

    public:
        BSRangeNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef BSRangeNode BSDebrisNode;

    //typedef NiNode BSFadeNode; // Seen in NIF version 20.2.0.7
    class BSFadeNode : public NiNode
    {
    public:
        BSFadeNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSFadeNode() {};
    };

    typedef NiNode BSLeafAnimNode; // Seen in NIF version 20.2.0.7

    // Seen in NIF version 20.2.0.7
    class BSMasterParticleSystem : public NiNode
    {
    public:
        std::uint16_t mMaxEmitterObjects;
        std::int32_t  mNumParticleSystems;
        NiAVObjectRef mParticleSystemsRef;

        BSMasterParticleSystem(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    class BSMultiBoundNode : public NiNode
    {
        BSMultiBoundRef mMultiBoundRef;
        std::uint32_t mUnknown; // from 20.2.0.7

    public:
        BSMultiBoundNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSMultiBoundNode() {};

        //void build(NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSOrderedNode : public NiNode
    {
        Ogre::Vector4 mAlphaSortBound;
        unsigned char mIsStaticBound;

    public:
        BSOrderedNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSOrderedNode() {};

        //virtual void build(NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSTreeNode : public NiNode
    {
        std::vector<NiNodeRef> mBones1;
        std::vector<NiNodeRef> mBones2;

    public:
        BSTreeNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSTreeNode() {};

        //void build(NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSValueNode : public NiNode
    {
        std::int32_t mValue;

    public:
        BSValueNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~BSValueNode() {};

        //void build(NiObject *parentNiNode = nullptr);
    };

    struct NiBillboardNode : public NiNode
    {
        std::uint16_t mBillboardMode;

    public:
        NiBillboardNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~NiBillboardNode() {};

        //void build(NiObject *parentNiNode = nullptr);
    };

    typedef NiNode NiBSAnimationNode;
    typedef NiNode NiBSParticleNode;

    // Seen in NIF version 20.2.0.7
    class NiSwitchNode : public NiNode
    {
        std::uint16_t mNiSwitchFlags; // 1 = update only active child, 2 = update controllers
        std::int32_t mIndex;

    public:
        NiSwitchNode(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        void addBones(Ogre::Skeleton *skeleton,
                Ogre::Bone *parentBone, std::map<std::uint32_t, std::uint16_t>& indexToHandle);
    };

    typedef NiNode RootCollisionNode;
}

#endif // NIBTOGRE_NINODE_H
