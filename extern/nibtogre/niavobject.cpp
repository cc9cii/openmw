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
#include "niavobject.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "btogreinst.hpp"
#include "nitimecontroller.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

NiBtOgre::NiAVObject::NiAVObject(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObjectNET(index, stream, model, data), mHasBoundingBox(false)
      //, mWorldTransform(Ogre::Matrix4::IDENTITY)
{
    if (!stream) // must be a dummy block being inserted
    {
        mFlags = 0;
        mTranslation = Ogre::Vector3::ZERO;
        mRotation = Ogre::Matrix3::IDENTITY;
        mScale = 1.f;

        mProperty.clear();
        mCollisionObjectRef = -1;

        return;
    }

    stream->read(mFlags);

    if (stream->nifVer() >= 0x14020007 && (stream->userVer() >= 11 && stream->userVer2() > 26)) // from 20.2.0.7
        stream->skip(sizeof(std::uint16_t));

    stream->read(mTranslation);
    stream->read(mRotation);
    stream->read(mScale);

    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        stream->read(mVelocity);

    if (stream->nifVer() < 0x14020007 || stream->userVer() <= 11) // less than 20.2.0.7 (or user version <= 11)
        stream->readVector<NiPropertyRef>(mProperty);

    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
    {
        // Looks like only used for animations, examples include:
        //   ./r/xashslave.nif
        //   ./r/xbabelfish.nif
        //   ./r/xcavemudcrab.nif
        //   ./r/xcliffracer.nif
        //   ./r/xcorprus_stalker.nif
        //   ./r/xguar.nif
        //   ./r/xkwama forager.nif
        //   ./r/xminescrib.nif
        //   ./r/xrust rat.nif
        //   ./r/xscamp_fetch.nif
        //   ./r/xslaughterfish.nif
        //   ./xbase_anim.1st.nif
        //   ./xbase_anim.nif
        //   ./xbase_animkna.nif
        if(mHasBoundingBox = stream->getBool())
        {
            stream->read(mBoundingBox.unknownInt);
            stream->read(mBoundingBox.translation);
            stream->read(mBoundingBox.rotation);
            stream->read(mBoundingBox.radius);
        }
    }

    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
        stream->read(mCollisionObjectRef);
}

//void NiBtOgre::NiAVObject::build(BtOgreInst *inst, NiObject *parent)
//{
    // probably never called, remove?
//}

NiBtOgre::NiTimeController *NiBtOgre::NiAVObject::findController(const std::string& controllerType)
{
    NiTimeControllerRef index = NiObjectNET::mControllerRef;
    while (index != -1)
    {
        if (mModel.blockType(index) == controllerType)
            return mModel.getRef<NiTimeController>(index);

        index = mModel.getRef<NiTimeController>(index)->mNextControllerRef;
    }

    return nullptr;
}

NiBtOgre::NiCamera::NiCamera(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiAVObject(index, stream, model, data), mUseOrthographicProjection(false)
{
    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
        stream->skip(sizeof(std::uint16_t));

    stream->read(mFrustumLeft);
    stream->read(mFrustumRight);
    stream->read(mFrustumTop);
    stream->read(mFrustumBottom);
    stream->read(mFrustumNear);
    stream->read(mFrustumFar);

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
        mUseOrthographicProjection = stream->getBool();

    stream->read(mViewportLeft);
    stream->read(mViewportRight);
    stream->read(mViewportTop);
    stream->read(mViewportBottom);

    stream->read(mLODAdjust);

    stream->skip(sizeof(std::uint32_t)); // Unknown Ref
    stream->skip(sizeof(std::uint32_t)); // Unknown Int
    if (stream->nifVer() >= 0x04020100) // from 4.2.1.0
        stream->skip(sizeof(std::uint32_t)); // Unknown Int2
}

void NiBtOgre::NiCamera::build(BuildData *data, NiObject *parent)
{
}

NiBtOgre::NiDynamicEffect::NiDynamicEffect(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiAVObject(index, stream, model, data), mSwitchState(false)
{
    if (stream->nifVer() >= 0x0a01006a) // from 10.1.0.106
        mSwitchState = stream->getBool();
    // TODO: how to decode the pointers in ver 4.0.0.2
    if (stream->nifVer() >= 0x0a010000) // GOG Characters\Imperial\EarsHuman.nif
        stream->readVector<NiAVObjectRef>(mAffectedNodes);
}

NiBtOgre::NiLight::NiLight(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiDynamicEffect(index, stream, model, data)
{
    stream->read(mDimmer);
    stream->read(mAmbientColor);
    stream->read(mDiffuseColor);
    stream->read(mSpecularColor);
}

NiBtOgre::NiPointLight::NiPointLight(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiLight(index, stream, model, data)
{
    stream->read(mAttenuationConstant);
    stream->read(mAttenuationLinear);
    stream->read(mAttenuationQuadratic);
}

NiBtOgre::NiTextureEffect::NiTextureEffect(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiDynamicEffect(index, stream, model, data)
{
    stream->read(mModelProjectionMatrix);
    stream->read(mModelProjectionTransform);

    stream->read(mTextureFiltering);
    stream->read(mTextureClamping);
    stream->read(mTextureType);
    stream->read(mCoordGenType);

    stream->read(mSourceTexture); // from 4.0.0.0

    stream->read(mClippingPlane);
    stream->skip(4*sizeof(float)); // Unknown Vector3 and float

    if (stream->nifVer() <= 0x0a020000) // up to 10.2.0.0
    {
        stream->skip(sizeof(std::int16_t)); // 0?
        stream->skip(sizeof(std::int16_t)); // -75?
    }

    if (stream->nifVer() <= 0x0401000c) // up to 4.1.0.12
        stream->skip(sizeof(std::uint16_t));
}
