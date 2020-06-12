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
#ifndef NIBTOGRE_NIPROPERTY_H
#define NIBTOGRE_NIPROPERTY_H

#include <string>
#include <vector>

#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreVector4.h>

#include "niobjectnet.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiProperty
//         BSEffectShaderProperty
//         BSLightingShaderProperty
//         BSShaderProperty <----------------------- /* Not implemented */
//             BSShaderLightingProperty
//                 BSShaderPPLightingProperty
//                 BSShaderNoLightingProperty
//         BSSkyShaderProperty <-------------------- /* TODO */
//         BSWaterShaderProperty
//         NiAlphaProperty
//         NiDitherProperty
//         NiFogProperty
//         NiMaterialProperty
//         NiShadeProperty
//         NiSpecularProperty
//         NiStencilProperty
//         NiTexturingProperty
//         NiVertexColorProperty
//         NiWireframeProperty
//         NiZBufferProperty
namespace NiBtOgre
{
    struct OgreMaterial;

    // Seen in NIF version 20.2.0.7
    class NiProperty : public NiObjectNET
    {
    //protected: // 25/02/20  this does not belong here
        //bool mIsBSLightingShaderProperty; // 25/02/20  this does not belong here

    public:
        NiProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isBSLightingShaderProperty = false);

        virtual void applyMaterialProperty(OgreMaterial& material, std::vector<Ogre::Controller<float> >& controllers);
    };
    //typedef NiObjectNET NiProperty; // Seen in NIF version 20.2.0.7

    struct NiTexturingProperty : public NiProperty
    {
        enum TextureType {
            Texture_Base     =  0,
            Texture_Dark     =  1,
            Texture_Detail   =  2,
            Texture_Gloss    =  3,
            Texture_Glow     =  4,
            Texture_BumpMap  =  5,
            Texture_Normal   =  6,
            Texture_Unknown2 =  7,
            Texture_Decal0   =  8,
            Texture_Decal1   =  9,
            Texture_Decal2   = 10,
            Texture_Decal3   = 11
        };

        struct TexDesc
        {
            typedef std::uint32_t TexClampMode;
            typedef std::uint32_t TexFilterMode;

            NiSourceTextureRef mSourceRef;

            TexClampMode clampMode;
            TexFilterMode filterMode;
            std::uint32_t uvSet;
            std::uint16_t flags;
            std::int16_t unknownShort;

            bool hasTextureTransform;
            TexCoord translation;
            TexCoord tiling;
            float wRotation;
            std::uint32_t transformType;
            TexCoord centerOffset;

            void read(NiStream *stream);
        };

        std::uint16_t mFlags;
        std::uint32_t mApplyMode;
        std::uint32_t mTextureCount;

        struct ShaderTexDesc
        {
            bool isused;
            TexDesc textureData;
            std::uint32_t mapIndex;
        };
#if 0
        bool mHasBaseTexture;
        TexDesc mBaseTexture;

        bool mHasDarkTexture;
        TexDesc mDarkTexture;

        bool mHasDetailTexture;
        TexDesc mDetailTexture;

        bool mHasGlossTexture;
        TexDesc mGlossTexture;

        bool mHasGlowTexture;
        TexDesc mGlowTexture;

        bool mHasBumpMapTexture;
        TexDesc mBumpMapTexture;

        bool mHasNormalTexture;
        TexDesc mNormalTexture;

        bool mHasUnknown2Texture;
        TexDesc mUnknown2Texture;

        bool mHasDecal0Texture;
        TexDesc mDecal0Texture;

        bool mHasDecal1Texture;
        TexDesc mDecal1Texture;

        bool mHasDecal2Texture;
        TexDesc mDecal2Texture;

        bool mHasDecal3Texture;
        TexDesc mDecal3Texture;
#else
        std::map<TextureType, TexDesc> mTextureDescriptions;
#endif
        std::vector<ShaderTexDesc> mShaderTextures;

        NiTexturingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    // Seen in NIF version 20.2.0.7
    class BSEffectShaderProperty : public NiProperty
    {
    public:
        std::uint32_t mShaderFlags1;
        std::uint32_t mShaderFlags2;
        TexCoord mUVOffset;
        TexCoord mUVScale;
        std::string mSourceTexture;
        std::uint32_t mTextureClampMode;
        float mFalloffStartAngle;
        float mFalloffStopAngle;
        float mFalloffStartOpacity;
        float mFalloffStopOpacity;
        Ogre::Vector4 mEmissiveColor;
        float mEmissiveMultiple;
        float mSoftFalloffDepth;
        std::string mGreyscaleTexture;

        // FIXME
        std::map<NiTexturingProperty::TextureType, NiTexturingProperty::TexDesc> mTextureDescriptions;

        BSEffectShaderProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct BSShaderTextureSet;

    // Seen in NIF version 20.2.0.7
    class BSLightingShaderProperty : public NiProperty
    {
    public:
        std::uint32_t mShaderFlags1;
        std::uint32_t mShaderFlags2;
        TexCoord mUVOffset;
        TexCoord mUVScale;
        BSShaderTextureSetRef mTextureSetRef;
        Ogre::Vector3 mEmissiveColor;
        float mEmissiveMultiple;
        std::uint32_t mTextureClampMode;
        float mAlpha;
        float mRefractionStrength;
        float mGlossiness;
        Ogre::Vector3 mSpecularColor;
        float mSpecularStrength;
        float mLightingEffect1;
        float mLightingEffect2;

        float mEnvironmentMapScale;
        Ogre::Vector3 mSkinTintColor;
        Ogre::Vector3 mHairTintColor;
        float mMaxPasses;
        float mScale;
        float mParallaxInnerLayerThickness;
        float mParallaxRefractionScale;
        TexCoord mParallaxInnerLayerTextureScale;
        float mParallaxEnvmapStrength;
        Ogre::Vector4 mSparkleParameters;
        float mEyeCubemapScale;
        Ogre::Vector3 mLeftEyeReflectionCenter;
        Ogre::Vector3 mRightEyeReflectionCenter;

        // FIXME
        std::map<NiTexturingProperty::TextureType, NiTexturingProperty::TexDesc> mTextureDescriptions;

        BSLightingShaderProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);

        std::uint32_t getSkyrimShaderType() const { return NiObjectNET::mSkyrimShaderType; }

        const BSShaderTextureSet *getBSShaderTextureSet() const;
    };

    class BSShaderLightingProperty : public NiProperty
    {
    public:
        std::uint16_t mFlags;
        std::uint32_t mShaderType;
        std::uint32_t mShaderFlags;
        std::int32_t  mUnknownInt2;
        float         mEnvmapScale;
        std::int32_t  mUnknownInt3;

        // FIXME
        std::map<NiTexturingProperty::TextureType, NiTexturingProperty::TexDesc> mTextureDescriptions;

        BSShaderLightingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // FO3
    class BSShaderPPLightingProperty : public BSShaderLightingProperty
    {
    public:
        BSShaderTextureSetRef mTextureSetRef;
        float         mUnknownFloat2;
        std::int32_t  mRefractionPeriod;
        float         mUnknownFloat4;
        float         mUnknownFloat5;
        Ogre::Vector4 mEmissiveColor;

        BSShaderPPLightingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    // FO3
    class BSShaderNoLightingProperty : public BSShaderLightingProperty
    {
    public:
        std::string mFileName;

        float mUnknownFloat2;
        float mUnknownFloat3;
        float mUnknownFloat4;
        float mUnknownFloat5;

        BSShaderNoLightingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    // Seen in NIF version 20.2.0.7
    class BSWaterShaderProperty : public NiProperty
    {
    public:
        std::uint32_t mShaderFlags1;
        std::uint32_t mShaderFlags2;
        TexCoord mUVOffset;
        TexCoord mUVScale;
        unsigned char mWaterShaderFlags;
        unsigned char mWaterDirection;
        std::uint16_t mUnknownS3;

        BSWaterShaderProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct NiAlphaProperty : public NiProperty
    {
        std::uint16_t mFlags;
        unsigned char mThreshold;

        NiAlphaProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct NiDitherProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiDitherProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiFogProperty : public NiProperty
    {
        std::uint16_t mFlags;
        float         mFogDepth;
        Ogre::Vector3 mFogColor;

        NiFogProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiMaterialProperty : public NiProperty
    {
        std::uint16_t mFlags;

        Ogre::Vector3 mAmbientColor;
        Ogre::Vector3 mDiffuseColor;
        Ogre::Vector3 mSpecularColor;
        Ogre::Vector3 mEmissiveColor;
        float mGlossiness;
        float mAlpha;
        float mEmitMulti;

        NiMaterialProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct NiShadeProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiShadeProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiSpecularProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiSpecularProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    struct NiStencilProperty : public NiProperty
    {
        std::uint16_t mFlags;

        unsigned char mStencilEnabled;

        std::uint32_t mStencilFunction;
        std::uint32_t mStencilRef;
        std::uint32_t mStencilMask;
        std::uint32_t mFailAction;
        std::uint32_t mZFailAction;
        std::uint32_t mZPassAction;
        std::uint32_t mDrawMode;

        NiStencilProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct NiVertexColorProperty : public NiProperty
    {
        std::uint16_t mFlags;
        std::uint32_t mVertexMode;
        std::uint32_t mLightingMode;

        NiVertexColorProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct NiWireframeProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiWireframeProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };

    struct NiZBufferProperty : public NiProperty
    {
        std::uint16_t mFlags;
        std::uint32_t mFunction;

        NiZBufferProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void applyMaterialProperty(OgreMaterial& material,
                                           std::vector<Ogre::Controller<float> >& controllers);
    };
}

#endif // NIBTOGRE_NIPROPERTY_H
