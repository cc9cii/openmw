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
#ifndef NIBTOGRE_NIAVOBJECT_H
#define NIBTOGRE_NIAVOBJECT_H

#include <OgreVector3.h>
#include <OgreMatrix3.h>
#include <OgreMatrix4.h>

#include "niobjectnet.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiAVObject
//         NiCamera
//         NiDynamicEffect
//             NiLight
//                 NiAmbientLight <--------- /* typedef NiLight */
//                 NiDirectionalLight <----- /* typedef NiLight */
//                 NiPointLight
//             NiTextureEffect
namespace NiBtOgre
{
    class NiStream;
    class NiTimeController;

    class NiAVObject : public NiObjectNET
    {
    public:
        struct BoundingBox
        {
            std::uint32_t unknownInt;
            Ogre::Vector3 translation;
            Ogre::Matrix3 rotation;
            Ogre::Vector3 radius; // per direction
        };

        NiAVObject(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~NiAVObject() {}

        inline const Ogre::Matrix4& getWorldTransform() const { return mWorldTransform; }
        inline const Ogre::Matrix4& getLocalTransform() const { return mLocalTransform; }

        NiTimeController *findController(const std::string& controllerType);

    protected:
        std::uint16_t mFlags;

        Ogre::Vector3 mTranslation;
        Ogre::Matrix3 mRotation;
        float         mScale; // only uniform scaling

        Ogre::Matrix4 mWorldTransform; // includes local translation, rotation and scale
        Ogre::Matrix4 mLocalTransform;

        Ogre::Vector3 mVelocity; // unknown, to 4.2.2.0

        std::vector<NiPropertyRef> mProperty;

        bool mHasBoundingBox;       // to 4.2.2.0
        BoundingBox   mBoundingBox; // to 4.2.2.0

        NiCollisionObjectRef mCollisionObjectRef; // from 10.0.1.0
    };

    class NiCamera : public NiAVObject
    {
        float mFrustumLeft, mFrustumRight, mFrustumTop, mFrustumBottom, mFrustumNear, mFrustumFar;
        bool  mUseOrthographicProjection;
        float mViewportLeft, mViewportRight, mViewportTop, mViewportBottom;
        float mLODAdjust;

    public:
        NiCamera(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
        virtual ~NiCamera() {}

        void build(BuildData *data, NiObject *parentNiNode = nullptr);
    };

    struct NiDynamicEffect : public NiAVObject
    {
        bool mSwitchState;
        std::vector<NiAVObjectRef> mAffectedNodes;

        NiDynamicEffect(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        //virtual void build(BuildData *data, NiObject *parentNiNode = nullptr);
    };

    struct NiLight : public NiDynamicEffect
    {
        float mDimmer;
        Ogre::Vector3 mAmbientColor;
        Ogre::Vector3 mDiffuseColor;
        Ogre::Vector3 mSpecularColor;

        NiLight(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        //void build(BuildData *data, NiObject *parentNiNode = nullptr);
    };

    struct NiPointLight : public NiLight
    {
        float mAttenuationConstant;
        float mAttenuationLinear;
        float mAttenuationQuadratic;

        NiPointLight(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiLight NiAmbientLight;

    typedef NiLight NiDirectionalLight;

    struct NiTextureEffect : public NiDynamicEffect
    {
        Ogre::Matrix3 mModelProjectionMatrix;
        Ogre::Vector3 mModelProjectionTransform;

        std::uint32_t mTextureFiltering;
        std::uint32_t mTextureClamping;
        std::uint32_t mTextureType;
        std::uint32_t mCoordGenType;

        NiSourceTextureRef mSourceTexture;

        char mClippingPlane;

        NiTextureEffect(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        //void build(BuildData *data, NiObject *parentNiNode = nullptr);
    };
}

#endif // NIBTOGRE_NIAVOBJECT_H
