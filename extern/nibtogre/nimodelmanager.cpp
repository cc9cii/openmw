/*
  Copyright (C) 2019, 2020 cc9cii

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

  Below code is based on Ogre::SkeletonManager and Ogre::MeshManager.

*/
#include "nimodelmanager.hpp"

#include <vector>
#include <cassert>
#include <algorithm> // std::min
#include <iostream> // FIXME: for testing only

#include <boost/algorithm/string.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreTextureManager.h>
#include <OgreTexture.h>
#include <OgreVector3.h>
#include <OgreSkeleton.h>
#include <OgrePixelFormat.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCommon.h> // Ogre::Box

#include <extern/esm4/npc_.hpp>
#include <extern/esm4/race.hpp>
#include <extern/esm4/formid.hpp>
#include <extern/fglib/fgsam.hpp>
#include <extern/fglib/fgfile.hpp>
#include <extern/fglib/fgegt.hpp>

#include "nimodel.hpp"

template<> NiBtOgre::NiModelManager* Ogre::Singleton<NiBtOgre::NiModelManager>::msSingleton = 0;

namespace NiBtOgre
{
    NiModelManager* NiModelManager::getSingletonPtr(void)
    {
        return msSingleton;
    }

    NiModelManager& NiModelManager::getSingleton(void)
    {
        assert( msSingleton ); return ( *msSingleton );
    }

    NiModelManager::NiModelManager()
    {
        // FIXME: how to choose an appropriate value?
        // OgreMain/src/OgreGpuProgramManager.cpp           mLoadOrder =  50.0f;
        // OgreMain/src/OgreHighLevelGpuProgramManager.cpp  mLoadOrder =  50.0f;
        // OgreMain/src/Deprecated/OgreTextureManager.cpp   mLoadOrder =  75.0f;
        // OgreMain/src/OgreMaterialManager.cpp             mLoadOrder = 100.0f;
        // Components/Overlay/src/OgreFontManager.cpp       mLoadOrder = 200.0f;
        // OgreMain/src/OgreMeshManager2.cpp                mLoadOrder = 300.0f;
        // OgreMain/src/OgreOldSkeletonManager.cpp          mLoadOrder = 300.0f;
        // OgreMain/src/OgreMeshManager.cpp                 mLoadOrder = 350.0f;
        mLoadOrder = 400.0f;
        mResourceType = "NiModel";

        Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }

    NiModelManager::~NiModelManager()
    {
        Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }

    NiModelPtr NiModelManager::getByName(const Ogre::String& name, const Ogre::String& group)
    {
        return Ogre::static_pointer_cast<NiModel>(getResourceByName(name, group));
    }

    NiModelPtr NiModelManager::create(const Ogre::String& name, const Ogre::String& group,
                                       bool isManual, Ogre::ManualResourceLoader* loader,
                                       const Ogre::NameValuePairList* createParams)
    {
        return Ogre::static_pointer_cast<NiModel>(createResource(name, group, isManual, loader, createParams));
    }

    NiModelPtr NiModelManager::createManual(const Ogre::String& name, const Ogre::String& group,
                                            const Ogre::String& nif, Ogre::ManualResourceLoader* loader)
    {
        // Don't try to get existing, create should fail if already exists
        if( this->getResourceByName( name, group ) )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_DUPLICATE_ITEM,
                         "NiModel with name '" + name + "' already exists.",
                         "NiModelManager::createManual" );
        }

        Ogre::NameValuePairList params;
        params.insert( std::make_pair(Ogre::String("nif"), nif) );

        return create(name, group, true/*isManual*/, loader, &params);
    }

    Ogre::Resource* NiModelManager::createImpl(const Ogre::String& name,
            Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual,
            Ogre::ManualResourceLoader* loader, const Ogre::NameValuePairList* createParams)
    {
        if (createParams)
        {
            Ogre::String nif("nif");

            std::map<Ogre::String, Ogre::String>::const_iterator it = createParams->find(nif);
            if (it != createParams->end())
            {
                return OGRE_NEW NiModel(this, name, handle, group, isManual, loader, it->second);
            }
        }

        OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                     "Cannot find build parameters for " + name,
                     "NiModelManager::createImpl");
    }

    NiModelPtr NiModelManager::createManualModel(const Ogre::String& nif, const Ogre::String& group,
            const Ogre::String& raceName, const Ogre::String& texture)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(raceName+nif, group, nif, this); // raceName has "$" postpended

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Skinned;
        bInfo.baseNif = nif;
        bInfo.baseTexture = texture;
        bInfo.raceName = raceName;
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createSkinnedModel(const Ogre::String& nif, const Ogre::String& group,
            NiModel *skeleton, const Ogre::String& raceName, const Ogre::String& texture)
    {
        std::string skelName = boost::to_lower_copy(skeleton->getName());
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(raceName+skelName+"_"+nif, group, nif, this); // raceName has "$"

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Skinned;
        bInfo.baseNif = nif;
        bInfo.skel = skeleton; // TODO: throw exeption if no skeleton?
        bInfo.raceName = raceName;
        bInfo.baseTexture = texture;
        bInfo.skelNif = skeleton->getName();
        bInfo.skelGroup = skeleton->getOgreGroup();
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createSkeletonModel(const Ogre::String& nif, const Ogre::String& group)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Skeleton;
        bInfo.baseNif = nif;
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createMorphedModel(const Ogre::String& nif, const Ogre::String& group,
            const ESM4::Npc *npc, const ESM4::Race *race, NiModel *skeleton, const Ogre::String& texture,
            ModelBodyPart bodyPart)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(npc->mEditorId + "_" + nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Morphed;
        bInfo.npc = npc;
        bInfo.race = race;
        bInfo.baseNif = nif;
        bInfo.baseTexture = texture;
        bInfo.bodyPart = bodyPart;
        bInfo.skel = skeleton;
        if (skeleton)
        {
            bInfo.skelNif = skeleton->getName();
            bInfo.skelGroup = skeleton->getOgreGroup();
        }
        else
        {
            bInfo.skelNif.clear();
            bInfo.skelGroup.clear();
        }
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createMorphedHair(const Ogre::String& nif, const Ogre::String& group,
            const ESM4::Npc *npc, const ESM4::Race *race, NiModel *skeleton, const Ogre::String& texture,
            ModelBodyPart bodyPart, bool hat)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(npc->mEditorId + (hat ? "_Hat_" : "_NoHat_") + nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_FO_Hair;
        bInfo.npc = npc;
        bInfo.race = race;
        bInfo.baseNif = nif;
        bInfo.baseTexture = texture;
        bInfo.bodyPart = bodyPart;
        bInfo.skel = skeleton;
        bInfo.hat = hat;
        if (skeleton)
        {
            bInfo.skelNif = skeleton->getName();
            bInfo.skelGroup = skeleton->getOgreGroup();
        }
        else
        {
            bInfo.skelNif.clear();
            bInfo.skelGroup.clear();
        }
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createAnimModel(const Ogre::String& nif, const Ogre::String& group,
            NiModel *skeleton)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Anim;
        bInfo.baseNif = nif;
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::getOrLoadByName(const Ogre::String& name, const Ogre::String& group)
    {
        Ogre::ResourcePtr res = getResourceByName(name, group);

        // load() calls ResourceManager::createOrRetrieve() then Resource::load()
        // which results in Resource::loadImpl() being called
        if (!res)
        {
            Ogre::NameValuePairList params;
            params.insert( std::make_pair(Ogre::String("nif"), name) );

            res = load(name, group, false/*isManual*/, nullptr/*ManualResourceLoader**/, &params);
        }

        return Ogre::static_pointer_cast<NiModel>(res);
    }

    void NiModelManager::loadResource(Ogre::Resource* res)
    {
        NiModel* model = static_cast<NiModel*>(res);
        model->createNiObjects(); // read the NIF file and create the objects

        // Find build parameters
        std::map<Ogre::Resource*, ModelBuildInfo>::iterator it = mModelBuildInfoMap.find(res);
        if (it == mModelBuildInfoMap.end())
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find build parameters for " + res->getName(),
                         "NiModelManager::loadResource");
        }

        ModelBuildInfo& bInfo = it->second;
        switch(bInfo.type)
        {
            case MBT_Object:
                loadManualModel(model, bInfo);
                break;
            case MBT_Skinned:
                loadManualSkinnedModel(model, bInfo);
                break;
            case MBT_Skeleton:
                loadManualSkeletonModel(model, bInfo);
                break;
            case MBT_Morphed:
                loadManualMorphedModel(model, bInfo);
                break;
            case MBT_FO_Hair:
                loadManualHairModel(model, bInfo);
                break;
            case MBT_Anim:
                loadManualAnimModel(model, bInfo);
                break;
            default:
                OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                             "Unknown build parameters for " + res->getName(),
                             "NiModelManager::loadResource");
        }
    }

    void NiModelManager::loadManualModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        if (!bInfo.baseTexture.empty())
            pModel->setSkinTexture(bInfo.baseTexture);
        pModel->createMesh(false/*isMorphed*/);
        pModel->buildModel();
    }

    void NiModelManager::loadManualSkinnedModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        Ogre::SkeletonPtr skel;

        try
        {
            skel = bInfo.skel->getSkeleton();
        }
        catch (...)
        {
            NiModelPtr skelModel = getByName(bInfo.skelNif, bInfo.skelGroup);
            skel = skelModel->getSkeleton();
        }

        if (!bInfo.baseTexture.empty())
            pModel->setSkinTexture(bInfo.baseTexture);
        pModel->createMesh(false, skel);
        pModel->buildSkinnedModel(skel);
    }

    void NiModelManager::loadManualSkeletonModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        pModel->findBoneNodes(true/*buildObjectPalette*/);
        pModel->buildSkeleton(true/*load*/);
        pModel->createMesh(false/*isMorphed*/);
        if (!pModel->buildData().mMeshBuildList.empty())
        {
            pModel->buildModel(); // this skeleton may have controllers
        }
    }

    void NiModelManager::loadManualMorphedModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        bool isTES4 = bInfo.npc->mIsTES4;
        bool isFemale = (bInfo.npc->mBaseConfig.tes4.flags & 0x1) != 0; // FIXME: depends on the game

        const std::vector<float>& sRaceCoeff
            = ((isFemale && !isTES4) ? bInfo.race->mSymShapeModeCoeffFemale : bInfo.race->mSymShapeModeCoefficients);
        const std::vector<float>& aRaceCoeff
            = ((isFemale && !isTES4) ? bInfo.race->mAsymShapeModeCoeffFemale : bInfo.race->mAsymShapeModeCoefficients);
        const std::vector<float>& sRaceTCoeff
            = ((isFemale && !isTES4) ? bInfo.race->mSymTextureModeCoeffFemale : bInfo.race->mSymTextureModeCoefficients);
        const std::vector<float>& sCoeff = bInfo.npc->mSymShapeModeCoefficients;
        const std::vector<float>& aCoeff = bInfo.npc->mAsymShapeModeCoefficients;
        const std::vector<float>& sTCoeff = bInfo.npc->mSymTextureModeCoefficients;

        FgLib::FgSam sam;
        std::string normalTexture;
        std::string skinTexture = bInfo.baseTexture;

        // if EGM and TRI both found, build morphed vertices
        // NOTE: some helmets do not have an associated EGM, e.g. "Armor\Daedric\M\Helmet.NIF"
        // WARN: fgMorphVertices() throws if more than one NiTriBasedGeom in the model
        if (sam.buildMorphedVertices(pModel->fgMorphVertices(), pModel->fgVertices(),
                                     bInfo.baseNif, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff))
        {
            pModel->useFgMorphVertices();

            // special material for headhuman.dds // FIXME: TES4 only
            // FIXME: case sensitive!
            if (bInfo.bodyPart == BP_Head && bInfo.baseNif.find("HeadHuman.nif") != std::string::npos)
            {
                bool isFemale = (bInfo.npc->mBaseConfig.tes4.flags & 0x1) != 0; // FIXME: depends on the game

                std::string ageTextureFile
                    = sam.getHeadHumanDetailTexture(bInfo.baseNif, sam.getAge(sRaceCoeff, sCoeff), isFemale);
#if 0
                if (!ageTextureFile.empty())
                    std::cout << bInfo.npc->mEditorId << "has age detail texture " << ageTextureFile << std::endl;
#endif
                // find the corresponding normal texture
                std::size_t pos = ageTextureFile.find_last_of(".");
                if (pos == std::string::npos)
                    return; // FIXME: should throw

                /*std::string*/ normalTexture = ageTextureFile.substr(0, pos) + "_n.dds";

                // FIXME: do stuff here including morphed textue

                // FIXME: need to pass to shader FaceGen maps textures\faces\oblivion.esm\<formid>_[01].dds

                std::string faceDetailFile
                    = sam.getTES4NpcDetailTexture_0(ESM4::formIdToString(bInfo.npc->mFormId));
//#if 0
                if (faceDetailFile.empty())
                    std::cout << bInfo.npc->mEditorId << " does not have a facegen npc detail texture" << std::endl;
//#endif



#if 0





                FgLib::FgFile<FgLib::FgEgt> egtFile;
                const FgLib::FgEgt *egt = egtFile.getOrLoadByMeshName(bInfo.baseNif);

                if (egt == nullptr)
                    return; // FIXME: throw?

                // try to regtrieve previously created morph texture
                Ogre::TexturePtr morphTexture = Ogre::TextureManager::getSingleton().getByName(
                       bInfo.npc->mEditorId+"_"+bInfo.baseTexture,
                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (!morphTexture)
                {
                    // create a blank one
                    morphTexture = Ogre::TextureManager::getSingleton().createManual(
                        bInfo.npc->mEditorId+"_"+bInfo.baseTexture, // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,  // type
                        egt->numRows(), egt->numColumns(), // width & height
                        0,                  // number of mipmaps; FIXME: should be 2? or 1?
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                            // textures updated very often (e.g. each frame)
                }

                // we need the base texture
                Ogre::ResourcePtr baseTexture = Ogre::TextureManager::getSingleton().createOrRetrieve(
                       bInfo.baseTexture, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
                if (!baseTexture)
                    return; // FIXME: throw?

                // we also need the age detail modulation texture
                Ogre::TexturePtr ageTexture = Ogre::TextureManager::getSingleton().getByName(
                       bInfo.npc->mEditorId+"_"+ageTextureFile,
                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (!ageTexture)
                {
                    ageTexture = Ogre::TextureManager::getSingleton().createManual(
                        bInfo.npc->mEditorId+"_"+ageTextureFile,
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,
                        egt->numRows(), egt->numColumns(),
                        0,
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);
                }

                // we need the age texture src
                Ogre::ResourcePtr ageTextureSrc = Ogre::TextureManager::getSingleton().createOrRetrieve(
                       ageTextureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
                if (!ageTextureSrc)
                    return; // FIXME: throw?

                // dest: usually 256x256 (egt.numRows()*egt.numColumns())
                Ogre::HardwarePixelBufferSharedPtr pixelBuffer = morphTexture->getBuffer();
                pixelBuffer->unlock(); // prepare for blit()
                // src: can be 128x128
                Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc
                    = Ogre::static_pointer_cast<Ogre::Texture>(baseTexture)->getBuffer();
                pixelBufferSrc->unlock(); // prepare for blit()
                // if source and destination dimensions don't match, scaling is done
                pixelBuffer->blit(pixelBufferSrc);

                // age dest:
                Ogre::HardwarePixelBufferSharedPtr pixelBufferAge = ageTexture->getBuffer();
                pixelBufferAge->unlock(); // prepare for blit()
                // age src:
                Ogre::HardwarePixelBufferSharedPtr pixelBufferAgeSrc
                    = Ogre::static_pointer_cast<Ogre::Texture>(ageTextureSrc)->getBuffer();
                //if (!pixelBufferAgeSrc)
                    //std::cout << "detail texture null" << std::endl;
                pixelBufferAgeSrc->unlock(); // prepare for blit()
                // if source and destination dimensions don't match, scaling is done
                pixelBufferAge->blit(pixelBufferAgeSrc); // FIXME: can't we just use the src?

                // Lock the pixel buffer and get a pixel box
                //pixelBufferSrc->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                //const Ogre::PixelBox& pixelBoxSrc = pixelBufferSrc->getCurrentLock();

                pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
                uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

                pixelBufferAge->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                const Ogre::PixelBox& pixelBoxAge = pixelBufferAge->getCurrentLock();
                uint8_t *pAge = static_cast<uint8_t*>(pixelBoxAge.data);
                if (!pAge)
                    std::cout << "null age detail" << std::endl;






                // FIXME: this one should be passed to a shader, along with the "_1" variant
                Ogre::TexturePtr faceDetailTexture = Ogre::TextureManager::getSingleton().getByName(
                        faceDetailFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (faceDetailTexture.isNull())
                {
                    faceDetailTexture = Ogre::TextureManager::getSingleton().create(
                        faceDetailFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    faceDetailTexture->load();
                }

                //pixelBufferDetail->lock(Ogre::HardwareBuffer::HBL_NORMAL);






                Ogre::Vector3 sym;
                const std::vector<Ogre::Vector3>& symTextureModes = egt->symTextureModes();

                // update the pixels with SCM and detail texture
                // NOTE: mSymTextureModes is assumed to have the image pixels in row-major order
                for (size_t i = 0; i < egt->numRows()*egt->numColumns(); ++i) // height*width, should be 256*256
                {
                    // FIXME: for some reason adding the race coefficients makes it look worse
                    //        even though it is clear that for shapes they are needed
                    // sum all the symmetric texture modes for a given pixel i
                    sym = Ogre::Vector3::ZERO;
                    // CheydinhalGuardCityPostNight03 does not have any symmetric texture coeff
                    for (size_t j = 0; j < 50/*mNumSymTextureModes*/; ++j)
                        sym += (sRaceTCoeff[j] + (sTCoeff.empty() ? 0.f : sTCoeff[j])) * symTextureModes[50*i + j];

                    // Detail texture is applied after reconstruction of the colour map from the SCM.
                    // Using an average of the 3 colors makes the resulting texture less blotchy. Also see:
                    // "Each such factor is coded as a single unsigned byte in the range [0,255]..."
                    int t = *(pAge+0) + *(pAge+1) + *(pAge+2);
                    float ft = t/192.f; // 64 * 3 = 192

                    int r = *(pAge+0);
                    float fr = r/64.f;
                    fr = ft; // use average
                    r = std::min(int((*(pDest+0)+sym.x) * fr), 255);
                    //r = std::min(int(*(pDest+0)*fr + sym.x), 255); // experiment

                    int g = *(pAge+1);
                    float fg = g/64.f;
                    fg = ft; // use average
                    g = std::min(int((*(pDest+1)+sym.y) * fg), 255);
                    //g = std::min(int(*(pDest+1)*fg + sym.y), 255); // experiment

                    int b = *(pAge+2);
                    float fb = b/64.f;
                    fb = ft; // use average
                    b = std::min(int((*(pDest+2)+sym.z) * fb), 255);
                    //b = std::min(int(*(pDest+2)*fb + sym.z), 255); // experiment

                    *(pDest+0) = r;
                    *(pDest+1) = g;
                    *(pDest+2) = b;
                    pDest += 4;
                    pAge += 4;
                }

                // Unlock the pixel buffers
                //pixelBufferSrc->unlock();
                pixelBuffer->unlock();
                pixelBufferAge->unlock();

                // FIXME: unfortunately we aren't able to use non-filename for texture
                //        (maybe modify Shiny to allow it?)
                //skinTexture = morphTexture->getName();
                //std::cout << bInfo.npc->mEditorId << ", " << morphTexture->getName() <<
                    //", " << bInfo.baseTexture << std::endl;
#endif
                // hack to add age based normal texture, diffuse will be replaced later
                if (bInfo.race->mEditorId != "Dremora")
                    skinTexture = ageTextureFile;
            }
            else if (bInfo.bodyPart == BP_Head) // non-human heads
            {
            }
            else if (bInfo.bodyPart == BP_Hair) // special material for hair
            {
                size_t pos = bInfo.baseTexture.find_last_of(".");
                if (pos == std::string::npos)
                    return; // FIXME: should throw

                std::string texture = bInfo.baseTexture.substr(0, pos);
                /*std::string*/ normalTexture = texture + "_n.dds";
                std::string heightMap = texture + "_hh.dds";  // NOTE: apparently only the R channel needed?
                std::string layerMap = texture + "_hl.dds";

                // FIXME: how to extract the anisoMap from normal texture's alpha channel?

                // FIXME: do other stuff here (but no texture morphing since there aren't any EGT for hair)

            }
        }

        // Attempt texture morphing regardless of vertex morphing status
        if (bInfo.bodyPart == BP_EarMale || bInfo.bodyPart == BP_EarFemale)
        {
            FgLib::FgFile<FgLib::FgEgt> egtFile;
            const FgLib::FgEgt *egt = egtFile.getOrLoadByMeshName(bInfo.baseNif);

            if (egt == nullptr)
                return; // FIXME: throw?

            // try to regtrieve previously created morph texture
            Ogre::TexturePtr morphTexture = Ogre::TextureManager::getSingleton().getByName(
                   bInfo.npc->mEditorId+"_"+bInfo.baseTexture,
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (!morphTexture)
            {
                // create a blank one
                morphTexture = Ogre::TextureManager::getSingleton().createManual(
                    bInfo.npc->mEditorId+"_"+bInfo.baseTexture, // name
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::TEX_TYPE_2D,  // type
                    egt->numRows(), egt->numColumns(), // width & height
                    0,                  // number of mipmaps; FIXME: should be 2? or 1?
                    Ogre::PF_BYTE_RGBA,
                    Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                        // textures updated very often (e.g. each frame)
            }

            // we need the base texture
            Ogre::ResourcePtr baseTexture = Ogre::TextureManager::getSingleton().createOrRetrieve(
                   bInfo.baseTexture, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
            if (!baseTexture)
                return; // FIXME: throw?

            // dest: usually 256x256 (egt.numRows()*egt.numColumns())
            Ogre::HardwarePixelBufferSharedPtr pixelBuffer = morphTexture->getBuffer();
            pixelBuffer->unlock(); // prepare for blit()
            // src: can be 128x128
            Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc
                = Ogre::static_pointer_cast<Ogre::Texture>(baseTexture)->getBuffer();
            pixelBufferSrc->unlock(); // prepare for blit()
            // if source and destination dimensions don't match, scaling is done
            pixelBuffer->blit(pixelBufferSrc);

            pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
            const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
            uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

            Ogre::Vector3 sym;
            const std::vector<Ogre::Vector3>& symTextureModes = egt->symTextureModes();

            // update the pixels with SCM and detail texture
            // NOTE: mSymTextureModes is assumed to have the image pixels in row-major order
            for (size_t i = 0; i < egt->numRows()*egt->numColumns(); ++i) // height*width, should be 256*256
            {
                // FIXME: for some reason adding the race coefficients makes it look worse
                //        even though it is clear that for shapes they are needed
                // sum all the symmetric texture modes for a given pixel i
                sym = Ogre::Vector3::ZERO;
                for (size_t j = 0; j < 50/*mNumSymTextureModes*/; ++j)
                    sym += (sRaceTCoeff[j] + sTCoeff[j]) * symTextureModes[50*i + j];

                *(pDest+0) = std::min(int(*(pDest+0)+sym.x), 255);
                *(pDest+1) = std::min(int(*(pDest+1)+sym.y), 255);
                *(pDest+2) = std::min(int(*(pDest+2)+sym.z), 255);
                pDest += 4;
            }

            // Unlock the pixel buffer
            pixelBuffer->unlock();
        }
        else if (bInfo.bodyPart == BP_UpperBody || bInfo.bodyPart == BP_LowerBody) // humans only
        {
        }
        else
        {
            // no special materials for eyes, mouth, teeth and tongue, just the base texture

        }


#if 0
        // build the morphed texture and use it as the base texture
        Ogre::TexturePtr fgTexture
            = Ogre::TextureManager::getSingleton().getByName(bInfo.baseTexture, pModel->getGroup());
        if (!fgTexture)
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Base texture not found for " + pModel->getName(),
                         "NiModelManager::loadManualMorphedModel");
        }

        sam.getMorphedTexture(fgTexture,
                bInfo.baseNif, bInfo.npc->mEditorId, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);
#endif
        // build the material

        // build the mesh ??? maybe not since circular?  just create?

        // build the rest of the model / misc odds and ends

        //if (pModel->targetBone() != "") // this should also work
        if (!pModel->buildData().isSkinnedModel()) // does it have an NiSkinInstance?
        {
            if (!bInfo.baseTexture.empty())
                pModel->setSkinTexture(skinTexture);
            pModel->createMesh(true/*isMorphed*/);
            pModel->buildModel();
        }
        else // skinned, need skeleton
        {
            Ogre::SkeletonPtr skel;

            try
            {
                skel = bInfo.skel->getSkeleton();
            }
            catch (...)
            {
                NiModelPtr skelModel = getByName(bInfo.skelNif, bInfo.skelGroup);
                skel = skelModel->getSkeleton();
            }

            // FIXME: temp for testing only, we should create a material instead
            if (!bInfo.baseTexture.empty())
                pModel->setSkinTexture(skinTexture);

            pModel->createMesh(true/*isMorphed*/, skel);
            pModel->buildSkinnedModel(skel);
        }
    }

    void NiModelManager::loadManualHairModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        bool isFemale = (bInfo.npc->mBaseConfig.fo3.flags & 0x1) != 0; // FIXME: depends on the game

        const std::vector<float>& sRaceCoeff
            = (isFemale ? bInfo.race->mSymShapeModeCoeffFemale : bInfo.race->mSymShapeModeCoefficients);
        const std::vector<float>& aRaceCoeff
            = (isFemale ? bInfo.race->mAsymShapeModeCoeffFemale : bInfo.race->mAsymShapeModeCoefficients);
        //const std::vector<float>& sRaceTCoeff // unused
            //= (isFemale ? bInfo.race->mSymTextureModeCoeffFemale : bInfo.race->mSymTextureModeCoefficients);
        const std::vector<float>& sCoeff = bInfo.npc->mSymShapeModeCoefficients;
        const std::vector<float>& aCoeff = bInfo.npc->mAsymShapeModeCoefficients;
        //const std::vector<float>& sTCoeff = bInfo.npc->mSymTextureModeCoefficients; // unused

        FgLib::FgSam sam;
        std::string normalTexture;
        std::string skinTexture = bInfo.baseTexture;

        // if EGM and TRI both found, build morphed vertices
        // WARN: fgMorphVertices() throws if more than one NiTriBasedGeom in the model
        // FIXME: FO3 mModelName = "Raider1GunAAFTEMPLATE_meshes\\Characters\\Hair\\HairRaiderFCat.NIF"
        //        has 2, "NoHat" and "Hat" which means we have to check the currently equipped inventory
        //        and let the model know which one to choose
        if (sam.buildMorphedVertices(pModel->fgMorphVerticesFO3(bInfo.hat), pModel->fgVerticesFO3(bInfo.hat),
                                     bInfo.baseNif, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff, bInfo.hat))
        {
            pModel->useFgMorphVertices();

            if (bInfo.bodyPart == BP_Hair) // special material for hair
            {
                size_t pos = bInfo.baseTexture.find_last_of(".");
                if (pos == std::string::npos)
                    return; // FIXME: should throw

                std::string texture = bInfo.baseTexture.substr(0, pos);
                /*std::string*/ normalTexture = texture + "_n.dds";
                std::string heightMap = texture + "_hh.dds";  // NOTE: apparently only the R channel needed?
                std::string layerMap = texture + "_hl.dds";

                // FIXME: how to extract the anisoMap from normal texture's alpha channel?

                // FIXME: do other stuff here (but no texture morphing since there aren't any EGT for hair)

            }
        }

        if (!pModel->buildData().isSkinnedModel()) // does it have an NiSkinInstance?
        {
            if (!bInfo.baseTexture.empty())
                pModel->setSkinTexture(skinTexture);
            pModel->createMesh(true/*isMorphed*/);
            pModel->buildModel();
        }
        else // skinned, need skeleton
            std::cout << bInfo.npc->mEditorId << " has skinned hair?" << std::endl;
    }

    void NiModelManager::loadManualAnimModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
#if 0
        pModel->buildAnimation(
                Ogre::Entity *skelBase,
                NiModelPtr anim,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<Ogre::Real> >& controllers,
                NiModel *skeleton,
                NiModel *bow)
#endif
    }
}
