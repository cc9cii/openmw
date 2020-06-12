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

  Much of the material handling code is based on OpenMW version 0.36.

*/
#include "niproperty.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include "nistream.hpp"
#include "nimodel.hpp"
#include "nitimecontroller.hpp"
#include "ogrematerial.hpp"
#include "nidata.hpp" // BSShaderTextureSet

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

//#if 0
// Seen in NIF version 20.2.0.7
NiBtOgre::NiProperty::NiProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data, bool isBSLightingShaderProperty)
    : NiObjectNET(index, stream, model, data, isBSLightingShaderProperty)
{
}

void NiBtOgre::NiProperty::applyMaterialProperty(OgreMaterial& material,
                                                 std::vector<Ogre::Controller<float> >& controllers)
{
#if 0
    //std::cerr << "property not implemented: " << NiObject::mModel.getName() << std::endl;
    std::cerr << "unhandled property "
        << NiObject::mModel.indexToString(NiObjectNET::getNameIndex()) << std::endl;

    if (mControllerRef != -1)
        std::cerr << "unhandled controller " << NiObject::mModel.blockType(
                mModel.getRef<NiTimeController>(mControllerRef)->selfRef()) << std::endl;
#endif
}
//#endif

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderProperty::BSEffectShaderProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mShaderFlags1);
    stream->read(mShaderFlags2);
    stream->read(mUVOffset.u);
    stream->read(mUVOffset.v);
    stream->read(mUVScale.u);
    stream->read(mUVScale.v);
    stream->readSizedString(mSourceTexture);
    stream->read(mTextureClampMode);
    stream->read(mFalloffStartAngle);
    stream->read(mFalloffStopAngle);
    stream->read(mFalloffStartOpacity);
    stream->read(mFalloffStopOpacity);
    stream->read(mEmissiveColor);
    stream->read(mEmissiveMultiple);
    stream->read(mSoftFalloffDepth);
    stream->readSizedString(mGreyscaleTexture);
}

void NiBtOgre::BSEffectShaderProperty::applyMaterialProperty(OgreMaterial& material,
                                                             std::vector<Ogre::Controller<float> >& controllers)
{
    if (mSourceTexture == "")
        return;

    // FIXME: TEMP HACK
    NiTexturingProperty::TexDesc texDesc;
    texDesc.clampMode = mTextureClampMode;
    texDesc.uvSet = 0; // FIXME: material system needs re-written

    material.texName[NiTexturingProperty::Texture_Base] = mSourceTexture; // diffuse
    mTextureDescriptions[NiTexturingProperty::Texture_Base] = texDesc;

    material.textureDescriptions = &mTextureDescriptions;

    // FIXME: the rest of the stuff
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderProperty::BSLightingShaderProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data, true)
{
    if (stream->userVer() == 12)
    {
        stream->read(mShaderFlags1);
        stream->read(mShaderFlags2);
    }
    stream->read(mUVOffset.u);
    stream->read(mUVOffset.v);
    stream->read(mUVScale.u);
    stream->read(mUVScale.v);
    stream->read(mTextureSetRef);
    stream->read(mEmissiveColor);
    stream->read(mEmissiveMultiple);
    stream->read(mTextureClampMode);
    stream->read(mAlpha);
    stream->read(mRefractionStrength);
    stream->read(mGlossiness);
    stream->read(mSpecularColor);
    stream->read(mSpecularStrength);
    stream->read(mLightingEffect1);
    stream->read(mLightingEffect2);

    switch (mSkyrimShaderType)
    {
        case 1: // Environment Map
            stream->read(mEnvironmentMapScale);
            break;
        case 5: // Skin Tint
            stream->read(mSkinTintColor);
            break;
        case 6: // Hair Tint
            stream->read(mHairTintColor);
            break;
        case 7: // Parallax Occ Material
            stream->read(mMaxPasses);
            stream->read(mScale);
            break;
        case 11: // MultiLayer Parallax
            stream->read(mParallaxInnerLayerThickness);
            stream->read(mParallaxRefractionScale);
            stream->read(mParallaxInnerLayerTextureScale.u);
            stream->read(mParallaxInnerLayerTextureScale.v);
            stream->read(mParallaxEnvmapStrength);
            break;
        case 14: // Sparkle Snow
            stream->read(mSparkleParameters);
            break;
        case 16: // Eye Envmap
            stream->read(mEyeCubemapScale);
            stream->read(mLeftEyeReflectionCenter);
            stream->read(mRightEyeReflectionCenter);
            break;
        default:
            break;
    }
}

void NiBtOgre::BSLightingShaderProperty::applyMaterialProperty(OgreMaterial& material,
                                                               std::vector<Ogre::Controller<float> >& controllers)
{
    material.bsLightingShaderProperty = this;

    if ((mShaderFlags1 & 0x00001000) != 0) // msn
        return;
    material.alpha = mAlpha;

    // FIXME: TEMP HACK
    NiTexturingProperty::TexDesc texDesc;
    texDesc.clampMode = mTextureClampMode;
    texDesc.uvSet = 0; // FIXME: material system needs re-written


//#if 0
    BSShaderTextureSet* tset = mModel.getRef<BSShaderTextureSet>(mTextureSetRef);
    if (tset && tset->mTextures[0] != "")
    {
        material.texName[NiTexturingProperty::Texture_Base]    = tset->mTextures[0]; // diffuse
        mTextureDescriptions[NiTexturingProperty::Texture_Base] = texDesc;
    }

    if (tset->mTextures[1] != "")
    {
        if (tset->mTextures[1].find("_n.") != std::string::npos ||
            tset->mTextures[1].find("_N.") != std::string::npos)
        {
            material.texName[NiTexturingProperty::Texture_BumpMap] = tset->mTextures[1];
            mTextureDescriptions[NiTexturingProperty::Texture_BumpMap] = texDesc;
        }
        else if (0)
        {
            material.texName[NiTexturingProperty::Texture_Gloss] = tset->mTextures[1];
            mTextureDescriptions[NiTexturingProperty::Texture_Gloss] = texDesc;
        }
    }

    if (tset->mTextures[2] != "")
    {
        if (tset->mTextures[2].find("_sk.") != std::string::npos)
        {
            material.texName[NiTexturingProperty::Texture_Detail]  = tset->mTextures[2]; // skin
            mTextureDescriptions[NiTexturingProperty::Texture_Detail] = texDesc;
        }
        else if (tset->mTextures[2].find("_g.") != std::string::npos ||
                 tset->mTextures[2].find("_G.") != std::string::npos)
        {
            material.texName[NiTexturingProperty::Texture_Glow] = tset->mTextures[2]; // glow
            mTextureDescriptions[NiTexturingProperty::Texture_Glow] = texDesc;
        }
        else
            std::cout << "not skin nor glow " << tset->mTextures[2] << std::endl; //_hl.dds == hair?
    }

    if (tset->mTextures[6] != "")
    {
        std::string backlight = tset->mTextures[6];
        if (backlight.substr(1, 5) == "ata\\")
            material.texName[NiTexturingProperty::Texture_Normal]  = "t"+tset->mTextures[6].substr(6);
        else
            material.texName[NiTexturingProperty::Texture_Normal]  = tset->mTextures[6];

        mTextureDescriptions[NiTexturingProperty::Texture_Normal] = texDesc;
    }

    if (tset->mTextures[7] != "")
    {
        material.texName[NiTexturingProperty::Texture_Unknown2]  = tset->mTextures[7];

        mTextureDescriptions[NiTexturingProperty::Texture_Unknown2] = texDesc;
    }

    material.textureDescriptions = &mTextureDescriptions;
//#endif
}

const NiBtOgre::BSShaderTextureSet *NiBtOgre::BSLightingShaderProperty::getBSShaderTextureSet() const
{
    if (mTextureSetRef != -1)
        return mModel.getRef<BSShaderTextureSet>(mTextureSetRef);
    else
        return nullptr;
}

NiBtOgre::BSShaderLightingProperty::BSShaderLightingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
    stream->read(mShaderType);
    stream->read(mShaderFlags);
    stream->read(mUnknownInt2);

    if (stream->userVer() == 11)
        stream->read(mEnvmapScale);

    if (stream->userVer() <= 11)
        stream->read(mUnknownInt3);
}

NiBtOgre::BSShaderPPLightingProperty::BSShaderPPLightingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : BSShaderLightingProperty(index, stream, model, data)
{
    stream->read(mTextureSetRef);

    if (stream->userVer() == 11)
    {
        if (stream->userVer2() > 14)
        {
            stream->read(mUnknownFloat2);
            stream->read(mRefractionPeriod);
        }
        if (stream->userVer2() > 24)
        {
            stream->read(mUnknownFloat4);
            stream->read(mUnknownFloat5);
        }
    }

    if (stream->userVer() >= 12)
        stream->read(mEmissiveColor);
}

// FO3
void NiBtOgre::BSShaderPPLightingProperty::applyMaterialProperty(OgreMaterial& material,
                                                                 std::vector<Ogre::Controller<float> >& controllers)
{
    // FIXME: TEMP HACK
    NiTexturingProperty::TexDesc texDesc;
    texDesc.clampMode = mUnknownInt3;
    texDesc.uvSet = 0;

    BSShaderTextureSet* tset = mModel.getRef<BSShaderTextureSet>(mTextureSetRef);

    if (tset->mTextures[0] != "")
    {
        material.texName[NiTexturingProperty::Texture_Base]    = tset->mTextures[0]; // diffuse
        mTextureDescriptions[NiTexturingProperty::Texture_Base] = texDesc;
    }

  //material.texName[NiTexturingProperty::Texture_Dark]    = "";
    if (tset->mTextures[1] != "")
    {
        if (tset->mTextures[1].find("_n.") != std::string::npos ||
            tset->mTextures[1].find("_N.") != std::string::npos)
        {
            material.texName[NiTexturingProperty::Texture_Normal] = tset->mTextures[1];
            mTextureDescriptions[NiTexturingProperty::Texture_Normal] = texDesc;
        }
        else
        {
            material.texName[NiTexturingProperty::Texture_Gloss] = tset->mTextures[1];
            mTextureDescriptions[NiTexturingProperty::Texture_Gloss] = texDesc;
        }
    }
    if (tset->mTextures[2] != "")
    {
        if (tset->mTextures[2].find("_sk.") != std::string::npos)
        {
            //material.texName[NiTexturingProperty::Texture_Detail]  = tset->mTextures[2]; // skin
            //mTextureDescriptions[NiTexturingProperty::Texture_Detail] = texDesc;
        }
        else if (tset->mTextures[2].find("_g.") != std::string::npos ||
                 tset->mTextures[2].find("_G.") != std::string::npos)
        {
            material.texName[NiTexturingProperty::Texture_Glow] = tset->mTextures[2]; // glow
            mTextureDescriptions[NiTexturingProperty::Texture_Glow] = texDesc;
        }
        else if (tset->mTextures[2].find("_hl.") != std::string::npos)
        {
            // noop
        }
        else
            std::cout << "not skin, hair or glow " << tset->mTextures[2] << std::endl; // _hl.dds == hair?
    }
  //material.texName[NiTexturingProperty::Texture_BumpMap] = "";

                                                            //3: Height/Parallax
                                                            //4: Environment
                                                            //5: Environment Mask
                                                            //6: Subsurface for Multilayer Parallax
                                                            //7: Back Lighting Map (SLSF2_Back_Lighting)




    material.textureDescriptions = &mTextureDescriptions;
}

NiBtOgre::BSShaderNoLightingProperty::BSShaderNoLightingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : BSShaderLightingProperty(index, stream, model, data)
{
    stream->readSizedString(mFileName);

    if (stream->userVer() >= 11 && stream->userVer2() > 26)
    {
        stream->read(mUnknownFloat2);
        stream->read(mUnknownFloat3);
        stream->read(mUnknownFloat4);
        stream->read(mUnknownFloat5);
    }
}

void NiBtOgre::BSShaderNoLightingProperty::applyMaterialProperty(OgreMaterial& material,
                                                                 std::vector<Ogre::Controller<float> >& controllers)
{
    // FO3
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSWaterShaderProperty::BSWaterShaderProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mShaderFlags1);
    stream->read(mShaderFlags2);
    stream->read(mUVOffset.u);
    stream->read(mUVOffset.v);
    stream->read(mUVScale.u);
    stream->read(mUVScale.v);
    stream->read(mWaterShaderFlags);
    stream->read(mWaterDirection);
    stream->read(mUnknownS3);
}

void NiBtOgre::BSWaterShaderProperty::applyMaterialProperty(OgreMaterial& material,
                                                            std::vector<Ogre::Controller<float> >& controllers)
{
    // FIXME: TES5 water\water1024.nif
    std::cerr << "unhandled BSWaterShader property "
        //<< NiObject::mModel.indexToString(NiObjectNET::getNameIndex()) << std::endl;
        << mModel.getName() << std::endl;
}

NiBtOgre::NiAlphaProperty::NiAlphaProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
    stream->read(mThreshold);
}

void NiBtOgre::NiAlphaProperty::applyMaterialProperty(OgreMaterial& material,
                                                      std::vector<Ogre::Controller<float> >& controllers)
{
    material.alphaFlags = mFlags;
    material.alphaTest = mThreshold;
}

NiBtOgre::NiDitherProperty::NiDitherProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
}

NiBtOgre::NiFogProperty::NiFogProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
    stream->read(mFogDepth);
    stream->read(mFogColor);
}

NiBtOgre::NiMaterialProperty::NiMaterialProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    if (stream->nifVer() <= 0x0a000102) // 10.0.1.2
        stream->read(mFlags);

    if (!(stream->nifVer() == 0x14020007 && stream->userVer() >= 11 && stream->userVer2() > 21))
    {
        stream->read(mAmbientColor);
        stream->read(mDiffuseColor);
    }
    stream->read(mSpecularColor);
    stream->read(mEmissiveColor);
    stream->read(mGlossiness);
    stream->read(mAlpha);
    if (stream->nifVer() == 0x14020007 && stream->userVer() >= 11 && stream->userVer2() > 21)
        stream->read(mEmitMulti);
}

void NiBtOgre::NiMaterialProperty::applyMaterialProperty(OgreMaterial& material,
                                                         std::vector<Ogre::Controller<float> >& controllers)
{
    if (!(mModel.nifVer() == 0x14020007/* && mModel.userVer() >= 11 && mModel.userVer2() > 21*/))
    {
        material.ambient = mAmbientColor;
        material.diffuse = mDiffuseColor;
    }
    material.specular = mSpecularColor;
    material.emissive = mEmissiveColor;
    material.glossiness = mGlossiness;
    material.alpha = mAlpha;
    //material.emitmulti = mEmitMulti;

    // probably NiAlphaController or NiMaterialColorController
    NiTimeControllerRef controllerRef = NiObjectNET::mControllerRef;
    //while (controllerRef != -1)
    {




        // FIXME: testing only
        //std::cout << "prop controller " << mModel.blockType(controller->selfRef()) << std::endl;
        NiTimeController* controller = mModel.getRef<NiTimeController>(controllerRef);
        //controllerRef = controller->build(NiObjectNET::mControllers);




    }
}

NiBtOgre::NiShadeProperty::NiShadeProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
}

NiBtOgre::NiSpecularProperty::NiSpecularProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
}

NiBtOgre::NiStencilProperty::NiStencilProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    if (stream->nifVer() <= 0x0a000102) // 10.0.1.2
        stream->read(mFlags);

    if (stream->nifVer() <= 0x14000005) // not in FO3
    {
        stream->read(mStencilEnabled);
        stream->read(mStencilFunction);
        stream->read(mStencilRef);
        stream->read(mStencilMask);
        stream->read(mFailAction);
        stream->read(mZFailAction);
        stream->read(mZPassAction);
        stream->read(mDrawMode);
    }

    if (stream->nifVer() >= 0x14010003) // FO3
    {
        stream->read(mFlags); // NOTE: possibly different properties to above
        stream->read(mStencilRef);
        stream->read(mStencilMask);
    }
}

void NiBtOgre::NiStencilProperty::applyMaterialProperty(OgreMaterial& material,
                                                        std::vector<Ogre::Controller<float> >& controllers)
{
    material.drawMode = mDrawMode;
    // FIXME: what to do with other properties?
}

void NiBtOgre::NiTexturingProperty::TexDesc::read(NiStream *stream)
{
    stream->read(mSourceRef);

    if (stream->nifVer() <= 0x14000005) // up to 20.0.0.5
    {
        stream->read(clampMode);
        stream->read(filterMode);
    }
    else
    {
        clampMode = 0;
        filterMode = 0;
    }

    if (stream->nifVer() >= 0x14010003) // from 20.1.0.3
        stream->read(flags);
    else
        flags = 0;

    //if (stream->nifVer() >= 0x14060000) // from 20.6.0.0 (unsupported version, commented out)
        //stream->read(unknownShort);

    if (stream->nifVer() <= 0x14000005) // up to 20.0.0.5
        stream->read(uvSet);

    if (stream->nifVer() <= 0x0a040001) // up to 10.4.0.1
    {
        stream->skip(sizeof(std::int16_t)*2);
    }

    if (stream->nifVer() <= 0x0401000c) // up to 4.1.0.12
        stream->skip(sizeof(std::uint16_t)); // Unknown1

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        if (hasTextureTransform = stream->getBool())
        {
            stream->read(translation.u);
            stream->read(translation.v);
            stream->read(tiling.u);
            stream->read(tiling.v);
            stream->read(wRotation);
            stream->read(transformType);
            stream->read(centerOffset.u);
            stream->read(centerOffset.v);
        }
    }
    else
        hasTextureTransform = false;
}

NiBtOgre::NiTexturingProperty::NiTexturingProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data), mFlags(0), mApplyMode(/* APPLY_MODULATE */2)/*, mHasNormalTexture(false),
      mHasUnknown2Texture(false), mHasDecal1Texture(false), mHasDecal2Texture(false), mHasDecal3Texture(false)*/
{
    if (!stream) // must be a dummy block being inserted
    {
        mTextureCount = 7;

        TexDesc td;
        NiObject *tex
            = const_cast<NiModel&>(model).insertDummyBlock<NiSourceTexture>("NiSourceTexture"); // const hack
        td.mSourceRef = tex->selfRef();

        // FIXME: find suitable default values
        td.clampMode = 3;
        td.filterMode = 2;
        td.uvSet = 0;
        td.hasTextureTransform = false;

        mTextureDescriptions[Texture_Base] = td;

        return;
    }

    if (stream->nifVer() <= 0x0a000102 || stream->nifVer() >= 0x14010003) // up to 10.0.1.2 or from 20.1.0.3
        stream->read(mFlags);

    if (stream->nifVer() <= 0x14000005) // up to 20.0.0.5
        stream->read(mApplyMode);

    stream->read(mTextureCount);
#if 0
    if (mHasBaseTexture = stream->getBool())
        mBaseTexture.read(stream);

    if (mHasDarkTexture = stream->getBool())
        mDarkTexture.read(stream);

    if (mHasDetailTexture = stream->getBool())
        mDetailTexture.read(stream);

    if (mHasGlossTexture = stream->getBool())
        mGlossTexture.read(stream);

    if (mHasGlowTexture = stream->getBool())
        mGlowTexture.read(stream);

    if (mHasBumpMapTexture = stream->getBool())
    {
        mBumpMapTexture.read(stream);

        stream->skip(sizeof(float));   // Bump Map Luma Scale
        stream->skip(sizeof(float));   // Bump Map Luma Offset
        stream->skip(sizeof(float)*4); // Bump Map Matrix
    }

    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7
    {
        if (mHasNormalTexture = stream->getBool())
            mNormalTexture.read(stream);

        if (mHasUnknown2Texture = stream->getBool())
        {
            mUnknown2Texture.read(stream);
            stream->skip(sizeof(float));
        }
    }

    if (mHasDecal0Texture = stream->getBool())
        mDecal0Texture.read(stream);

    unsigned int offset = 0;
    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7
        offset = 2;

    if (mTextureCount >= 8+offset)
        if (mHasDecal1Texture = stream->getBool())
            mDecal1Texture.read(stream);

    if (mTextureCount >= 9+offset)
        if (mHasDecal2Texture = stream->getBool())
            mDecal2Texture.read(stream);

    if (mTextureCount >= 10+offset)
        if (mHasDecal3Texture = stream->getBool())
            mDecal3Texture.read(stream);
#else
    if (stream->getBool())
        mTextureDescriptions[Texture_Base].read(stream);

    if (stream->getBool())
        mTextureDescriptions[Texture_Dark].read(stream);

    if (stream->getBool())
        mTextureDescriptions[Texture_Detail].read(stream);

    if (stream->getBool())
        mTextureDescriptions[Texture_Gloss].read(stream);

    if (stream->getBool())
        mTextureDescriptions[Texture_Glow].read(stream);

    if (stream->getBool())
    {
        mTextureDescriptions[Texture_BumpMap].read(stream);

        stream->skip(sizeof(float));   // Bump Map Luma Scale
        stream->skip(sizeof(float));   // Bump Map Luma Offset
        stream->skip(sizeof(float)*4); // Bump Map Matrix
    }

    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7
    {
        if (stream->getBool())
            mTextureDescriptions[Texture_Normal].read(stream);

        if (stream->getBool())
        {
            mTextureDescriptions[Texture_Unknown2].read(stream);
            stream->skip(sizeof(float));
        }
    }

    if (stream->getBool())
        mTextureDescriptions[Texture_Decal0].read(stream);

    unsigned int offset = 0;
    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7
        offset = 2;

    if (mTextureCount >= 8+offset)
        if (stream->getBool())
            mTextureDescriptions[Texture_Decal1].read(stream);

    if (mTextureCount >= 9+offset)
        if (stream->getBool())
            mTextureDescriptions[Texture_Decal2].read(stream);

    if (mTextureCount >= 10+offset)
        if (stream->getBool())
            mTextureDescriptions[Texture_Decal3].read(stream);
#endif
    if (stream->nifVer() >= 0x0a000100) // 10.0.1.0
    {
        std::uint32_t numShaderTextures;
        stream->read(numShaderTextures);
        mShaderTextures.resize(numShaderTextures);
        for (unsigned int i = 0; i < numShaderTextures; ++i)
        {
            mShaderTextures[i].textureData.read(stream);
            stream->read(mShaderTextures[i].mapIndex);
        }
    }
}

void NiBtOgre::NiTexturingProperty::applyMaterialProperty(OgreMaterial& material,
                                                          std::vector<Ogre::Controller<float> >& controllers)
{
    std::map<TextureType, TexDesc>::const_iterator iter = mTextureDescriptions.begin();
    for (; iter != mTextureDescriptions.end(); ++iter)
    {
        // /FIXME: throw if iter->second.mSourceRef is -1
        const NiSourceTexture *src = mModel.getRef<NiSourceTexture>(iter->second.mSourceRef);
        if (src && src->mUseExternal)
            material.texName[iter->first] = mModel.indexToString(src->mFileName);

        // FIXME: testing
        if (material.texName[iter->first].find("EarsHuman") != std::string::npos)
            material.texName[iter->first] = "textures\\characters\\imperial\\earshuman.dds";
    }

    material.textureDescriptions = &mTextureDescriptions;

    // probably NiFlipController or NiTextureTransformController (ascensionparticles.nif)
    // COW "Tamriel" 2 13  (meshes/fire/firetorchlargesmoke.nif)
    NiTimeControllerRef controllerRef = NiObjectNET::mControllerRef;
    //while (controllerRef != -1)
    {




        // FIXME: testing only
        //std::cout << "prop controller " << mModel.blockType(controller->selfRef()) << std::endl;
        NiTimeController* controller = mModel.getRef<NiTimeController>(controllerRef);
        //controllerRef = controller->build(NiObjectNET::mControllers);




    }
}

NiBtOgre::NiVertexColorProperty::NiVertexColorProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
    stream->read(mVertexMode);
    stream->read(mLightingMode);
}

void NiBtOgre::NiVertexColorProperty::applyMaterialProperty(OgreMaterial& material,
                                                            std::vector<Ogre::Controller<float> >& controllers)
{
    // mFlags?
    if (mModel.getName().find("air") == std::string::npos)
    material.vertMode = mVertexMode;
    else
        material.vertMode = 0;
    material.lightMode = mLightingMode;
}

NiBtOgre::NiWireframeProperty::NiWireframeProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);
}

void NiBtOgre::NiWireframeProperty::applyMaterialProperty(OgreMaterial& material,
                                                          std::vector<Ogre::Controller<float> >& controllers)
{
    material.wireFlags = mFlags;
}

// either NiGeometry or NiBillboardNode
NiBtOgre::NiZBufferProperty::NiZBufferProperty(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiProperty(index, stream, model, data)
{
    stream->read(mFlags);

    if (stream->nifVer() >= 0x0401000c && stream->nifVer() <= 0x14000005) // 4.1.0.12 to 20.0.0.5
        stream->read(mFunction);
}

void NiBtOgre::NiZBufferProperty::applyMaterialProperty(OgreMaterial& material,
                                                        std::vector<Ogre::Controller<float> >& controllers)
{
    material.depthFlags = mFlags;
}
