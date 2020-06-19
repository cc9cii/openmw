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
#ifndef NIBTOGRE_NIDATA_H
#define NIBTOGRE_NIDATA_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>
#include <OgreQuaternion.h>
#include <OgreAnimationTrack.h>

#include "niobject.hpp"
#include "nistream.hpp" // FIXME: not sure why this is needed for Key<Ogre::Quaternion>::read()

// Based on NifTools/NifSkope/doc/index.html
//
//  ATextureRenderData
//      NiPixelData
//  BSMultiBound
//  BSMultiBoundData <------------------- /* typedef NiObject */
//      BSMultiBoundOBB
//  NiAVObjectPalette <------------------ /* typedef NiObject */
//      NiDefaultAVObjectPalette
//  BSShaderTextureSet
//  NiBSplineBasisData
//  NiBSplineData
//  NiBoolData
//  NiColorData
//  NiExtraData
//      BSBehaviorGraphExtraData
//      BSBoneLODExtraData
//      BSBound
//      BSDecalPlacementVectorExtraData
//      BSFurnitureMarker
//          BSFurnitureMarkerNode <------ /* typedef BSFurnitureMarker */
//      BSInvMarker
//      NiBinaryExtraData
//      NiBooleanExtraData
//      NiFloatExtraData
//      NiIntegerExtraData
//          BSXFlags <------------------- /* typedef NiIntegerExtraData */
//      NiStringExtraData
//      NiTextKeyExtraData
//      NiVertWeightsExtraData
//  NiFloatData
//  NiGeometryData
//      NiParticlesData
//          NiAutoNormalParticlesData <-- /* typedef NiParticlesData */
//          NiRotatingParticlesData
//              NiPSysData
//                  BSStripPSysData
//      NiTriBasedGeomData
//          NiTriShapeData
//          NiTriStripsData
//  NiKeyframeData
//      NiTransformData <---------------- /* typedef NiKeyframeData */
//  NiMorphData
//  NiPosData
//  NiSkinData
//  NiSkinInstance
//      BSDismemberSkinInstance // FO3
//  NiSkinPartition
//  NiStringPalette
//  NiUVData
//  NiVisData

namespace Ogre
{
    class AnimationTrack;
    class TimeIndex;
    class KeyFrame;
}

namespace NiBtOgre
{
    struct ATextureRenderData : public NiObject
    {
        typedef std::uint32_t PixelFormat;

        struct ChannelData
        {
            typedef std::uint32_t ChannelType;
            typedef std::uint32_t ChannelConvention;

            ChannelType mType;
            ChannelConvention mConvention;
            unsigned char bitsPerChannel;
            unsigned char unknownByte1;
        };

        struct MipMap
        {
            std::uint32_t width;
            std::uint32_t height;
            std::uint32_t offset;
        };

        PixelFormat mPixelFormat;
        std::uint32_t mRedMask;
        std::uint32_t mGreenMask;
        std::uint32_t mBlueMask;
        std::uint32_t mAlphaMask;
        unsigned char mBitsPerPixel;
        std::vector<unsigned char> mUnknown3Bytes;
        std::vector<unsigned char> mUnknown8Bytes;
        //unsigned char mFlags;               // only 20.0.0.4 (TES4) onwards
        //std::vector<ChannelData> mChannels; // only 20.0.0.4 (TES4) onwards
        NiPaletteRef mPaletteRef;
        std::uint32_t mNumMipmaps;
        std::uint32_t mBytesPerPixel;
        std::vector<MipMap> mMipmaps;

        ATextureRenderData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiPixelData : public ATextureRenderData
    {
        std::uint32_t mNumPixels;
        std::vector<unsigned char> mPixelData;

        NiPixelData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    class BSMultiBound : public NiObject
    {
        BSMultiBoundDataRef mDataRef;

    public:
        BSMultiBound(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiObject BSMultiBoundData; // Seen in NIF version 20.2.0.7

    // Seen in NIF version 20.2.0.7
    struct BSMultiBoundOBB : public BSMultiBoundData
    {
        Ogre::Vector3 mCenter;
        Ogre::Vector3 mSize;     // each axis
        Ogre::Matrix3 mRotation;

        BSMultiBoundOBB(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiObject NiAVObjectPalette;

    class NiAVObject;

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiDefaultAVObjectPalette : public NiAVObjectPalette
    {
#if 0
        struct AVObject
        {
            std::string name;
            // clutter/minotaurhead01.nif (TES4) shows that some of the Ptr refer to objects not yet
            // loaded.  Change to Ref instead.
            //NiAVObject *avObject; // Ptr
            NiAVObjectRef avObjectRef;
        };

        // unknown int here
        std::vector<AVObject> mObjs;
#endif
    private:
        std::map<std::string, NiAVObjectRef> mObjRefMap;

    public:
        NiDefaultAVObjectPalette(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        const std::map<std::string, NiAVObjectRef>& getObjectPalette() const { return mObjRefMap; }

        NiAVObjectRef getObjectRef(const std::string& name) const {
            std::map<std::string, NiAVObjectRef>::const_iterator it = mObjRefMap.find(name);
            if (it != mObjRefMap.cend())
                return it->second;
            else
                return -1;
        }
    };

    // Seen in NIF version 20.2.0.7
    struct BSShaderTextureSet : public NiObject
    {
        std::vector<std::string> mTextures;

        BSShaderTextureSet(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBSplineBasisData : public NiObject
    {
        std::uint32_t mNumControlPoints;

        NiBSplineBasisData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBSplineData : public NiObject
    {
        std::vector<float>        mFloatControlPoints;
        std::vector<std::int16_t> mShortControlPoints;

        NiBSplineData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // 1  LINEAR_KEY       Use linear interpolation.
    // 2  QUADRATIC_KEY    Use quadratic interpolation. Forward and back tangents will be stored.
    // 3  TBC_KEY          Use Tension Bias Continuity interpolation.
    //                     Tension, bias, and continuity will be stored.
    // 4  XYZ_ROTATION_KEY For use only with rotation data.
    //                     Separate X, Y, and Z keys will be stored instead of using quaternions.
    // 5  CONST_KEY        Step function. Used for visibility keys in NiBoolData.
    typedef std::uint32_t KeyType;

    template<typename T>
    struct Key
    {
        float time;
        T value;
        T forward;
        T backward;
        float tension;
        float bias;
        float continuity;

        void read(NiStream *stream, KeyType interpolation)
        {
            if (interpolation == 1 /* LINEAR */ || interpolation == 5 /* CONSTANT */)
            {
                stream->read(time);
                stream->read(value);
            }
            else if (interpolation == 2 /* QUADRATIC */)
            {
                stream->read(time);
                stream->read(value);
                stream->read(forward);
                stream->read(backward);
            }
            else if (interpolation == 3 /* TBC */)
            {
                stream->read(time);
                stream->read(value);
                stream->read(tension);
                stream->read(bias);
                stream->read(continuity);
            }
            else if (interpolation == 4 /* XYZ */)
            {
                //if ( numKeys != 1 ) // FIXME
                    //throw std::runtime_error("NiBtOgre::KeyGroup::XYZ Roatation"); // FIXME: better message
            }
            else if (interpolation == 7 /* ignore */)
            {
                // do nothing for now, have no idea what this is meant to do
            }
            else if (interpolation == 0)
            {
                // TES5 dungeons\nordic\levers\pullchain\norpullchain01.nif ??
                // FIXME: Morroblivion\Creatures\Kwama\KwamaForager\forward.kf
                throw std::runtime_error("NiBtOgre::Key::interpolation is 0"); // FIXME: better message
            }
            else // FIXME: Morrowblivion morroblivion\creatures\kwama\scrib\idle.kf
                throw std::runtime_error("NiBtOgre::Key::interpolation unknown value"); // FIXME: better message
        }
    };

    template<>
    inline void Key<Ogre::Quaternion>::read(NiStream *stream, KeyType interpolation)
    {
        stream->read(time);
        stream->read(value);
        if (interpolation == 3 /* TBC */)
        {
            stream->read(tension);
            stream->read(bias);
            stream->read(continuity);
        }
    }

    template<typename T>
    struct KeyGroup
    {
        KeyType interpolation;
        std::vector<Key<T> > keys;
        std::map<float, int> indexMap;

        void read(NiStream *stream)
        {
            interpolation = 0;

            std::uint32_t numKeys;
            stream->read(numKeys);
            if(numKeys == 0)
                return;

            keys.clear();

            stream->read(interpolation);

            for(unsigned int i = 0; i < numKeys; ++i)
            {
                Key<T> key;
                key.read(stream, interpolation);
                indexMap[key.time] = (int) keys.size();
                keys.push_back(key);
            }
        }
    };

    template<typename T>
    class AnimTrackInterpolator: public Ogre::AnimationTrack::Listener
    {
        KeyType mInterpolation;
        Key<T>  mKey;

        public:
            AnimTrackInterpolator<T>(const KeyType interpolation, const Key<T>& key)
                : mInterpolation(interpolation), mKey(key) {}

        virtual bool getInterpolatedKeyFrame(const Ogre::AnimationTrack *t, const Ogre::TimeIndex& timeIndex,
            Ogre::KeyFrame *kf);
    };

    template<> // for gcc
    bool AnimTrackInterpolator<float>::getInterpolatedKeyFrame(const Ogre::AnimationTrack *t,
        const Ogre::TimeIndex& timeIndex, Ogre::KeyFrame *kf);

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBoolData : public NiObject
    {
        KeyGroup<char> mData;

        NiBoolData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiColorData : public NiObject
    {
        KeyGroup<Ogre::Vector4> mData;

        NiColorData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiExtraData : public NiObject
    {
        StringIndex    mName;
        NiExtraDataRef mNextRef;

        NiExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    struct BSBehaviorGraphExtraData : public NiExtraData
    {
        StringIndex   mBehaviourGraphFile;
        unsigned char mControlBaseSkeleton;

        BSBehaviorGraphExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct BSBound : public NiExtraData
    {
        Ogre::Vector3 mCenter;
        Ogre::Vector3 mDimensions;

        BSBound(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    struct BSBoneLODExtraData : public NiExtraData
    {
        struct BoneLOD
        {
            std::uint32_t distance;
            StringIndex   boneName;
        };

        std::uint32_t   mBoneLODCount;
        std::vector<BoneLOD> mBoneLODInfo;

        BSBoneLODExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    struct BSDecalPlacementVectorExtraData : public NiExtraData
    {
        struct DecalVectorArray
        {
            std::vector<Ogre::Vector3> points;
            std::vector<Ogre::Vector3> normals;

        };

        float mUnknown1;
        std::vector<DecalVectorArray> mVectorBlocks;

        BSDecalPlacementVectorExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct BSFurnitureMarker : public NiExtraData
    {
        struct FurniturePosition
        {
            Ogre::Vector3 offset;
            // TES4
            std::uint16_t orientation;
            unsigned char posRef1;
            unsigned char posRef2;
            // TES5
            float         heading;
            std::uint16_t animationType;
            std::uint16_t entryProperties;
        };

        std::vector<FurniturePosition> mPositions;

        BSFurnitureMarker(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef BSFurnitureMarker BSFurnitureMarkerNode; // Seen in NIF version 20.2.0.7

    // Seen in NIF version 20.2.0.7
    struct BSInvMarker : public NiExtraData
    {
        std::uint16_t mRotationX;
        std::uint16_t mRotationY;
        std::uint16_t mRotationZ;
        float         mZoom;

        BSInvMarker(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBinaryExtraData : public NiExtraData
    {
        std::vector<char> mData;

        NiBinaryExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBooleanExtraData : public NiExtraData
    {
        unsigned char mBooleanData;

        NiBooleanExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    struct NiFloatExtraData : public NiExtraData
    {
        float mFloatData;

        NiFloatExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiIntegerExtraData : public NiExtraData
    {
        std::uint32_t mIntegerData;

        NiIntegerExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiIntegerExtraData BSXFlags; // Seen in NIF version 20.0.0.4, 20.0.0.5

    struct NiStringExtraData : public NiExtraData
    {
        StringIndex mStringData;

        NiStringExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiTextKeyExtraData : public NiExtraData
    {
        struct TextKey // FIXME: use template instead?
        {
            float time;
            StringIndex text;
        };
        std::vector<TextKey> mTextKeys;

        NiTextKeyExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiVertWeightsExtraData : public NiExtraData
    {
      //std::uint32_t mNumBytes;
      //std::vector<float> mWeight;

        NiVertWeightsExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiFloatData : public NiObject
    {
        KeyGroup<float> mData;

        NiFloatData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiGeometryData : public NiObject
    {
        std::uint16_t mNumVertices;
        std::uint16_t mBSMaxVertices;
        char          mKeepFlags;     // from 10.1.0.0
        char          mCompressFlags; // from 10.1.0.0

        std::uint16_t mNumUVSets;
        std::uint16_t mBSNumUVSets;

        std::vector<Ogre::Vector3> mVertices;
        std::vector<Ogre::Vector3> mNormals;
        std::vector<Ogre::Vector3> mTangents;   // from 10.1.0.0
        std::vector<Ogre::Vector3> mBitangents; // from 10.1.0.0
        Ogre::Vector3              mCenter;
        float                      mRadius;
        std::vector<Ogre::Vector4> mVertexColors;
        //std::vector<std::vector<TexCoord> > mUVSets; // FIXME
        std::vector<std::vector<Ogre::Vector2> > mUVSets;

        std::uint16_t mConsistencyFlags;        // from 10.0.1.0
        AbstractAdditionalGeometryDataRef mAdditionalDataRef; // from 20.0.0.4

        NiGeometryData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isNiPSysData = false);

    protected:
        bool mIsNiPSysData; // set true by NiPSysData
    };

    struct NiParticlesData : public NiGeometryData
    {
        std::uint16_t mNumParticles; // to 4.0.0.2

        float mParticleRadius; // to 10.0.1.0

      //bool hasRadii;             // from 10.1.0.0
        std::vector<float> mRadii; // from 10.1.0.0

        std::uint16_t      mNumActive;

      //bool hasSizes;
        std::vector<float> mSizes;

      //bool hasRotations;                        // from 10.0.1.0
        std::vector<Ogre::Quaternion> mRotations; // from 10.0.1.0

      //bool hasRotationAngles;                   // from 20.0.0.4
        std::vector<float>         mRotationAngles;
      //bool hasRotationAxes;                     // from 20.0.0.4
        std::vector<Ogre::Vector3> mRotationAxes; // from 20.0.0.4
      //bool hasUVQuads;
        std::vector<Ogre::Vector4> mUVQuadrants;

        NiParticlesData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isNiPSysData = false);
    };

    typedef NiParticlesData NiAutoNormalParticlesData;

    struct NiRotatingParticlesData : public NiParticlesData
    {
        std::vector<Ogre::Quaternion> mRotations2; // to 4.2.2.0

        NiRotatingParticlesData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isNiPSysData = false);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiPSysData : public NiRotatingParticlesData
    {
        struct ParticleDesc
        {
            Ogre::Vector3 translation;
          //std::vector<float> unknownFloats; // to 10.4.0.1
            Ogre::Vector3 unknownFloats; // to 10.4.0.1
            float unknown1;
            float unknown2;
            float unknown3;
            std::int32_t unknown;
        };

        std::vector<ParticleDesc> mParticleDescriptions;
        std::vector<float> mUnknownFloats3;
      //bool hasSubTextureUVs;
        float mAspectRatio;
        std::vector<Ogre::Vector4> mSubTextureOffsetUVs;

        NiPSysData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    struct BSStripPSysData : public NiPSysData
    {
        std::int16_t mUnknown5;
        char         mUnknown6;
        std::int32_t mUnknown7;
        float        mUnknown8;

        BSStripPSysData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiTriBasedGeomData : public NiGeometryData
    {
        std::uint16_t mNumTriangles;

        NiTriBasedGeomData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual const std::vector<std::uint16_t>& getTriangles() const = 0;
    };

    // NiTriShapeData, SkinPartition, hkTriangle
    // NOTE: But not used, these vertices are simply stored in a vector
    // (maybe except SkinPartition)
    struct Triangle
    {
        std::uint16_t v1;
        std::uint16_t v2;
        std::uint16_t v3;
    };

    struct NiTriShapeData : public NiTriBasedGeomData
    {
        std::vector<std::uint16_t> mTriangles; // vector of vertices rather than triangles

        NiTriShapeData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        const std::vector<std::uint16_t>& getTriangles() const { return mTriangles; }
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiTriStripsData : public NiTriBasedGeomData
    {
        std::vector<std::uint16_t>               mStripLengths; // FIXME: no need to keep?
        std::vector<std::vector<std::uint16_t> > mPoints;       // FIXME: no need to keep?

        // Ogre doesn't seem to allow meshes to be created using triangle strips
        // unless using ManualObject - just convert to trinagles for now
        std::vector<std::uint16_t> mTriangles; // vector of vertices rather than triangles

        NiTriStripsData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        const std::vector<std::uint16_t>& getTriangles() const { return mTriangles; }
    };

    template<typename KeyType>
    struct QuatKey
    {
        float time;
        KeyType value;
        float tension;
        float bias;
        float continuity;
    };

    struct NiKeyframeData : public NiObject
    {
        std::uint32_t mRotationType;

        // Key<T> and QuatKey<T> are close enough to use Key<T> instead (makes the code simpler
        // at the expense of additional memory usage for forward/backward)
        //std::vector<QuatKey<Ogre::Quaternion> > mQuaternionKeys;
        KeyGroup<Ogre::Quaternion> mQuaternionKeys;

        KeyGroup<float> mXRotations;
        KeyGroup<float> mYRotations;
        KeyGroup<float> mZRotations;

        KeyGroup<Ogre::Vector3> mTranslations;
        KeyGroup<float> mScales;

        NiKeyframeData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiKeyframeData NiTransformData; // Seen in NIF version 20.0.0.4, 20.0.0.5

    struct NiMorphData : public NiObject
    {
        struct Morph
        {
            StringIndex mFrameName;              // TES4
            std::uint32_t mInterpolation;        // TES3
            std::vector<Key<float> > mKeys;      // TES3
            std::vector<Ogre::Vector3> mVectors;
        };

        std::vector<Morph> mMorphs;

        NiMorphData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiPosData : public NiObject
    {
        KeyGroup<Ogre::Vector3> mData;

        NiPosData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiSkinData : public NiObject
    {
        struct SkinTransform // also used in NiSkinningMeshModifier
        {
            Ogre::Matrix3 rotation;
            Ogre::Vector3 translation;
            float scale;
        };

        struct SkinData
        {
            struct SkinWeight
            {
                std::uint16_t vertex;
                float         weight;
            };

            SkinTransform skinTransform;
            Ogre::Vector3 boundingSphereOffset;
            float         boundingSphereRadius;
            std::vector<SkinWeight> vertexWeights;

            void read(NiStream *stream, bool hasVertexWeights);
        };

        SkinTransform         mSkinTransform;
        NiSkinPartitionRef    mSkinPartitionRef;
        std::vector<SkinData> mBoneList;

        NiSkinData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    class NiNode;

    class NiSkinInstance : public NiObject
    {
    public:
        NiSkinDataRef         mDataRef;
        NiSkinPartitionRef    mSkinPartitionRef;
      //NiNodeRef             mSkeletonRootRef;
        NiNode               *mSkeletonRoot; // Ptr
        // imperial/headhuman.nif (TES4) shows that some of the Ptr refer to objects not yet
        // loaded.  Change to Ref instead.
        //std::vector<NiNode*>  mBones;        // Ptr
        std::vector<NiNodeRef>  mBoneRefs;

        NiSkinInstance(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    class BSDismemberSkinInstance : public NiSkinInstance
    {
    public:
        struct BodyPartList
        {
            std::uint16_t partFlag;
            std::uint16_t bodyPart;
        };

        std::vector<BodyPartList>  mPartitions;

        BSDismemberSkinInstance(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    class NiSkinPartition : public NiObject
    {
        struct SkinPartition
        {
            std::uint16_t numVertices;
            std::uint16_t numTriangles;
            std::uint16_t numBones;
            std::uint16_t numStrips;
            std::uint16_t numWeightsPerVertex;
            std::vector<std::uint16_t> bones;
            bool hasVertexMap;
            std::vector<std::uint16_t> vertexMap;
            bool hasVertexWeights;
            std::vector<std::vector<float> > vertexWeights;
            std::vector<std::uint16_t> stripLengths;
            bool hasFaces;
            std::vector<std::vector<std::uint16_t> > strips;
            std::vector<Triangle> triangles;
            bool hasBoneIndices;
            std::vector<std::vector<unsigned char> > boneIndices;

            void read(NiStream *stream);
        };

        std::uint32_t              mNumSkinPartitionBlocks;
        std::vector<SkinPartition> mSkinPartitionBlocks;

    public:
        NiSkinPartition(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiStringPalette : public NiObject
    {
        std::string   mPalette;
        std::uint32_t mLength;

        NiStringPalette(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiUVData : public NiObject
    {
        KeyGroup<float> mUVGroups0;
        KeyGroup<float> mUVGroups1;
        KeyGroup<float> mUVGroups2;
        KeyGroup<float> mUVGroups3;

        NiUVData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiVisData : public NiObject
    {
        std::vector<Key<char> > mKeys;

        NiVisData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };
}

#endif // NIBTOGRE_NIDATA_H
