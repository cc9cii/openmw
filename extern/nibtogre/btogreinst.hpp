/*
  Copyright (C) 2018 cc9cii

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
#ifndef NIBTOGRE_BTOGREINST_H
#define NIBTOGRE_BTOGREINST_H

#include <vector>
#include <map>
#include <cstdint>
#include <memory>

#include <btBulletDynamicsCommon.h>

#include <OgreController.h>

#include "nimodel.hpp"
#include "nimeshloader.hpp"
#include "nidata.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace NiBtOgre
{
    struct bhkEntity;
    struct bhkConstraint;

    enum BuildFlags {
        Flag_EnableHavok        = 0x0001,
        Flag_EnableCollision    = 0x0002,
        Flag_EnableAnimation    = 0x0008,
        Flag_HasSkin            = 0x0010,
        Flag_IgnoreEditorMarker = 0x0020, // FIXME: no longer used?
        Flag_NonRootObject      = 0x1000, // FIXME: no longer used?
        Flag_None               = 0x0000
    };

    // Associating controllers with Sub-Entities
    // =========================================
    //
    // NiNode::build                               | iterate through NiNode children
    //   NiGeometry::build                         | NiTriShape/NiTriStrips is a sub-mesh
    //   : BtOgreInst::registerNiGeometry          |
    //   :   NiMeshLoader::registerSubMeshGeometry | store pointer to sub-mesh
    //   :                                         |
    //   BtOgreInst::buildMeshAndEntity            |
    //     Ogre::MeshManager::createManual         | register unique mesh name and associated loader
    //     Ogre::SceneManager::createEntity        | load the mesh using the unique name
    //     :    |                                  |
    //     :    |                                  | callback via Ogre::ManualResourceLoader
    //     :    v                                  |
    //     : NiMeshLoader::loadResource            | build the mesh (using stored sub-mesh pointers)
    //     :   NiGeometry::createSubMesh           |
    //     :     NiGeometry::applyProperties       | 0..N properties each with 0..N controllers
    //     :     :
    //     :     :<-------------------------------------- assess properties and apply material here?
    //     :
    //     :<-------------------------------------------- associate controllers to sub-entities here?
    //
    // TODD: lifetime of NiGeometry?  Mesh loader will reference it later.
    struct BtOgreInst
    {
        // A NiNode with NiGeometry children may be an Ogre::Entity.
#if 0
        struct EntityConstructionInfo
        {
            // A unique name for the mesh.  Uniqueness is needed by the Ogre 1.x MeshManager,
            // possibly Ogre 2.x as well. Concatenation of model name, ":", parent NiNode name.
            // (e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01)
            std::string                 mMeshAndNodeName;

            std::auto_ptr<NiMeshLoader> mMeshLoader; // FIXME: was unique_ptr but can't remember why

            // Each sub-entity may have associated controllers.
            //
            // The key to the map is the index of the sub-mesh that has NiProperty with NiTimeControllers
            // (note that there may be multiple controllers, hence a vector).
            std::map<std::uint32_t, std::vector<Ogre::Controller<float> > > mSubEntityControllers;
        };
#endif
        // Keep the model around in case Ogre wants to load the resource (i.e. Mesh) again
        std::auto_ptr<NiBtOgre::NiModel> mModel;

        int mFlags; // some global properties
        Ogre::SceneNode *mBaseSceneNode;
        std::vector<std::pair<bhkConstraint*, bhkEntity*> > mbhkConstraints;

        // The key to the map is the block index of the parent NiNode; each child may add a mesh loader.
        // The first of the construction info is 'name' parameter in registerNiGeometry (see below).
        //std::map<std::uint32_t, EntityConstructionInfo> mEntityCIMap;
        std::map<std::uint32_t, std::pair<std::string, std::unique_ptr<NiMeshLoader> > > mMeshes;

        // index = parent NiNode's block index
        // name  = concatenation of model, ":", parent NiNode name
        //        (e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01)
        void registerNiGeometry(std::uint32_t nodeIndex, const std::string& name, NiGeometry* geometry);

        // called from NiNode after registering all the child NiGeometry objects (NiTriShape or NiTriStrips)
        void buildMeshAndEntity();

        std::vector<AnimTrackInterpolator<float>*> mInterpolators; // prevent memory leak

        // key is the block index of the target object (i.e. typically NiNode)
        std::map<std::uint32_t, std::unique_ptr<btRigidBody> > mRigidBodies;


        std::map<int, std::vector<int> > mGeomMorpherControllerMap;


        // btCollisionShapes
        // btConstraints

        BtOgreInst(Ogre::SceneNode *baseNode);
        ~BtOgreInst() {
            for (unsigned int i = 0; i < mInterpolators.size(); ++i)
                delete mInterpolators[i];
        }

        // register with bullet dynamics, make entities visible, etc
        void instantiate();
    };
}

#endif // NIBTOGRE_BTOGREINST_H