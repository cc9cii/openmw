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
*/
#include "nimeshloader.hpp"

#include <cassert>

#include <OgreMesh.h>
#include <OgreMeshManager.h>

#include "nimodel.hpp"
#include "nimodelmanager.hpp"
#include "ninode.hpp"

namespace NiBtOgre
{
    std::map<Ogre::Resource*, NiMeshLoader::ModelBuildInfo> NiMeshLoader::sModelBuildInfoMap;

    NiMeshLoader::NiMeshLoader()
    {
    }

    NiMeshLoader::~NiMeshLoader()
    {
    }

    Ogre::MeshPtr NiMeshLoader::createManual(const Ogre::String& name, const Ogre::String& group)
    {
        Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

        // Don't try to get existing, create should fail if already exists
        if( meshManager.getResourceByName( name, group ) )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_DUPLICATE_ITEM,
                         "Mesh with name '" + name + "' already exists.",
                         "NiMeshLoader::createManual" );
        }

        return meshManager.createManual(name, group, this);
    }

    Ogre::MeshPtr NiMeshLoader::createMesh(const Ogre::String& name, const Ogre::String& group,
            NiModel *model, std::int32_t ninodeIndex, const Ogre::String skeleton)
    {
        // Create manual model which calls back self to load
        Ogre::MeshPtr pMesh = createManual(name, group);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = (skeleton.empty() ?  MBT_Object : MBT_Skinned);
        bInfo.model = model;
        bInfo.ninodeIndex = ninodeIndex;
        bInfo.skeleton = skeleton;
        sModelBuildInfoMap[pMesh.get()] = bInfo;

        return pMesh; // at this point the mesh is created but not yet loaded
    }

    // WARN: must ensure that 'name' includes the NPC EditorID and 'model' must also be the NPC
    // specific one.
    Ogre::MeshPtr NiMeshLoader::createMorphedMesh(const Ogre::String& name, const Ogre::String& group,
            const Ogre::String& morphedTexture, Ogre::ResourcePtr model, std::int32_t ninodeIndex)
    {
        // Create manual model which calls back self to load
        Ogre::MeshPtr pMesh = createManual(name, group);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Morphed;
        bInfo.modelPtr = model;
        bInfo.ninodeIndex = ninodeIndex;
        // FIXME: create an Ogre::Material but not yet load and store in bInfo?
        sModelBuildInfoMap[pMesh.get()] = bInfo;

        return pMesh; // at this point the mesh is created but not yet loaded
    }

    // this method is not called until the associated Ogre::Entity is created
    void NiMeshLoader::loadResource(Ogre::Resource *res)
    {
        Ogre::Mesh *mesh = static_cast<Ogre::Mesh*>(res);

        // Find build parameters
        std::map<Ogre::Resource*, ModelBuildInfo>::iterator it = sModelBuildInfoMap.find(res);
        if (it == sModelBuildInfoMap.end())
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find build parameters for " + res->getName(),
                         "NiMeshLoader::loadResource");
        }

        ModelBuildInfo& bInfo = it->second;
        switch(bInfo.type)
        {
        case MBT_Object:
            loadManualMesh(mesh, bInfo);
            break;
        case MBT_Skinned:
            loadManualSkinnedMesh(mesh, bInfo);
            break;
        case MBT_Morphed:
            loadManualMorphedMesh(mesh, bInfo);
            break;
        default:
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Unknown build parameters for " + res->getName(),
                         "NiMeshLoader::loadResource");
        }
    }

    void NiMeshLoader::loadManualMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params)
    {
        NiNode *node = nullptr;

        try
        {
            node = params.model->getRef<NiNode>(params.ninodeIndex);
        }
        catch (...)
        {
            // mesh name = base model + # + NiNode index + @ + NiNode name
            std::string meshName = pMesh->getName();
            std::size_t pos = meshName.find_first_of("#");
            if (pos == std::string::npos)
                OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                             "Mesh name convention not met for " + pMesh->getName(),
                             "NiMeshLoader::loadManualMesh");

            std::string modelName = meshName.substr(0, pos);

            NiModelManager& modelManager = NiModelManager::getSingleton();
            NiModelPtr model = modelManager.getOrLoadByName(modelName, pMesh->getGroup());

            node = model->getRef<NiNode>(params.ninodeIndex);
        }

        node->buildMesh(pMesh);
    }

    void NiMeshLoader::loadManualSkinnedMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params)
    {
        NiNode *node = nullptr;

        try
        {
            node = params.model->getRef<NiNode>(params.ninodeIndex);
        }
        catch (...)
        {
            // mesh name = skeleton name + _ + base model + # + NiNode index + @ + NiNode name
            std::string meshName = pMesh->getName();
            std::size_t pos = meshName.find_first_of("#");
            if (pos == std::string::npos)
                OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                             "Mesh name convention not met for " + pMesh->getName(),
                             "NiMeshLoader::loadManualSkinnedMesh");

            size_t start = (params.skeleton.empty()) ? 0 : params.skeleton.size() + 1; // +1 for "_"
            std::string modelName = meshName.substr(start, pos-start);

            NiModelManager& modelManager = NiModelManager::getSingleton();
            NiModelPtr model = modelManager.getByName(modelName, pMesh->getGroup());

            // FIXME: probably have to do something about the skeleton the mesh needs to use
            //        (maybe create/load again?)

            node = model->getRef<NiNode>(params.ninodeIndex);
        }

        node->buildMesh(pMesh);
    }

    // morphed meshes hold the shared pointer to NiModel so in theory the model should never be destroyed
    void NiMeshLoader::loadManualMorphedMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params)
    {
        NiModelPtr model = Ogre::static_pointer_cast<NiModel>(params.modelPtr);
        NiNode *node = model->getRef<NiNode>(params.ninodeIndex); // get NiNode

        // FIXME
        // load material from params
        //Ogre::MaterialPtr mat =
        //node->setMaterial(mat);

        node->buildMesh(pMesh);
    }
}
