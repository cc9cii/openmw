/*
  Copyright (C) 2018-2020 cc9cii

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

  Much of below code is copied from components/nifogre/material.cpp

*/
#include "ogrematerial.hpp"

#include <iostream>

#include <boost/functional/hash.hpp>

#include <OgreMaterialManager.h>
#include <OgreResourceGroupManager.h>

#include <extern/shiny/Main/Factory.hpp>

#include <components/nifoverrides/nifoverrides.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "nidata.hpp" // BSShaderTextureSet
#include "nimodel.hpp"
//#include "niobjectnet.hpp" // SkyrimShaderType

namespace
{
// Conversion of blend / test mode from NIF
static const char *getBlendFactor(int mode)
{
    switch(mode)
    {
    case 0: return "one";
    case 1: return "zero";
    case 2: return "src_colour";
    case 3: return "one_minus_src_colour";
    case 4: return "dest_colour";
    case 5: return "one_minus_dest_colour";
    case 6: return "src_alpha";
    case 7: return "one_minus_src_alpha";
    case 8: return "dest_alpha";
    case 9: return "one_minus_dest_alpha";
    case 10: return "src_alpha_saturate";
    }
    std::cerr << "Unexpected blend mode: "<< mode <<std::endl;
    return "src_alpha";
}

static const char *getTestMode(int mode)
{
    switch(mode)
    {
    case 0: return "always_pass";
    case 1: return "less";
    case 2: return "equal";
    case 3: return "less_equal";
    case 4: return "greater";
    case 5: return "not_equal";
    case 6: return "greater_equal";
    case 7: return "always_fail";
    }
    std::cerr<< "Unexpected test mode: "<<mode <<std::endl;
    return "less_equal";
}

static void setTextureProperties(sh::MaterialInstance* material,
                                 const std::string& textureSlotName,
                                 const NiBtOgre::NiTexturingProperty::TexDesc& tex)
{
    material->setProperty(textureSlotName + "UVSet", sh::makeProperty(new sh::IntValue(tex.uvSet)));
    const std::string clampMode = textureSlotName + "ClampMode";
    switch (tex.clampMode)
    {
    case 0:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("clamp clamp")));
        break;
    case 1:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("clamp wrap")));
        break;
    case 2:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("wrap clamp")));
        break;
    case 3:
    default:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("wrap wrap")));
        break;
    }
}
}

namespace NiBtOgre
{
std::map<size_t, std::string> OgreMaterial::sMaterialMap;

void NiBtOgre::OgreMaterial::setExternalTexture(const std::string& texture)
{
    texName[NiTexturingProperty::Texture_Base] = texture;

    size_t pos = texture.find_last_of(".");
    if (pos == std::string::npos)
        return; // FIXME: should throw

    std::string normalTexture = texture.substr(0, pos) + "_n.dds";
    if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(normalTexture))
        texName[NiTexturingProperty::Texture_BumpMap] = normalTexture;
}

//
//
// FIXME: need to cleanup, especially all that repeated searching
//
//
std::string NiBtOgre::OgreMaterial::getOrCreateMaterial(const std::string& name)
{
    Ogre::MaterialManager &materialManager = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr material = materialManager.getByName(name);
    if(material)
        return name;

    // FIXME: just testing msn for now
    if (bsLightingShaderProperty && (bsLightingShaderProperty->mShaderFlags1 & 0x00001000) != 0)
        return getOrCreateLSMaterial(name);

    // Generate a hash out of all properties that can affect the material.
    size_t h = 0;
    boost::hash_combine(h, ambient.x);
    boost::hash_combine(h, ambient.y);
    boost::hash_combine(h, ambient.z);
    boost::hash_combine(h, diffuse.x);
    boost::hash_combine(h, diffuse.y);
    boost::hash_combine(h, diffuse.z);
    boost::hash_combine(h, alpha);
    boost::hash_combine(h, specular.x);
    boost::hash_combine(h, specular.y);
    boost::hash_combine(h, specular.z);
    boost::hash_combine(h, glossiness);
    boost::hash_combine(h, emissive.x);
    boost::hash_combine(h, emissive.y);
    boost::hash_combine(h, emissive.z);

    std::map<NiTexturingProperty::TextureType, std::string>::const_iterator iter = texName.begin();
    for (; iter != texName.end(); ++iter)
    {
        if(!iter->second.empty())
        {
            boost::hash_combine(h, Misc::ResourceHelpers::correctTexturePath(iter->second));
            boost::hash_combine(h, (*textureDescriptions)[iter->first].clampMode);
            boost::hash_combine(h, (*textureDescriptions)[iter->first].uvSet);
        }
    }
    boost::hash_combine(h, drawMode);
    boost::hash_combine(h, vertexColor);
    boost::hash_combine(h, alphaFlags);
    boost::hash_combine(h, alphaTest);
    boost::hash_combine(h, vertMode);
    boost::hash_combine(h, depthFlags);
    boost::hash_combine(h, specFlags);
    boost::hash_combine(h, wireFlags);

    std::map<size_t,std::string>::iterator itr = sMaterialMap.find(h);
    if (itr != sMaterialMap.end())
    {
        // a suitable material exists already - use it
        sh::MaterialInstance* instance = sh::Factory::getInstance().getMaterialInstance(itr->second);
        mNeedTangents
            = !sh::retrieveValue<sh::StringValue>(instance->getProperty("normalMap"), instance).get().empty();

        return itr->second;
    }

    // not found, create a new one
    sMaterialMap.insert(std::make_pair(h, name));

    // No existing material like this. Create a new one.
#if 0
    sh::MaterialInstance *instance = nullptr;
    if (texName.find(NiTexturingProperty::Texture_BumpMap) != texName.end() &&
            texName[NiTexturingProperty::Texture_BumpMap].find("_msn.dds") != std::string::npos)
        instance = sh::Factory::getInstance().createMaterialInstance(name, "openmw_tes5objects_base");
    else
        instance = sh::Factory::getInstance().createMaterialInstance(name, "openmw_objects_base");
#else
    sh::MaterialInstance *instance = sh::Factory::getInstance().createMaterialInstance(name, "openmw_objects_base");
#endif

    // some NIF (20.0.0.4 of clutter/farm/hoe01.nif) have NiTriStrips with no NiTexturingProperty
    if (texName.find(NiTexturingProperty::Texture_Base) != texName.end())
    {
        instance->setProperty("diffuseMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Base])));
        instance->setProperty("use_diffuse_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "diffuseMap", (*textureDescriptions)[NiTexturingProperty::Texture_Base]);
#if 0
        else if (bsprop)
        {
            Nif::NiTexturingProperty::Texture tex; // FIXME hack
            tex.uvSet = 0; // ???
            tex.clamp = bsprop->textureClampMode;
            setTextureProperties(instance, "diffuseMap", tex);
        }
        else if (prop)
        {
            Nif::NiTexturingProperty::Texture tex; // FIXME hack
            tex.uvSet = 0; // ???
            tex.clamp = 0; // ???
            setTextureProperties(instance, "diffuseMap", tex);
        }
#endif

        // Override alpha flags based on our override list (transparency-overrides.cfg)
        if ((alphaFlags&1))
        {
            NifOverrides::TransparencyResult result
                = NifOverrides::Overrides::getTransparencyOverride(texName[NiTexturingProperty::Texture_Base]);
            if (result.first)
            {
                alphaFlags = (1<<9) | (6<<10); /* alpha_rejection enabled, greater_equal */
                alphaTest = result.second;
                depthFlags = (1<<0) | (1<<1); // depth_write on, depth_check on
            }
        }

        NifOverrides::Overrides::getMaterialOverrides(texName[NiTexturingProperty::Texture_Base], instance);
    }
    else
        instance->setProperty("diffuseMap", sh::makeProperty(texName[NiTexturingProperty::Texture_Base]));

    if (texName.find(NiTexturingProperty::Texture_Dark) != texName.end())
    {
        instance->setProperty("darkMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Dark])));
        instance->setProperty("use_dark_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "darkMap", (*textureDescriptions)[NiTexturingProperty::Texture_Dark]);
    }
    else
        instance->setProperty("darkMap", sh::makeProperty(texName[NiTexturingProperty::Texture_Dark]));

    if (texName.find(NiTexturingProperty::Texture_Detail) != texName.end())
    {
        instance->setProperty("detailMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Detail])));
        instance->setProperty("use_detail_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "detailMap", (*textureDescriptions)[NiTexturingProperty::Texture_Detail]);
    }
    else
        instance->setProperty("detailMap", sh::makeProperty(texName[NiTexturingProperty::Texture_Detail]));

#if 0
    // FIXME: temp hack for TES5
    if (texName.find(NiTexturingProperty::Texture_Normal) != texName.end())
    {
        instance->setProperty("tintMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Normal])));

        // FIXME: prob not necessary
        instance->setProperty("use_tint_map", sh::makeProperty(new sh::BooleanValue(true)));

        setTextureProperties(instance, "tintMap", (*textureDescriptions)[NiTexturingProperty::Texture_Normal]);

        // FIXME
        std::cout << "tint "
            << Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Normal])
            << std::endl;
        //std::cout << "tint "
            //<< texName[NiTexturingProperty::Texture_Normal] << std::endl;
    }
    else
        instance->setProperty("tintMap", sh::makeProperty(texName[NiTexturingProperty::Texture_Normal]));

    // FIXME: temp hack for TES5
    if (texName.find(NiTexturingProperty::Texture_Unknown2) != texName.end())
    {
        instance->setProperty("backlightMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Unknown2])));

        // FIXME: prob not necessary
        instance->setProperty("use_backlight_map", sh::makeProperty(new sh::BooleanValue(true)));
        instance->setProperty("diffuseMapUVSet", sh::makeProperty(new sh::FloatValue(0)));

        setTextureProperties(instance, "backlightMap", (*textureDescriptions)[NiTexturingProperty::Texture_Unknown2]);

        // FIXME
        std::cout << "backlight "
            << Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Unknown2])
            << std::endl;
        if (texName[NiTexturingProperty::Texture_Unknown2] == "")
            std::cout << "backlight "
                << texName[NiTexturingProperty::Texture_Unknown2] << std::endl;
    }
    else
        instance->setProperty("backlightMap", sh::makeProperty(texName[NiTexturingProperty::Texture_Unknown2]));
#endif

    if (texName.find(NiTexturingProperty::Texture_Glow) != texName.end())
    {
        instance->setProperty("emissiveMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_Glow])));
        instance->setProperty("use_emissive_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "emissiveMap", (*textureDescriptions)[NiTexturingProperty::Texture_Glow]);
#if 0
        else if (bsprop)
        {
            Nif::NiTexturingProperty::Texture tex; // FIXME hack
            tex.uvSet = 0; // ???
            tex.clamp = bsprop->textureClampMode;
            setTextureProperties(instance, "emissiveMap", tex);
        }
        else if (prop)
        {
            Nif::NiTexturingProperty::Texture tex; // FIXME hack
            tex.uvSet = 0; // ???
            tex.clamp = 0; // ???
            setTextureProperties(instance, "emissiveMap", tex);
        }
#endif
    }
    else
        instance->setProperty("emissiveMap", sh::makeProperty(texName[NiTexturingProperty::Texture_Glow]));

    if (texName.find(NiTexturingProperty::Texture_BumpMap) != texName.end())
    {
        instance->setProperty("normalMap", sh::makeProperty(
                Misc::ResourceHelpers::correctTexturePath(texName[NiTexturingProperty::Texture_BumpMap])));
        bool useParallax = (texName[NiTexturingProperty::Texture_BumpMap].find("_nh.") != std::string::npos
                || texName[NiTexturingProperty::Texture_BumpMap].find("_n.") != std::string::npos
                || texName[NiTexturingProperty::Texture_BumpMap].find("_N.") != std::string::npos); // FIXME HACK
        instance->setProperty("use_parallax", sh::makeProperty(new sh::BooleanValue(useParallax)));
    }
    else
        instance->setProperty("normalMap", sh::makeProperty(texName[NiTexturingProperty::Texture_BumpMap]));

#if 0
    for(int i = 0;i < 7;i++)
    {
        if(i == Nif::NiTexturingProperty::BaseTexture ||
           i == Nif::NiTexturingProperty::DetailTexture ||
           i == Nif::NiTexturingProperty::DarkTexture ||
           i == Nif::NiTexturingProperty::BumpTexture ||
           i == Nif::NiTexturingProperty::GlowTexture)
            continue;
        if(!texName[i].empty())
            warn("Ignored texture "+texName[i]+" on layer "+Ogre::StringConverter::toString(i) + " in " + name);
    }
#endif

    if (drawMode == 1)
        instance->setProperty("cullmode", sh::makeProperty(new sh::StringValue("clockwise")));
    else if (drawMode == 2)
        instance->setProperty("cullmode", sh::makeProperty(new sh::StringValue("anticlockwise")));
    else if (drawMode == 3)
        instance->setProperty("cullmode", sh::makeProperty(new sh::StringValue("none")));

    if (vertexColor)
        instance->setProperty("has_vertex_colour", sh::makeProperty(new sh::BooleanValue(true)));

    // Add transparency if NiAlphaProperty was present
    if((alphaFlags&1))
    {
        std::string blend_mode;
        blend_mode += getBlendFactor((alphaFlags>>1)&0xf);
        blend_mode += " ";
        blend_mode += getBlendFactor((alphaFlags>>5)&0xf);
        instance->setProperty("scene_blend", sh::makeProperty(new sh::StringValue(blend_mode)));
    }

    if((alphaFlags>>9)&1)
    {
        std::string reject;
        reject += getTestMode((alphaFlags>>10)&0x7);
        reject += " ";
        reject += std::to_string(alphaTest);
        instance->setProperty("alpha_rejection", sh::makeProperty(new sh::StringValue(reject)));
    }
    else
        instance->getMaterial()->setShadowCasterMaterial("openmw_shadowcaster_noalpha");

    // Ogre usually only sorts if depth write is disabled, so we want "force" instead of "on"
    instance->setProperty("transparent_sorting", sh::makeProperty(new sh::StringValue(
        ((alphaFlags&1) && !((alphaFlags>>13)&1)) ? "force" : "off")));

    instance->setProperty("depth_check", sh::makeProperty(new sh::StringValue((depthFlags&1) ? "on" : "off")));
    instance->setProperty("depth_write", sh::makeProperty(new sh::StringValue(((depthFlags>>1)&1) ? "on" : "off")));
    // depth_func???

    if(vertMode == 0 || !vertexColor)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::Vector4(ambient.x, ambient.y, ambient.z, 1)));
        instance->setProperty("diffuse", sh::makeProperty(new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));
        instance->setProperty("emissive", sh::makeProperty(new sh::Vector4(emissive.x, emissive.y, emissive.z, 1)));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("0")));
    }
    else if(vertMode == 1)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::Vector4(ambient.x, ambient.y, ambient.z, 1)));
        instance->setProperty("diffuse", sh::makeProperty(new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));
        instance->setProperty("emissive", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("1")));
    }
    else if(vertMode == 2)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("diffuse", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("emissive", sh::makeProperty(new sh::Vector4(emissive.x, emissive.y, emissive.z, 1)));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("2")));
    }
    else
        std::cerr << "Unhandled vertex mode: " << vertMode << std::endl;

    if(specFlags)
    {
        instance->setProperty("specular",
                sh::makeProperty(new sh::Vector4(specular.x, specular.y, specular.z, glossiness)));
    }

    if(wireFlags)
    {
        instance->setProperty("polygon_mode", sh::makeProperty(new sh::StringValue("wireframe")));
    }

    // Don't use texName, as it may be overridden
    mNeedTangents = !sh::retrieveValue<sh::StringValue>(instance->getProperty("normalMap"), instance).get().empty()
        && (texName[NiTexturingProperty::Texture_BumpMap].find("_msn.") != std::string::npos); // FIXME: quick test

    return name;
}

std::string NiBtOgre::OgreMaterial::getOrCreateLSMaterial(const std::string& name)
{
    // Generate a hash out of all properties that can affect the material.
    size_t h = 0;
    boost::hash_combine(h, bsLightingShaderProperty->getSkyrimShaderType());
    // TODO: extra data?
    boost::hash_combine(h, bsLightingShaderProperty->mShaderFlags1);
    boost::hash_combine(h, bsLightingShaderProperty->mShaderFlags2);
    boost::hash_combine(h, bsLightingShaderProperty->mUVOffset.u);
    boost::hash_combine(h, bsLightingShaderProperty->mUVOffset.v);
    boost::hash_combine(h, bsLightingShaderProperty->mUVScale.u);
    boost::hash_combine(h, bsLightingShaderProperty->mUVScale.v);

    const NiBtOgre::BSShaderTextureSet* tset = nullptr;
    if (bsLightingShaderProperty->mTextureSetRef != -1)
    {
        tset = bsLightingShaderProperty->getBSShaderTextureSet();
        for (std::size_t i = 0; i < tset->mTextures.size(); ++i)
        {
            if (!tset->mTextures[i].empty())
            {
                std::string texture = tset->mTextures[i];
                if (texture.substr(1, 5) == "ata\\")
                    boost::hash_combine(h, "t"+texture.substr(6));
                else
                    boost::hash_combine(h, Misc::ResourceHelpers::correctTexturePath(texture));
            }
        }
    }

    boost::hash_combine(h, bsLightingShaderProperty->mEmissiveColor.x);
    boost::hash_combine(h, bsLightingShaderProperty->mEmissiveColor.y);
    boost::hash_combine(h, bsLightingShaderProperty->mEmissiveColor.z);
    boost::hash_combine(h, bsLightingShaderProperty->mEmissiveMultiple);

    boost::hash_combine(h, bsLightingShaderProperty->mTextureClampMode);
    boost::hash_combine(h, bsLightingShaderProperty->mAlpha);
    boost::hash_combine(h, bsLightingShaderProperty->mRefractionStrength);
    boost::hash_combine(h, bsLightingShaderProperty->mGlossiness);
    boost::hash_combine(h, bsLightingShaderProperty->mSpecularColor.x);
    boost::hash_combine(h, bsLightingShaderProperty->mSpecularColor.y);
    boost::hash_combine(h, bsLightingShaderProperty->mSpecularColor.z);
    boost::hash_combine(h, bsLightingShaderProperty->mSpecularStrength);
    boost::hash_combine(h, bsLightingShaderProperty->mLightingEffect1);
    boost::hash_combine(h, bsLightingShaderProperty->mLightingEffect2);

    switch (bsLightingShaderProperty->getSkyrimShaderType())
    {
        case 1: // Environment Map
            boost::hash_combine(h, bsLightingShaderProperty->mEnvironmentMapScale);
            break;
        case 5: // Skin Tint
            boost::hash_combine(h, bsLightingShaderProperty->mSkinTintColor.x);
            boost::hash_combine(h, bsLightingShaderProperty->mSkinTintColor.y);
            boost::hash_combine(h, bsLightingShaderProperty->mSkinTintColor.z);
            break;
        case 6: // Hair Tint
            boost::hash_combine(h, bsLightingShaderProperty->mHairTintColor.x);
            boost::hash_combine(h, bsLightingShaderProperty->mHairTintColor.y);
            boost::hash_combine(h, bsLightingShaderProperty->mHairTintColor.z);
            break;
        case 7: // Parallax Occ Material
            boost::hash_combine(h, bsLightingShaderProperty->mMaxPasses);
            boost::hash_combine(h, bsLightingShaderProperty->mScale);
            break;
        case 11: // MultiLayer Parallax
            boost::hash_combine(h, bsLightingShaderProperty->mParallaxInnerLayerThickness);
            boost::hash_combine(h, bsLightingShaderProperty->mParallaxRefractionScale);
            boost::hash_combine(h, bsLightingShaderProperty->mParallaxInnerLayerTextureScale.u);
            boost::hash_combine(h, bsLightingShaderProperty->mParallaxInnerLayerTextureScale.v);
            boost::hash_combine(h, bsLightingShaderProperty->mParallaxEnvmapStrength);
            break;
        case 14: // Sparkle Snow
            boost::hash_combine(h, bsLightingShaderProperty->mSparkleParameters.x);
            boost::hash_combine(h, bsLightingShaderProperty->mSparkleParameters.y);
            boost::hash_combine(h, bsLightingShaderProperty->mSparkleParameters.z);
            boost::hash_combine(h, bsLightingShaderProperty->mSparkleParameters.w);
            break;
        case 16: // Eye Envmap
            boost::hash_combine(h, bsLightingShaderProperty->mEyeCubemapScale);
            boost::hash_combine(h, bsLightingShaderProperty->mLeftEyeReflectionCenter.x);
            boost::hash_combine(h, bsLightingShaderProperty->mLeftEyeReflectionCenter.y);
            boost::hash_combine(h, bsLightingShaderProperty->mLeftEyeReflectionCenter.z);
            boost::hash_combine(h, bsLightingShaderProperty->mRightEyeReflectionCenter.x);
            boost::hash_combine(h, bsLightingShaderProperty->mRightEyeReflectionCenter.y);
            boost::hash_combine(h, bsLightingShaderProperty->mRightEyeReflectionCenter.z);
            break;
        default:
            break;
    }

    std::map<size_t,std::string>::iterator itr = sMaterialMap.find(h);
    if (itr != sMaterialMap.end())
    {
        // a suitable material exists already - use it
        sh::MaterialInstance* instance = sh::Factory::getInstance().getMaterialInstance(itr->second);

        // no vertex normals in NiTriShapeData if flag has Model_Space_Normals
        mNeedTangents = ((bsLightingShaderProperty->mShaderFlags1 & 0x00001000) != 0) &&
            !sh::retrieveValue<sh::StringValue>(instance->getProperty("normalMap"), instance).get().empty();

        return itr->second;
    }

    // not found, create a new one
    sMaterialMap.insert(std::make_pair(h, name));

    // choose a shader
    sh::MaterialInstance *instance = nullptr;
    if ((bsLightingShaderProperty->mShaderFlags1 & 0x00001000) != 0)
    {
        instance = sh::Factory::getInstance().createMaterialInstance(name, "openmw_tes5msnobjects_base");

        instance->setProperty("has_msn", sh::makeProperty(new sh::BooleanValue(true)));
    }
    else
        instance = sh::Factory::getInstance().createMaterialInstance(name, "openmw_tes5objects_base");

    // clamp mode
    switch (bsLightingShaderProperty->mTextureClampMode)
    {
        case 0:
            instance->setProperty("clamp_mode", sh::makeProperty(new sh::StringValue("clamp clamp")));
            break;
        case 1:
            instance->setProperty("clamp_mode", sh::makeProperty(new sh::StringValue("clamp wrap")));
            break;
        case 2:
            instance->setProperty("clamp_mode", sh::makeProperty(new sh::StringValue("wrap clamp")));
            break;
        case 3:
            instance->setProperty("clamp_mode", sh::makeProperty(new sh::StringValue("wrap wrap")));
            break;
        default:
            break;
    }

    // alpha
    instance->setProperty("alpha", sh::makeProperty(new sh::FloatValue(bsLightingShaderProperty->mAlpha)));

    // uv
    //instance->setProperty("uv_offset", sh::makeProperty(new sh::Vector2(
    sh::Factory::getInstance().setSharedParameter("uv_offset", sh::makeProperty(new sh::Vector2(
                bsLightingShaderProperty->mUVOffset.u,
                bsLightingShaderProperty->mUVOffset.v)));

    //instance->setProperty("uv_scale", sh::makeProperty(new sh::Vector2(
    sh::Factory::getInstance().setSharedParameter("uv_scale", sh::makeProperty(new sh::Vector2(
                bsLightingShaderProperty->mUVScale.u,
                bsLightingShaderProperty->mUVScale.v)));

    // flags
    if ((bsLightingShaderProperty->mShaderFlags1 & 0x00400000) != 0)
    {
        instance->setProperty("has_own_emit", sh::makeProperty(new sh::BooleanValue(true)));
    }

    // already set above
    //if ((bsLightingShaderProperty->mShaderFlags1 & 0x00001000) != 0)
        //instance->setProperty("has_msn", sh::makeProperty(new sh::BooleanValue(true)));

    if ((bsLightingShaderProperty->mShaderFlags2 & 0x02000000) != 0)
        instance->setProperty("has_soft_light", sh::makeProperty(new sh::BooleanValue(true)));

    if ((bsLightingShaderProperty->mShaderFlags2 & 0x04000000) != 0)
        instance->setProperty("has_rim_light", sh::makeProperty(new sh::BooleanValue(true)));

    if ((bsLightingShaderProperty->mShaderFlags2 & 0x08000000) != 0)
        instance->setProperty("has_back_light", sh::makeProperty(new sh::BooleanValue(true)));

    if (bsLightingShaderProperty->getSkyrimShaderType() == 4)  // face tint
        instance->setProperty("has_detail_mask", sh::makeProperty(new sh::BooleanValue(true)));

    if (bsLightingShaderProperty->getSkyrimShaderType() == 5) // skin tint
    {
        instance->setProperty("has_tint_color", sh::makeProperty(new sh::BooleanValue(true)));

        //instance->setProperty("tintColor", sh::makeProperty(new sh::Vector3(
        sh::Factory::getInstance().setSharedParameter("tintColor", sh::makeProperty(new sh::Vector3(
                    bsLightingShaderProperty->mSkinTintColor.x,
                    bsLightingShaderProperty->mSkinTintColor.y,
                    bsLightingShaderProperty->mSkinTintColor.z)));
    }
    else if (bsLightingShaderProperty->getSkyrimShaderType() == 6)    // hair tint
    {
        instance->setProperty("has_tint_color", sh::makeProperty(new sh::BooleanValue(true)));

        instance->setProperty("tintColor", sh::makeProperty(new sh::Vector3(
                    bsLightingShaderProperty->mHairTintColor.x,
                    bsLightingShaderProperty->mHairTintColor.y,
                    bsLightingShaderProperty->mHairTintColor.z)));
    }

    // specular
    if ((bsLightingShaderProperty->mShaderFlags1 & 0x00000001) != 0)
    {
        instance->setProperty("has_specular", sh::makeProperty(new sh::BooleanValue(true)));

        //instance->setProperty("specularColor", sh::makeProperty(new sh::Vector3(
        sh::Factory::getInstance().setSharedParameter("specularColor", sh::makeProperty(new sh::Vector3(
                    bsLightingShaderProperty->mSpecularColor.x,
                    bsLightingShaderProperty->mSpecularColor.y,
                    bsLightingShaderProperty->mSpecularColor.z)));

        //instance->setProperty("specular_strength", sh::makeProperty(new sh::FloatValue(
        sh::Factory::getInstance().setSharedParameter("specular_strength", sh::makeProperty(new sh::FloatValue(
                    bsLightingShaderProperty->mSpecularStrength)));

        //instance->setProperty("specular_glossiness", sh::makeProperty(new sh::FloatValue(
        sh::Factory::getInstance().setSharedParameter("specular_glossiness", sh::makeProperty(new sh::FloatValue(
                    bsLightingShaderProperty->mGlossiness)));
    }
    //else
    //{
    //    // FIXME: is setting false necessary?
    //    //instance->setProperty("has_specular", sh::makeProperty(new sh::BooleanValue(false)));

    //    instance->setProperty("specularColor", sh::makeProperty(new sh::Vector3(0.f, 0.f, 0.f)));
    //    instance->setProperty("specularStrength", sh::makeProperty(new sh::FloatValue(0.f)));
    //    instance->setProperty("specularGlossiness", sh::makeProperty(new sh::FloatValue(0.f)));
    //}

    // emissive
    sh::Factory::getInstance().setSharedParameter("emissiveColor", sh::makeProperty(new sh::Vector3(
                bsLightingShaderProperty->mEmissiveColor.x,
                bsLightingShaderProperty->mEmissiveColor.y,
                bsLightingShaderProperty->mEmissiveColor.z)));

    //instance->setProperty("emissive_mult", sh::makeProperty(new sh::FloatValue(
    sh::Factory::getInstance().setSharedParameter("emissive_mult", sh::makeProperty(new sh::FloatValue(
                bsLightingShaderProperty->mEmissiveMultiple)));

    sh::Factory::getInstance().setSharedParameter("lighting_effect1", sh::makeProperty(new sh::FloatValue(
                bsLightingShaderProperty->mLightingEffect1)));

    sh::Factory::getInstance().setSharedParameter("lighting_effect2", sh::makeProperty(new sh::FloatValue(
                bsLightingShaderProperty->mLightingEffect2)));

    if (tset)
    {
        // base/diffuse map
        if (tset->mTextures[0] != "")
        {
            instance->setProperty("diffuseMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[0])));
            instance->setProperty("use_diffuse_map", sh::makeProperty(new sh::BooleanValue(true)));
        }

        // normal map
        if (tset->mTextures[1] != "")
        {
            instance->setProperty("normalMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[1])));

            // FIXME: this property is for terrain
            //instance->setProperty("use_normal_map", sh::makeProperty(new sh::BooleanValue(true)));
        }

        // glow/specular map
        // FIXME: also light mask?
        if (tset->mTextures[2] != "")
        {
            // FIXME: glow is 5? (same as env?)
            if ((bsLightingShaderProperty->mShaderFlags2 & 0x00000040) != 0)
                instance->setProperty("glowMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[2])));
            else
                instance->setProperty("lightMask", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[2])));
        }

        // height map (default shader) / detail map (msn)
        if (tset->mTextures[3] != "")
        {
            if (bsLightingShaderProperty->getSkyrimShaderType() == 3 && // parallax/height
               (bsLightingShaderProperty->mShaderFlags1 & 0x00000800) != 0)
            {
                instance->setProperty("has_height_map", sh::makeProperty(new sh::BooleanValue(true)));

                instance->setProperty("heightMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[3])));
            }
            else
            {
                //instance->setProperty("has_detail_map", sh::makeProperty(new sh::BooleanValue(true)));

                instance->setProperty("detailMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[3])));
            }
        }
        //else
        // FIXME: use default?

        // environment/cube map; default shader only
        if (tset->mTextures[4] != "")
        {
            instance->setProperty("environmentMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[4])));
        }

        // FIXME: just a guess
        if (tset->mTextures[5] != "")
        {
            instance->setProperty("environmentMask", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[5])));
        }

        // FIXME: just a guess
        if (tset->mTextures[6] != "")
        {
            // default is grey?
            std::string texture = tset->mTextures[6];
            if (texture.substr(1, 4) == "ata\\")
                instance->setProperty("tintMap", sh::makeProperty("t"+texture.substr(6)));
            else
                instance->setProperty("tintMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[6])));
        }

        // backlight map? also specular?
        if (tset->mTextures[7] != "")
        {
            if ((bsLightingShaderProperty->mShaderFlags2 & 0x08000000) != 0)
            {
                instance->setProperty("backlightMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[7])));
            }
            else if ((bsLightingShaderProperty->mShaderFlags1 & 0x00000001) != 0)
                instance->setProperty("specularMap", sh::makeProperty(
                                           Misc::ResourceHelpers::correctTexturePath(tset->mTextures[7])));
        }
    }

    if (vertexColor)
        instance->setProperty("has_vertex_color", sh::makeProperty(new sh::BooleanValue(true)));
#if 0
    // Add transparency if NiAlphaProperty was present
    if((alphaFlags&1))
    {
        std::string blend_mode;
        blend_mode += getBlendFactor((alphaFlags>>1)&0xf);
        blend_mode += " ";
        blend_mode += getBlendFactor((alphaFlags>>5)&0xf);
        instance->setProperty("scene_blend", sh::makeProperty(new sh::StringValue(blend_mode)));
    }

    if((alphaFlags>>9)&1)
    {
        std::string reject;
        reject += getTestMode((alphaFlags>>10)&0x7);
        reject += " ";
        reject += std::to_string(alphaTest);
        instance->setProperty("alpha_rejection", sh::makeProperty(new sh::StringValue(reject)));
    }
    else
        instance->getMaterial()->setShadowCasterMaterial("openmw_shadowcaster_noalpha");

    // Ogre usually only sorts if depth write is disabled, so we want "force" instead of "on"
    instance->setProperty("transparent_sorting", sh::makeProperty(new sh::StringValue(
        ((alphaFlags&1) && !((alphaFlags>>13)&1)) ? "force" : "off")));

    instance->setProperty("depth_check", sh::makeProperty(new sh::StringValue((depthFlags&1) ? "on" : "off")));
    instance->setProperty("depth_write", sh::makeProperty(new sh::StringValue(((depthFlags>>1)&1) ? "on" : "off")));
    // depth_func???

    if(vertMode == 0 || !vertexColor)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::Vector4(ambient.x, ambient.y, ambient.z, 1)));
        instance->setProperty("diffuse", sh::makeProperty(new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));
        instance->setProperty("emissive", sh::makeProperty(new sh::Vector4(emissive.x, emissive.y, emissive.z, 1)));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("0")));
    }
    else if(vertMode == 1)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::Vector4(ambient.x, ambient.y, ambient.z, 1)));
        instance->setProperty("diffuse", sh::makeProperty(new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));
        instance->setProperty("emissive", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("1")));
    }
    else if(vertMode == 2)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("diffuse", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("emissive", sh::makeProperty(new sh::Vector4(emissive.x, emissive.y, emissive.z, 1)));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("2")));
    }
    else
        std::cerr << "Unhandled vertex mode: " << vertMode << std::endl;

    if(specFlags)
    {
        instance->setProperty("specular",
                sh::makeProperty(new sh::Vector4(specular.x, specular.y, specular.z, glossiness)));
    }

    if(wireFlags)
    {
        instance->setProperty("polygon_mode", sh::makeProperty(new sh::StringValue("wireframe")));
    }
#endif

    // Don't use texName, as it may be overridden
    mNeedTangents = false;//!sh::retrieveValue<sh::StringValue>(instance->getProperty("normalMap"), instance).get().empty()
        //&& (texName[NiTexturingProperty::Texture_BumpMap].find("_msn.") != std::string::npos); // FIXME: quick test

    return name;
}
}
