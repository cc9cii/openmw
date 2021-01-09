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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#include "nidata.hpp"

#include <cassert>
#include <stdexcept>

#include <OgreAnimationTrack.h>
#include <OgreKeyFrame.h>

#include "nistream.hpp"
#include "ninode.hpp" // static_cast NiNode
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::ATextureRenderData::ATextureRenderData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    if (stream->nifVer() > 0x14000004)
        throw std::runtime_error("NiBtOgre::ATextureRenderData::unsupported NIF file version");

    stream->read(mPixelFormat);
    stream->read(mRedMask);
    stream->read(mGreenMask);
    stream->read(mBlueMask);
    stream->read(mAlphaMask);
    stream->read(mBitsPerPixel);

    mUnknown3Bytes.resize(3);
    for (unsigned int i = 0; i < 3; ++i)
        stream->read(mUnknown3Bytes.at(i));

    mUnknown8Bytes.resize(8);
    for (unsigned int i = 0; i < 8; ++i)
        stream->read(mUnknown8Bytes.at(i));

    if (stream->nifVer() >= 0x0a010000 && stream->nifVer() <= 0x0a020000)
        stream->skip(sizeof(std::uint32_t));

    stream->read(mPaletteRef);
    stream->read(mNumMipmaps);
    stream->read(mBytesPerPixel);

    mMipmaps.resize(mNumMipmaps);
    for (unsigned int i = 0; i < mNumMipmaps; ++i)
    {
        stream->read(mMipmaps[i].width);
        stream->read(mMipmaps[i].height);
        stream->read(mMipmaps[i].offset);
    }
}

NiBtOgre::NiPixelData::NiPixelData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : ATextureRenderData(index, stream, model, data)
{
    if (stream->nifVer() > 0x14000004)
        throw std::runtime_error("NiBtOgre::NiPixelData::unsupported NIF file version");

    stream->read(mNumPixels);
    mPixelData.resize(mNumPixels);
    for (unsigned int i = 0; i < mNumPixels; ++i)
        stream->read(mPixelData.at(i));
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBound::BSMultiBound(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->read(mDataRef);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBoundOBB::BSMultiBoundOBB(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : BSMultiBoundData(index, stream, model, data)
{
    stream->read(mCenter);
    stream->read(mSize);
    stream->read(mRotation);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiAVObjectPalette(index, stream, model, data)
{
    stream->skip(sizeof(std::uint32_t));

    std::uint32_t numObjs = 0;
    stream->read(numObjs);

#if 0
    mObjs.resize(numObjs);
    for (unsigned int i = 0; i < numObjs; ++i)
    {
        stream->readSizedString(mObjs.at(i).name);
        stream->read(mObjs.at(i).avObjectRef);
    }
#endif
    std::string name;
    NiAVObjectRef objRef;
    for (unsigned int i = 0; i < numObjs; ++i)
    {
        stream->readSizedString(name);
        stream->read(objRef);
        mObjRefMap[name] = objRef;
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSShaderTextureSet::BSShaderTextureSet(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    std::uint32_t numTextures = 0;
    stream->read(numTextures);

    mTextures.resize(numTextures);
    for (unsigned int i = 0; i < numTextures; ++i)
        stream->readSizedString(mTextures.at(i));
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineBasisData::NiBSplineBasisData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->read(mNumControlPoints);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineData::NiBSplineData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->readVector<float>(mFloatControlPoints);
    stream->readVector<std::int16_t>(mShortControlPoints);
}

#if 0
template<>
NiBtOgre::KeyGroup<char>::KeyGroup(NiStream *stream)
{
    // FIXME
}

template<>
NiBtOgre::KeyGroup<Ogre::Vector4>::KeyGroup(NiStream *stream)
{
    // FIXME
}

template<>
NiBtOgre::KeyGroup<float>::KeyGroup(NiStream *stream)
{
    // FIXME
}
#endif

// below code is copied from NifSkope
template<>
bool NiBtOgre::AnimTrackInterpolator<float>::getInterpolatedKeyFrame(const Ogre::AnimationTrack *t,
        const Ogre::TimeIndex& timeIndex, Ogre::KeyFrame *kf)
{
    // only support VertexMorphKeyFrame for now
    Ogre::VertexPoseKeyFrame *keyFrame = dynamic_cast<Ogre::VertexPoseKeyFrame*>(kf);
    if (!keyFrame)
        return false;

    // only support quadratic for now
    if (mInterpolation != 2)
        return false;

    float x, v1, v2; // FIXME: not initialised ?!  fortunately nobody seems to be calling this method for now

    // Tangent 1
    float t1 = mKey.backward;
    // Tangent 2
    float t2 = mKey.forward;

    float x2 = x * x;
    float x3 = x2 * x;

    float value = v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);
    keyFrame->updatePoseReference(/*poseIndex*/0, value); // only one PoseRef per keyframe, so index is 0

    return true;
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBoolData::NiBoolData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    mData.read(stream);
}

NiBtOgre::NiColorData::NiColorData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    mData.read(stream);
}

NiBtOgre::NiExtraData::NiExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    if (stream->nifVer() == 0x0a000100) // GOG Clutter\Farm\Oar01.nif
        stream->skip(sizeof(std::int32_t));
    else if (stream->nifVer() == 0x0a01006a) // GOG dungeons\misc\skydome01.nif (userVer=10, userVer2=5)
        stream->skip(sizeof(std::int32_t));

    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
        stream->readLongString(mName);

    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        stream->read(mNextRef);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->readLongString(mBehaviourGraphFile);
    stream->read(mControlBaseSkeleton);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::BSBound::BSBound(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mCenter);
    stream->read(mDimensions);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSBoneLODExtraData::BSBoneLODExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mBoneLODCount);

    mBoneLODInfo.resize(mBoneLODCount);
    for (unsigned int i = 0; i < mBoneLODCount; ++i)
    {
        stream->read(mBoneLODInfo.at(i).distance);
        stream->readLongString(mBoneLODInfo.at(i).boneName);
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mUnknown1);

    std::uint16_t numVectorBlocks;
    stream->read(numVectorBlocks);

    mVectorBlocks.resize(numVectorBlocks);
    for (unsigned int i = 0; i < numVectorBlocks; ++i)
    {
        std::uint16_t numVectors;
        stream->read(numVectors);

        mVectorBlocks[i].points.resize(numVectors);
        for (unsigned int j = 0; j < numVectors; ++j)
            stream->read(mVectorBlocks[i].points.at(j));

        mVectorBlocks[i].normals.resize(numVectors);
        for (unsigned int j = 0; j < numVectors; ++j)
            stream->read(mVectorBlocks[i].normals.at(j));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::BSFurnitureMarker::BSFurnitureMarker(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    std::uint32_t numPos;
    stream->read(numPos);

    mPositions.resize(numPos);
    for (unsigned int i = 0; i < numPos; ++i)
    {
        stream->read(mPositions[i].offset);
        if (stream->userVer() <= 11)
        {
            stream->read(mPositions[i].orientation);
            stream->read(mPositions[i].posRef1);
            stream->read(mPositions[i].posRef2);
        }
        if (stream->nifVer() >= 0x14020007 && stream->userVer() >= 12) // from 20.2.0.7
        {
            stream->read(mPositions[i].heading);
            stream->read(mPositions[i].animationType);
            stream->read(mPositions[i].entryProperties);
        }
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSInvMarker::BSInvMarker(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mRotationX);
    stream->read(mRotationY);
    stream->read(mRotationZ);
    stream->read(mZoom);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBinaryExtraData::NiBinaryExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->readVector<char>(mData);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBooleanExtraData::NiBooleanExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mBooleanData);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiFloatExtraData::NiFloatExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mFloatData);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
//
// BSXFlags example: ./fire/fireopenmediumsmoke.nif
//   has NiBillboardNode as the root with BSX == 0x23, and has a child EditorMarker NiNode
//
// EditorMarker NiNodes seem to have Flag == 0x10 == 0b10000(similarly Gravity NiNode)
//   - but ./effects/sefxbreathstreight512.nif has 0xE == 0b01110
NiBtOgre::NiIntegerExtraData::NiIntegerExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    stream->read(mIntegerData);

    /* ---------------------------------------------------------------------- */

    // FIXME: this probably doesn't belong here
    //        used by NiNode and NiTriBasedGeom
    // clutter\magesguild\mageguildrugcircle01.nif has mName = -1
    if (mName != -1 && model.indexToString(mName) == "BSX")
    {
#if 0
        if ((mIntegerData & 0x20) != 0)
        //{
            data.mBuildFlags |= Flag_EditorMarkerPresent;
            //data.mEditorMarkerPresent = true;
        //}
        //else
            //data.mEditorMarkerPresent = false;

        if (stream->nifVer() < 0x14020007) // FIXME: TES5 differrent
        {
            if ((mIntegerData & 0x01) != 0)
                data.mBuildFlags |= Flag_EnableHavok;

            if ((mIntegerData & 0x02) != 0)
                data.mBuildFlags |= Flag_EnableCollision;

            if ((mIntegerData & 0x04) != 0)
            //{
                data.mBuildFlags |= Flag_IsSkeleton;
                //data.mIsSkeleton = true;
            //}
            //else
                //data.mIsSkeleton = false;

            if ((mIntegerData & 0x08) != 0)
                data.mBuildFlags |= Flag_EnableAnimation;

            if ((mIntegerData & 0x10) != 0)
            //{
                data.mBuildFlags |= Flag_FlameNodesPresent;
                //data.mFlameNodesPresent = true;
            //}
            //else
                //data.mFlameNodesPresent = false;
        }
#else
        if (stream->nifVer() < 0x14020007)
            data.mBuildFlags |= (mIntegerData & 0x3f); // lower 6 bits
        else
        {
            if ((mIntegerData & 0x01) != 0)
                data.mBuildFlags |= Flag_EnableAnimation;

            if ((mIntegerData & 0x02) != 0)
                data.mBuildFlags |= Flag_EnableHavok;

            if ((mIntegerData & 0x20) != 0)
                data.mBuildFlags |= Flag_EditorMarkerPresent;
        }

        // FIXME: testing TES5
        if (stream->nifVer() >= 0x14020007) // from 20.2.0.7
        {
            if ((mIntegerData & 0x04) != 0)
                data.mBuildFlags |= Flag_IsSkeleton; // Nifskope says this indicates a ragdoll
        }
#endif
    }
}

// ./x/ex_waterfall_mist_01.nif (has examples of MRK and sgoKeep)
//
// NCO means "No COllision" (e.g. ./l/light_com_lantern_02.nif)
// MRK means "there is an Editor Marker in this file"
NiBtOgre::NiStringExtraData::NiStringExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        stream->skip(sizeof(std::uint32_t));

    stream->readLongString(mStringData);

#if 0 // FIXME: testing only
    if (model.indexToString(mStringData) == "MRK") // FIXME: testing only
        std::cout << "NiStringExtraData : MRK" << std::endl;
#endif
}

// ./base_anim.nif (seems like all the animations are in one file and start/stop times are
//                  specified in text keys)
// ./architecture/anvil/anvildoormcanim01.nif (special sound effect)
// ./architecture/solitude/interiors/ssecretjaildoor01.nif (start/end)
NiBtOgre::NiTextKeyExtraData::NiTextKeyExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        stream->skip(sizeof(std::uint32_t)); // always 0?

    std::uint32_t numTextKeys;
    stream->read(numTextKeys);
    mTextKeys.resize(numTextKeys);
    for (unsigned int i = 0; i < numTextKeys; ++i)
    {
        stream->read(mTextKeys.at(i).time);
        stream->readLongString(mTextKeys.at(i).text);
    }
}

NiBtOgre::NiVertWeightsExtraData::NiVertWeightsExtraData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiExtraData(index, stream, model, data)
{
    std::uint32_t numBytes;
    stream->read(numBytes);

    std::uint16_t numVertices;
    stream->read(numVertices);

    stream->skip(numVertices * sizeof(float));
}

NiBtOgre::NiFloatData::NiFloatData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    if (stream->nifVer() == 0x0a01006a) // 10.1.0.106
        stream->skip(sizeof(int32_t)); // e.g. creatures/horse/Bridle.NIF version 10.1.0.106

    mData.read(stream);
}

NiBtOgre::NiGeometryData::NiGeometryData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isNiPSysData)
    : NiObject(index, stream, model, data), mNumVertices(0), mNumUVSets(0), mBSNumUVSets(0) , mIsNiPSysData(isNiPSysData)
{
    if (stream->nifVer() == 0x0a000100)     // HACK: not sure why this is needed
        stream->skip(sizeof(std::int32_t)); // e.g. clutter/farm/oar01.nif version 10.0.1.0
    else if (stream->nifVer() == 0x0a01006a)
        stream->skip(sizeof(std::int32_t)); // e.g. creatures/horse/Bridle.NIF version 10.1.0.106

    if (stream->nifVer() >= 0x0a020000) // from 10.2.0.0
    {
        std::int32_t unknownInt;
        stream->read(unknownInt); // always 0
    }

    if (!mIsNiPSysData || (mIsNiPSysData && (stream->nifVer() < 0x14020007 || stream->userVer() < 11)))
        stream->read(mNumVertices);

    if (mIsNiPSysData && stream->nifVer() >= 0x14020007 && stream->userVer() >= 11)
        stream->read(mBSMaxVertices);

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        stream->read(mKeepFlags);
        stream->read(mCompressFlags);
    }

    if (stream->getBool()) // has vertices
    {
        stream->readVector<Ogre::Vector3>(mVertices, mNumVertices);
    }

    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
    {
        if (stream->nifVer() < 0x14020007 || stream->userVer() < 11)
            stream->read(mNumUVSets);

        if (stream->nifVer() >= 0x14020007 && stream->userVer() >= 11)
            stream->read(mBSNumUVSets);
    }

    if (!mIsNiPSysData && stream->nifVer() >= 0x14020007 && stream->userVer() == 12)
        stream->skip(sizeof(std::uint32_t)); // Unknown Int 2

    bool hasNormals = stream->getBool();
    if (hasNormals)
        stream->readVector<Ogre::Vector3>(mNormals, mNumVertices);

    if (hasNormals && ((mNumUVSets & 0xf000) || (mBSNumUVSets & 0xf000)))
    {
        stream->readVector<Ogre::Vector3>(mTangents, mNumVertices);
        stream->readVector<Ogre::Vector3>(mBitangents, mNumVertices);
    }

    stream->read(mCenter);
    stream->read(mRadius);

    if (stream->getBool()) // has vertex colors
        stream->readVector<Ogre::Vector4>(mVertexColors, mNumVertices);

    std::uint16_t numUVSets = 0;
    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
    {
        stream->read(numUVSets);
        numUVSets &= 0x3f;
    }
    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
        numUVSets = mNumUVSets & 0x3f;
    if (stream->nifVer() >= 0x14020007 && stream->userVer() >= 11)
        numUVSets |= mBSNumUVSets & 1;

    if (stream->nifVer() <= 0x04000002) // up to 4.0.0.2
        bool hasUV = stream->getBool(); // is this used anywhere?

    mUVSets.resize(numUVSets);
    for (unsigned int i = 0; i < numUVSets; ++i)
    {
        mUVSets.at(i).resize(mNumVertices);
        for (unsigned int j = 0; j < mNumVertices; ++j)
        {
            //stream->read(mUVSets.at(i).at(j).u);
            //stream->read(mUVSets.at(i).at(j).v);
            Ogre::Vector2 uv; // FIXME
            stream->read(uv.x);
            stream->read(uv.y);
            mUVSets.at(i).at(j) = uv;
        }
    }

    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
        if (stream->userVer() < 12 || (stream->userVer() >= 12 && !mIsNiPSysData))
            stream->read(mConsistencyFlags);

    if (stream->nifVer() >= 0x14000004) // from 20.0.0.4
        if (stream->userVer() < 12 || (stream->userVer() >= 12 && !mIsNiPSysData))
            stream->read(mAdditionalDataRef);
}

NiBtOgre::NiParticlesData::NiParticlesData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isNiPSysData)
    : NiGeometryData(index, stream, model, data, isNiPSysData)
{
    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        stream->read(mNumParticles);

    if (stream->nifVer() <= 0x0a000100) // up to 10.0.1.0
        stream->read(mParticleRadius);

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        if (stream->getBool() && !(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
            stream->readVector<float>(mRadii, mNumVertices);
    }

    stream->read(mNumActive);

    if (stream->getBool() && !(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
        stream->readVector<float>(mSizes, mNumVertices);

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        if (stream->getBool() && !(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
            stream->readVector<Ogre::Quaternion>(mRotations, mNumVertices);
    }

    if (stream->nifVer() >= 0x14020007 && stream->userVer() >= 12)
    {
        stream->skip(sizeof(char)); // unknown byte 1
        NiObjectRef unknownLink;
        stream->read(unknownLink);
    }

    if (stream->nifVer() >= 0x14000004) // from 20.0.0.4
    {
        if (stream->getBool() && !(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
            stream->readVector<float>(mRotationAngles, mNumVertices);
    }

    if (stream->nifVer() >= 0x14000004) // from 20.0.0.4
    {
        if (stream->getBool() && !(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
            stream->readVector<Ogre::Vector3>(mRotationAxes, mNumVertices);
    }

    if (stream->nifVer() >= 0x14020007 && stream->userVer() == 11)
    {
        bool hasUVQuads = stream->getBool();
        unsigned char numUVQuadrants = 0;
        stream->read(numUVQuadrants);
        if (hasUVQuads)
            stream->readVector<Ogre::Vector4>(mUVQuadrants, numUVQuadrants);
    }

    if (stream->nifVer() == 0x14020007 && stream->userVer() >= 11)
        stream->skip(sizeof(char)); // unknown byte 2
}

NiBtOgre::NiRotatingParticlesData::NiRotatingParticlesData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data,
        bool isNiPSysData)
    : NiParticlesData(index, stream, model, data, isNiPSysData)
{
    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        if (stream->getBool())
            stream->readVector<Ogre::Quaternion>(mRotations2, mNumVertices);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysData::NiPSysData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiRotatingParticlesData(index, stream, model, data, true)
{
    if (!(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
    {
        mParticleDescriptions.resize(mNumVertices);
        for (unsigned int i = 0; i < mNumVertices; ++i)
        {
            // ParticleDesc
            stream->read(mParticleDescriptions.at(i).translation);
            if (stream->nifVer() <= 0x0a040001) // up to 10.4.0.1
                stream->read(mParticleDescriptions.at(i).unknownFloats);
            stream->read(mParticleDescriptions.at(i).unknown1);
            stream->read(mParticleDescriptions.at(i).unknown2);
            stream->read(mParticleDescriptions.at(i).unknown3);
            stream->read(mParticleDescriptions.at(i).unknown);
        }
    }

    if (stream->nifVer() >= 0x14000004 && !(stream->nifVer() >= 0x14020007 && stream->userVer() >= 11))
    {
        if (stream->getBool())
            stream->readVector<float>(mUnknownFloats3, mNumVertices);
    }

    if (!(stream->nifVer() >= 0x14020007 && stream->userVer() == 11))
    {
        stream->skip(sizeof(std::uint16_t));
        stream->skip(sizeof(std::uint16_t));
    }

    if (stream->nifVer() >= 0x14020007 && stream->userVer() >= 12)
    {
        bool hasSubTexOffsetUVs = stream->getBool();
        std::uint32_t numSubTexOffsetUVs;
        stream->read(numSubTexOffsetUVs);
        stream->read(mAspectRatio);
        if (hasSubTexOffsetUVs)
            stream->readVector<Ogre::Vector4>(mSubTextureOffsetUVs, numSubTexOffsetUVs);
        stream->skip(sizeof(std::uint32_t)); // unknown Int 4
        stream->skip(sizeof(std::uint32_t)); // unknown Int 5
        stream->skip(sizeof(std::uint32_t)); // unknown Int 6
        stream->skip(sizeof(std::uint16_t)); // unknown short 3
        stream->skip(sizeof(char)); // unknown byte 4
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSStripPSysData::BSStripPSysData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiPSysData(index, stream, model, data)
{
    stream->read(mUnknown5);
    stream->read(mUnknown6);
    stream->read(mUnknown7);
    stream->read(mUnknown8);
}

NiBtOgre::NiTriBasedGeomData::NiTriBasedGeomData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiGeometryData(index, stream, model, data)
{
    stream->read(mNumTriangles);
}

NiBtOgre::NiTriShapeData::NiTriShapeData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTriBasedGeomData(index, stream, model, data)
{
    std::uint32_t numTrianglePoints; // mNumTriangles times 3
    stream->read(numTrianglePoints);

    bool hasTriangles = false;
    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
        hasTriangles = stream->getBool();
    else
        hasTriangles = true;

    if (hasTriangles)
        stream->readVector<std::uint16_t>(mTriangles, numTrianglePoints);

    std::uint16_t numMatchGroups;
    stream->read(numMatchGroups);

    std::uint16_t numVertices;
    for (unsigned int i = 0; i < numMatchGroups; ++i)
    {
        stream->read(numVertices);
        stream->skip(numVertices * sizeof(std::uint16_t));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTriStripsData::NiTriStripsData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTriBasedGeomData(index, stream, model, data)
{
    std::uint16_t numStrips;
    stream->read(numStrips);

    mStripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        stream->read(mStripLengths.at(i));

    bool hasPoints = false;
    if (stream->nifVer() <= 0x0a000102) // up to 10.0.1.2
        hasPoints = true;
    else
        hasPoints = stream->getBool();

    if (hasPoints)
    {
        mPoints.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            mPoints[i].resize(mStripLengths[i]);
            for (unsigned int j = 0; j < mStripLengths[i]; ++j)
                stream->read(mPoints.at(i).at(j));
        }
    }

    // there are (N-2)*3 vertex indices for triangles
    // where N = stripLengths[stripIndex]
    //
    // e.g. strip length = 150
    //      (150-2)*3 = 148*3 = 444
    unsigned int base = 0;
    for (unsigned int i = 0; i < numStrips; ++i)
    {
        base = static_cast<unsigned int>(mTriangles.size());
        mTriangles.resize(base + (mStripLengths[i]-2)*3);
        for (unsigned int j = 0; j < (unsigned int)(mStripLengths[i]-2); ++j)
        {
            if (j & 1)
            {
                mTriangles[base+j*3]   = mPoints[i][j];
                mTriangles[base+j*3+1] = mPoints[i][j+2];
                mTriangles[base+j*3+2] = mPoints[i][j+1];
            }
            else
            {
                mTriangles[base+j*3]   = mPoints[i][j];
                mTriangles[base+j*3+1] = mPoints[i][j+1];
                mTriangles[base+j*3+2] = mPoints[i][j+2];
            }
        }
    }
#if 0
    //std::vector<Triangle> packedTriangle;
    std::vector<std::uint16_t> packedTriangleStrip;
    for (unsigned int i = 0; i < numStrips; ++i)
    {
        base = static_cast<unsigned int>(mTriangles.size());
        for (unsigned int j = 0; j < (unsigned int)(mStripLengths[i]-2); ++j)
        {
            // skipping (packing?) idea copied from NifSkope nvtristripwrapper::triangulate()
            // i.e. ( a != b && b != c && c != a )
            if (mPoints[i][j]   != mPoints[i][j+1] &&
                mPoints[i][j+1] != mPoints[i][j+2] &&
                mPoints[i][j+2] != mPoints[i][j]     )
            {
                //Triangle triangle;
                if (j & 1)
                {
                    //triangle.v1 = mPoints[i][j];
                    //triangle.v2 = mPoints[i][j+2];
                    //triangle.v3 = mPoints[i][j+1];
                    packedTriangleStrip.push_back(mPoints[i][j]);
                    packedTriangleStrip.push_back(mPoints[i][j+2]);
                    packedTriangleStrip.push_back(mPoints[i][j+1]);
                }
                else
                {
                    //triangle.v1 = mPoints[i][j];
                    //triangle.v2 = mPoints[i][j+1];
                    //triangle.v3 = mPoints[i][j+2];
                    packedTriangleStrip.push_back(mPoints[i][j]);
                    packedTriangleStrip.push_back(mPoints[i][j+1]);
                    packedTriangleStrip.push_back(mPoints[i][j+2]);
                }
                //packedTriangle.push_back(triangle);
            }
        }
    }
#endif
}

NiBtOgre::NiKeyframeData::NiKeyframeData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data), mRotationType(1/*LINEAR_KEY*/)
{
    std::uint32_t numRotationKeys = 0;
    stream->read(numRotationKeys);

    if (numRotationKeys != 0)
        stream->read(mRotationType);

    if (mRotationType != 4) // not XYZ_ROTATION_KEY (i.e. LINEAR, QUADRATIC, TBC, CONSTANT)
    {
        mQuaternionKeys.interpolation = mRotationType; // TODO: check not quadratic?
        for(unsigned int i = 0; i < numRotationKeys; ++i)
        {
            Key<Ogre::Quaternion> key;
            key.read(stream, mRotationType);

            // FIXME: testing
            if (mQuaternionKeys.indexMap.find(key.time) != mQuaternionKeys.indexMap.end())
                throw std::runtime_error("Quat key map collision");

            mQuaternionKeys.indexMap[key.time] = (int) mQuaternionKeys.keys.size();
            mQuaternionKeys.keys.push_back(key);
        }
    }
    else                    // XYZ_ROTATION_KEY
    {
        if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
            stream->skip(sizeof(float)); // unknown float
        mXRotations.read(stream);
        mYRotations.read(stream);
        mZRotations.read(stream);
    }
    mTranslations.read(stream);
    mScales.read(stream);
}

NiBtOgre::NiMorphData::NiMorphData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    std::uint32_t numMorphs;
    stream->read(numMorphs);

    std::uint32_t numVertices;
    stream->read(numVertices);

    stream->skip(sizeof(char)); // relative targets

    mMorphs.resize(numMorphs);
    for (unsigned int i = 0; i < numMorphs; ++i)
    {
        if (stream->nifVer() >= 0x0a01006a) // from 10.1.0.106
            stream->readLongString(mMorphs.at(i).mFrameName);

        if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
        {
            std::uint32_t numKeys;
            stream->read(numKeys);
            std::uint32_t interpolation;
            stream->read(interpolation);

            mMorphs.at(i).mKeys.resize(numKeys);
            for (unsigned int j = 0; j < numKeys; ++j)
            {
                Key<float> key;
                key.read(stream, interpolation);
            }
        }

        if (stream->nifVer() >= 0x0a01006a && stream->nifVer() <= 0x0a020000)
        {
            // don't skip for GOG creatures\mudcrab\Mud Crbeye L00.NIF
            if (!(stream->nifVer() == 0x0a020000 && stream->userVer() == 10 && stream->userVer2() == 11))
                stream->skip(sizeof(std::uint32_t));
        }

        if (stream->nifVer() >= 0x14000004 && stream->nifVer() <= 0x14000005 && stream->userVer() == 0)
            stream->skip(sizeof(std::uint32_t));

        stream->readVector<Ogre::Vector3>(mMorphs.at(i).mVectors, numVertices);
    }
}

NiBtOgre::NiPosData::NiPosData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    mData.read(stream);
}

NiBtOgre::NiSkinData::NiSkinData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data), mSkinPartitionRef(-1)
{
    stream->read(mSkinTransform.rotation);
    stream->read(mSkinTransform.translation);
    stream->read(mSkinTransform.scale);

    std::uint32_t numBones;
    stream->read(numBones);
    if (/*stream->nifVer() >= 0x04000002 && */stream->nifVer() <= 0x0a010000) // we don't support < 4.0.0.2
        stream->read(mSkinPartitionRef);

    unsigned char hasVertexWeights = 0;
    if (stream->nifVer() >= 0x04020100) // from 4.2.1.0 (should this be 4.2.2.0?)
        stream->read(hasVertexWeights);

    mBoneList.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
    {
        SkinData &skin = mBoneList.at(i);

        stream->read(skin.skinTransform.rotation);
        stream->read(skin.skinTransform.translation);
        stream->read(skin.skinTransform.scale);
        stream->read(skin.boundingSphereOffset);
        stream->read(skin.boundingSphereRadius);

        std::uint16_t numVertices;
        stream->read(numVertices);
        if (stream->nifVer() <= 0x04020100 || (stream->nifVer() >= 0x04020200 && hasVertexWeights))
        {
            skin.vertexWeights.resize(numVertices);
            for (unsigned int j = 0; j < numVertices; ++j)
            {
                stream->read(skin.vertexWeights[j].vertex);
                stream->read(skin.vertexWeights[j].weight);
            }
        }
    }
}

NiBtOgre::NiSkinInstance::NiSkinInstance(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->read(mDataRef);
    if (stream->nifVer() >= 0x0a020000) // from 10.2.0.0
        stream->read(mSkinPartitionRef);
    //stream->getPtr<NiNode>(mSkeletonRoot, model.objects());
  //stream->read(mSkeletonRootRef);
    std::int32_t rIndex = -1;
    stream->read(rIndex);
    mSkeletonRoot = model.getRef<NiNode>(rIndex);

    std::uint32_t numBones;
    stream->read(numBones);
    mBoneRefs.resize(numBones);
    bool hasValidBoneRef = false;
    for (unsigned int i = 0; i < numBones; ++i)
    {
        stream->read(mBoneRefs.at(i));
        if (mBoneRefs[i] == -1)
            continue;

        hasValidBoneRef = true;
        // NOTE: for a skinned model we're going to be using an external skeleton
        data.addBoneTreeLeafIndex(mBoneRefs[i]); // register for building a skeleton
    }

    // NOTE: SkeletonRoot is *not* added to mBoneTreeLeafIndices.  This is intentional.
    if (hasValidBoneRef && rIndex != -1)
        data.setSkeletonRoot(rIndex);
}

NiBtOgre::BSDismemberSkinInstance::BSDismemberSkinInstance(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSkinInstance(index, stream, model, data)
{
    std::uint32_t numPartitions;
    stream->read(numPartitions);
    mPartitions.resize(numPartitions);
    for (uint32_t i = 0; i < numPartitions; ++i)
    {
        stream->read(mPartitions.at(i).partFlag);
        stream->read(mPartitions.at(i).bodyPart);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
void NiBtOgre::NiSkinPartition::SkinPartition::read(NiStream *stream)
{
    stream->read(numVertices);
    stream->read(numTriangles);
    stream->read(numBones);
    stream->read(numStrips);
    stream->read(numWeightsPerVertex);

    bones.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
        stream->read(bones.at(i));

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
        hasVertexMap = stream->getBool();
    if (stream->nifVer() <= 0x0a000102 || (stream->nifVer() >= 0x0a010000 && hasVertexMap))
    {
        vertexMap.resize(numVertices);
        for (unsigned int i = 0; i < numVertices; ++i)
            stream->read(vertexMap.at(i));
    }

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
        hasVertexWeights = stream->getBool();
    if (stream->nifVer() <= 0x0a000102 || (stream->nifVer() >= 0x0a010000 && hasVertexWeights))
    {
        vertexWeights.resize(numVertices);
        for (unsigned int i = 0; i < numVertices; ++i)
        {
            vertexWeights[i].resize(numWeightsPerVertex);
            for (unsigned int j = 0; j < numWeightsPerVertex; ++j)
            {
                stream->read(vertexWeights.at(i).at(j));
            }
        }
    }

    stripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        stream->read(stripLengths.at(i));

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
        hasFaces = stream->getBool();

    if (hasFaces && numStrips != 0)
    {
        if (stream->nifVer() <= 0x0a000102 || (stream->nifVer() >= 0x0a010000 && hasFaces))
        {
            strips.resize(numStrips);
            for (unsigned int i = 0; i < numStrips; ++i)
            {
                strips[i].resize(stripLengths[i]);
                for (unsigned int j = 0; j < stripLengths[i]; ++j)
                {
                    stream->read(strips.at(i).at(j));
                }
            }
        }
    }
    else // numStrips == 0
    {
        if (stream->nifVer() <= 0x0a000102 || (stream->nifVer() >= 0x0a010000 && hasFaces))
        {
            triangles.resize(numTriangles);
            for (unsigned int i = 0; i < numTriangles; ++i)
            {
                stream->read(triangles.at(i).v1);
                stream->read(triangles.at(i).v2);
                stream->read(triangles.at(i).v3);
            }
        }
    }

    hasBoneIndices = stream->getBool();
    if (hasBoneIndices)
    {
        boneIndices.resize(numVertices);
        for (unsigned int i = 0; i < numVertices; ++i)
        {
            boneIndices[i].resize(numWeightsPerVertex);
            for (unsigned int j = 0; j < numWeightsPerVertex; ++j)
            {
                stream->read(boneIndices.at(i).at(j));
            }
        }
    }

    if (stream->userVer() >= 12)
        stream->skip(sizeof(std::uint16_t));

    // FIXME: more unknowns here for version 10.2.0.0
}

NiBtOgre::NiSkinPartition::NiSkinPartition(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->read(mNumSkinPartitionBlocks);
    mSkinPartitionBlocks.resize(mNumSkinPartitionBlocks);
    for (unsigned int i = 0; i < mNumSkinPartitionBlocks; ++i)
        mSkinPartitionBlocks[i].read(stream);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiStringPalette::NiStringPalette(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->readSizedString(mPalette);
    stream->read(mLength); // TODO: validate against mPalette.size()
// FIXME: for testing only
#if 0
    std::stringstream ss(mPalette);
    std::string n;
    std::vector<std::string> names;
    while (std::getline(ss, n, '\0')) // split the strings
    {
        names.push_back(n);
        std::cout << n << std::endl;
    }
#endif
}

NiBtOgre::NiUVData::NiUVData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    mUVGroups0.read(stream);
    mUVGroups1.read(stream);
    mUVGroups2.read(stream);
    mUVGroups3.read(stream);
}

NiBtOgre::NiVisData::NiVisData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    std::uint32_t numKeys;
    stream->read(numKeys);

    mKeys.resize(numKeys);
    for (unsigned int i = 0; i < numKeys; ++i)
        mKeys[i].read(stream, 1); // LINEAR_KEY
}
