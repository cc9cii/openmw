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

  Some of the Ogre code in this file is based on v0.36 of OpenMW.

*/
#include "nigeometry.hpp"

//#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBufferManager.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreSkeletonManager.h>
#include <OgreSkeleton.h>
#include <OgreBone.h>
#include <OgreKeyFrame.h>

#include <extern/fglib/fgtri.hpp>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "ninode.hpp"
#include "btogreinst.hpp"
#include "nidata.hpp"     // NiGeometryData
#include "niproperty.hpp" // NiProperty
#include "nitimecontroller.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

NiBtOgre::NiGeometry::NiGeometry(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiAVObject(index, stream, model, data), mHasShader(false), mDirtyFlag(false)
    , mParent(nullptr)
{
#if 0
    // Some NiTriShapes are "Shadow", possibly simplified mesh for animated (i.e. non-static)
    // objects to cast shadows
    if ((mFlags & 0x40) != 0) // FIXME: testing only, 67 == 0x43, 69 = 0x45
    {
        std::cout << "Shadow : " << model.getName() << " : " << model.indexToString(mName) << std::endl;
    }
#endif
    stream->read(mDataRef);
    stream->read(mSkinInstanceRef);

    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7 (TES5)
    {
        std::uint32_t numMaterials;
        stream->read(numMaterials);

        if (numMaterials != 0)
        {
            mMaterialName.resize(numMaterials);
            for (unsigned int i = 0; i < numMaterials; ++i)
                stream->readLongString(mMaterialName.at(i));

            mMaterialExtraData.resize(numMaterials);
            for (unsigned int i = 0; i < numMaterials; ++i)
                stream->read(mMaterialExtraData.at(i));
        }

        stream->skip(sizeof(std::int32_t)); // active material?
    }

    // apparently for userVer2 < 100
    if (stream->nifVer() >= 0x0a000100 && stream->nifVer() <= 0x14010003)
    {
        if (mHasShader = stream->getBool())
        {
            stream->readLongString(mShaderName);
            stream->skip(sizeof(std::int32_t)); // unknown integer
        }
    }

    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7 (TES5)
    {
        mDirtyFlag = stream->getBool();
        if (stream->userVer() == 12) // not present in FO3?
        {
            mBSProperties.resize(2);
            stream->read(mBSProperties.at(0));
            stream->read(mBSProperties.at(1));
        }
    }

    // special case for landscape LOD mesh (normally NiNode or BSFadeNode should have selfRef() == 0)
    if (NiObject::selfRef() == 0)
    {
        if (mNameIndex == -1)
            mNameIndex = const_cast<NiModel&>(model).addString("NiGeom"+std::to_string(index)); // const hack
        mParent = const_cast<NiModel&>(model).insertDummyBlock<NiNode>("NiNode"); // WARN: const hack

        // add NiTexturingProperty and NiSourceTexture blocks
        if (NiAVObject::mProperty.empty())
        {
            NiObject *prop
                = const_cast<NiModel&>(model).insertDummyBlock<NiTexturingProperty>("NiTexturingProperty");
            mProperty.push_back(prop->selfRef());
        }
    }

    if (!mParent)
        mParent = data.getNiNodeParent((NiAVObjectRef)NiObject::mSelfRef);
}

NiBtOgre::NiTriBasedGeom::NiTriBasedGeom(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiGeometry(index, stream, model, data)
    , mUseMorphed(false)
    , mData(data) // for accessing mSkeleton later
    , mSubMeshIndex(0)
{
    mMorphVertices.clear();

    // use the data read from NiAVObject to make mLocalTransform
    mLocalTransform.makeTransform(mTranslation, Ogre::Vector3(mScale), Ogre::Quaternion(mRotation));

    //  at least one shape's parent is NiTriStrips i.e. the world transform will be required
    //  FIXME: physics shape is a little offset from the render
    //  Furniture\MiddleClass\BearSkinRug01.NIF (0001C7CA)
    //  COC "ICMarketDistrictJensinesGoodasNewMerchandise"
    if (mCollisionObjectRef != -1)
        NiAVObject::mWorldTransform = NiGeometry::mParent->getWorldTransform() * mLocalTransform;

    // FIXME: some controller target nodes have dummy meshes - how to distinguish these?
    // e.g. FO3 block 182 "door_temp10" in UtlDoor01.NIF
    // do not register if "hidden" e.g. FO3 block 17 "Trigger" in Traps\PressurePlate.NIF
    bool isHidden = true;
    std::string typeName;
    for (std::size_t i = 0; i < NiAVObject::mProperty.size(); ++i)
    {
        if (mProperty[i] == -1)
            continue;

        // NOTE: can't use dynamic casting since the property object may not be constructed yet
        typeName = model.blockType(mProperty[i]);
        if (typeName == "NiTexturingProperty" || // TODO: add other property types?
            typeName == "BSShaderPPLightingProperty" || typeName == "BSShaderNoLightingProperty")
        {
            isHidden = false;
            break;
        }
    }

    if (mModel.nifVer() >= 0x14020007) // FIXME: user version ignored
    {
        if (!mBSProperties.empty())
            isHidden = false; // FIXME: Skyrim workaround
    }

// FIXME: temp disable lighting fx (e.g. fxglowsimpfill....)
//#if 0
    bool hasAlpha = false;
    bool hasStencil = false;
    for (std::size_t i = 0; i < NiAVObject::mProperty.size(); ++i)
    {
        if (mProperty[i] == -1)
            continue;

        typeName = model.blockType(mProperty[i]);
        if (typeName == "NiAlphaProperty")
            hasAlpha = true;

        if (typeName == "NiStencilProperty")
            hasStencil = true;
    }
    if (hasAlpha && hasStencil && model.getName().find("effects") != std::string::npos)
    {
        isHidden = true;
        std::cout << "hiding " << model.getName() << std::endl;
    }
//#endif
    if (!isHidden)
        mParent->registerSubMesh(this);

    // if there is node animation in the model include any sub-mesh to be part of the animation
    // FIXME: HACK for testing; may add bones that are not necessary
//  if (data.mSkelLeafIndices.size() > 0 && (model.blockType(mParent.selfRef()) == "NiNode" ||
//              model.blockType(mParent.selfRef()) == "BSFadeNode")) // FIXME
//      data.addBoneTreeLeafIndex(mParent.selfRef()); // may attempt to add bones already added
}

// workaround to identify the sub-mesh with visible skin
bool NiBtOgre::NiTriBasedGeom::hasVisibleSkin() const
{
    for (std::size_t i = 0; i < NiAVObject::mProperty.size(); ++i)
    {
        NiProperty* property = mModel.getRef<NiProperty>(NiAVObject::mProperty[i]);
        if (!property)
            continue;

        // FO3/FONV - note must come before name check since it can be empty
        if (mModel.blockType(NiAVObject::mProperty[i]) == "BSShaderPPLightingProperty")
        {
            BSShaderLightingProperty* property
                = mModel.getRef<BSShaderLightingProperty>(NiAVObject::mProperty[i]);

            if (property->mShaderType == 14)
                return true;
        }

        // TES4
        std::int32_t nameIndex = property->getNameIndex();
        if (nameIndex == -1)
            continue;

        std::string propertyName = mModel.indexToString(nameIndex);
        if (propertyName == "skin" || propertyName == "Skin")
            return true;
    }

    return false;
}

void NiBtOgre::NiTriBasedGeom::fillBodyParts(std::vector<std::uint16_t>& bodyParts) const
{
    if (mSkinInstanceRef == -1)
        return;

    const BSDismemberSkinInstance *skinInstance = mModel.getRef<BSDismemberSkinInstance>(mSkinInstanceRef);
    for (std::size_t i = 0; i < skinInstance->mPartitions.size(); ++i)
        bodyParts.push_back(skinInstance->mPartitions[i].bodyPart);
}

/*
 * **************************** DO NOT USE AS IS *******************************
 *
 * All of the material handling code, including the class OgreMaterial, are
 * just temporary and were never intended to stay as they are.  At some point
 * the NIF code was going to be ported to some other rendering engine and it
 * would have been (mostly) a waste of time to work on this.
 *
 * *****************************************************************************
 */

// Can't remember why I wanted SubEntityController (a base class maybe?)
// But it can be useful (map with sub-entity index) for BtOgreInst to associate them later.
// Should keep such a map in the NiModel tree somewhere so that a retrieved one from the cache
// already has them built.
//
// OgreMaterial would be an object that has all the properties the engine (in this case
// Ogre) can support.  It would be passed to each of the NiProperty objects for this sub-mesh
// and updated as required.
//
// Kind of works like Nif::Node::getProperties and NIFMaterialLoader::getMaterial
// (both called from NIFMeshLoader::createSubMesh).

// the parameter 'controllers' are keyed to the index of this sub-mesh
std::string NiBtOgre::NiTriBasedGeom::getMaterial()
{
    const NiGeometryData* data = mModel.getRef<NiGeometryData>(mDataRef);
    mOgreMaterial.vertexColor = (data->mVertexColors.size() != 0);

    // NiGeometry is derived from NiAVObject, so it has its own transform and properties (like NiNode)
    for (std::size_t i = 0; i < NiAVObject::mProperty.size(); ++i)
    {
        NiProperty* property = mModel.getRef<NiProperty>(NiAVObject::mProperty[i]);

        // FIXME: for testing only; note some properties have a blank name
        //std::cout << "property " << mModel.indexToString(property->getNameIndex()) << std::endl;

        // NOTE: some property (NiTexturingProperty) can have a chain of NiTimeControllers as well.
        property->applyMaterialProperty(mOgreMaterial, mControllers);

#if 0
        while (property->mControllerRef != -1)
        {
            NiTimeController* controller = mModel.getRef<NiTimeController*>(property->mControllerRef);

            // current place in controller map
            //
            //       BtOgreInst
            //          o
            //          | 0..N
            //      *NiMeshLoader       map key: parent NiNode block index
            //          o
            //          | 1..N
            //       *NiGeometry        vector index: sub mesh index
            //          o
            //          | 0..N
            //       *NiProperty
            //          o
            //          | 0..N
            //     *NiTimeController
            //
            int n = inst->mEntityCIMap[mParent->selfRef()].size(); // n-1 is the current sub mesh index

            // under this sub mesh index there are 0..N time controllers
        }
#endif
    }

    if (mModel.nifVer() >= 0x14020007) // FIXME: user version ignored
    {
        for (size_t i = 0; i < mBSProperties.size(); ++i)
        {
            if (mBSProperties[i] != -1)
            {
                NiProperty* property = mModel.getRef<NiProperty>(mBSProperties[i]);
                property->applyMaterialProperty(mOgreMaterial, mControllers);
            }
        }
    }

    // if an external texture was supplied, overwrite texture property but only if it is an exposed skin
    std::string skinTexture = mParent->getSkinTexture();
    bool useExt = false;
    if (!skinTexture.empty() && skinTexture != "textures\\" &&
        mOgreMaterial.texName.find(NiTexturingProperty::Texture_Base) != mOgreMaterial.texName.end())
    {
        // WARN: we rely on getMaterial() being called *after* the sub-meshes have been built
        if (mSkinInstanceRef == -1 && mData.mMeshBuildList.size() == 1 && mParent->getNumSubMeshChildren() == 1)
        {
            // If we're the only sub-mesh then force replace but this does not work for
            // Clothing\MiddleClass\04\M\Pants.NIF.  We need to force only for body/head parts.
            // NOTE: eyes, ears, hair are not skinned (but head is)
            mOgreMaterial.setExternalTexture(skinTexture); // TODO: ears need morphing
            useExt = true;
        }
        else if (mSkinInstanceRef != -1) // skin showing clothes/armor should all be skinned?
        {
            // loop again here rather than check the name of every property in the loop above
            if (hasVisibleSkin()
                ||
                skinTexture.find("Dremora") != std::string::npos) // HACK for Dremora head
            {
                mOgreMaterial.setExternalTexture(skinTexture);
                useExt = true;
            }
        }
    }

    // now the sub-mesh knows about *all* the properties, retrieve or create a material
    // NOTE: needs a unique name (in case of creation) for Ogre MaterialManager
    // TODO: probably don't need the parent node name, commented out for now
    return mOgreMaterial.getOrCreateMaterial((useExt ? skinTexture+"_" : "")+
                                             mModel.getName()+
                                             "%"+/*mParent->getNodeName()+":"+*/
                                             mModel.indexToString(NiObjectNET::getNameIndex()));
}

//void NiBtOgre::NiTriBasedGeom::setVertices(std::unique_ptr<std::vector<Ogre::Vector3> > morphedVertices)
//{
//    mMorphedVertices = std::move(morphedVertices);
//
//    if (mMorphedVertices.get()->empty())
//        throw std::runtime_error("NiTriBasedGeom: trying to re-popluate morphed vertices");
//}

//const std::vector<Ogre::Vector3>& NiBtOgre::NiTriBasedGeom::vertices()
//{
    //return mModel.getRef<NiGeometryData>(mDataRef)->mVertices;
//}

const std::vector<Ogre::Vector3>& NiBtOgre::NiTriBasedGeom::getVertices(bool morphed)
{
    // TODO: maybe check that either or both of them are not empty?
    const NiGeometryData* data = mModel.getRef<NiGeometryData>(mDataRef);
    //if (mMorphedVertices.get() != nullptr && (mMorphedVertices->size() == data->mVertices.size()))
        //return mMorphedVertices.get();
    //else
        //return &(data->mVertices);
    if (morphed && mMorphVertices.size() == data->mVertices.size())
        return mMorphVertices;
    else
        return data->mVertices;
}

// build Ogre mesh and apply shader/material using scene
//
// 1. get data and skin objects
// 2. get shader if present
// 3. TES5 has more stuff to do
//
// Should check if animated, has skeleton and/or has skin (static objects do not have these)
//
// Some NiTriStrips have NiBinaryExtraData (tangent space) - not sure what to do with them
bool NiBtOgre::NiTriBasedGeom::buildSubMesh(Ogre::Mesh *mesh, BoundsFinder& bounds, bool hasSkinnedSubMesh)
{
    // If BuildData::mBuildFlags says no animation, no havok then most likely static.  Also check
    // if there is a skin instance (and maybe also see if Oblivion layer is OL_STATIC even though
    // that flag is just for the editor)

    // If (mModel.buildData().havokEnabled())
    //
    // This means physics will determine the position of the rendering mesh. i.e. the
    // Ogre::Entity for the corresponding Ogre::Mesh will be controlled via its Ogre::SceneNode.
    //
    // Each NiGeometry probably should be a SubMesh. e.g. clutter/apple01.nif has 4 sub meshes
    // defined by 4 NiTriStrips, Apple01:0 - Apple01:3.  The ChildSceneNode associated with the
    // target of bhkRigidBody, Apple01, is then controlled by Bullet. Apple01 is the parent of
    // Apple01:0 - Apple01:3.
    //
    // There seems to be a convention with the names - e.g.
    //
    //   NiNode: TargetHeavyTarget <---------- Mesh
    //     bhkCollisionObject
    //     NiTriStrips TargetHeavyTarget:0 <-- SubMesh (front hitting surface)
    //     NiTriStrips TargetHeavyTarget:1 <-- SubMesh (top metal hooks)
    //     NiTriStrips TargetHeavyTarget:2 <-- SubMesh (main body)
    //     NiTriStrips TargetHeavyTarget:3 <-- SubMesh (side surfaces)
    //     NiTriStrips TargetHeavyTarget:4 <-- SubMesh (vertical metal bars)
    //
    // Note ragdoll here is done with Bullet moving the SceneNode associated with the Entity.
    // i.e. no bones are used (or required)

    // If the mesh is static, we need to apply the full transform of the model else we need the
    // transform only from the NiNode that is the base of the animation.

    const NiGeometryData* data = mModel.getRef<NiGeometryData>(mDataRef);
    //std::string type = mModel.blockType(data->selfRef()); // FIXME: temp testing

    Ogre::Matrix4 transform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    bool isStatic = false;

    // ICDoor04, UpperChest02 - these have animation flag but are static. Therefore we can't
    // rely on that flag.  We have to confirm with an extra check by calling isDynamicMesh().
    //
    // NOTE: Flag_HasSkin may not be set if only some of the NiGeometry blocks have skin.
    // NOTE: need to check for havok after the check for animation because some doors can have havok enabled
    //       e.g. GateDoor03 of ARNHallGateDoor01.NIF (COC "Vilverin")
    if (mModel.buildData().animEnabled())
    {
        NiNodeRef controlledNodeRef = 0;
        if (mParent->isDynamicMesh(&controlledNodeRef))
        {
            mParent->getTransform(controlledNodeRef, transform); // get transform to controlledNode
            transform = transform * mLocalTransform;

            if (controlledNodeRef != mParent->selfRef())
                mParent->setAnimRoot(controlledNodeRef);
        }
        else
            transform = mLocalTransform; // UpperScales.01.NIF
    }
    else if (mModel.buildData().havokEnabled())
    {
        transform = mLocalTransform;
    }
    else if (mSkinInstanceRef != -1)
    {
        transform = mParent->getLocalTransform() * mLocalTransform;
    }
    else
    {
        isStatic = true;
        // static, the entities will be attached to the root node
        transform = mParent->getWorldTransform() * mLocalTransform;
    }

#if 0
    bool isStatic = true;

//  Ogre::Matrix4 localTransform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
//  localTransform.makeTransform(NiAVObject::mTranslation,
//                               Ogre::Vector3(NiAVObject::mScale),
//                               Ogre::Quaternion(NiAVObject::mRotation));

    Ogre::Matrix4 transform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    //if (!isStatic)
    if (mSkinInstanceRef != -1) // using the local transform doesn't seem to do much
    {
#if 1
        transform = mParent->getLocalTransform() * mLocalTransform;
#else
        const NiSkinInstance *skinInstance = mModel.getRef<NiSkinInstance>(mSkinInstanceRef);
        const NiSkinData *skinData = mModel.getRef<NiSkinData>(skinInstance->mDataRef);

        Ogre::Matrix4 skinTransform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
        skinTransform.makeTransform(skinData->mSkinTransform.translation,
                                     Ogre::Vector3(skinData->mSkinTransform.scale),
                                     Ogre::Quaternion(skinData->mSkinTransform.rotation));
        transform = mParent->getLocalTransform() * mLocalTransform * skinTransform;
#endif
    }
    //else if (mModel.getName().find("geardoor") != std::string::npos)
        //transform = mParent->getLocalTransform() * mLocalTransform;
    else
        transform = mParent->getWorldTransform() * mLocalTransform;
#endif

    // NOTE: below code copied from components/nifogre/mesh.cpp (OpenMW)

    const std::vector<Ogre::Vector3>& srcVerts = getVertices(mUseMorphed ? true : false);
    std::vector<Ogre::Vector3> vertices;
    vertices.resize(srcVerts.size());

    std::vector<Ogre::Vector3> srcNorms = data->mNormals; // FIXME: do these need to be re-calculated for FG?
    Ogre::HardwareBuffer::Usage vertUsage = Ogre::HardwareBuffer::HBU_STATIC;
    bool vertShadowBuffer = false;

    // TODO: seems to make no difference to vertex anim
    if (!isStatic || mModel.hasSkeleton())
    {
        vertUsage = Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY;
        vertShadowBuffer = true;
    }
    std::string skelName = "";
    if (mModel.hasSkeleton())
        skelName = mModel.getSkeleton()->getName();

    if (0)//mSkinInstanceRef != -1)// && skelName.find("character") != std::string::npos
                               //&& mModel.getName().find("hand") != std::string::npos
                               //|| mModel.getName().find("foot") != std::string::npos)
                               //&& skelName.find("skeleton") != std::string::npos)
    {
        const NiSkinInstance *skinInstance = mModel.getRef<NiSkinInstance>(mSkinInstanceRef);
        const NiSkinData *skinData = mModel.getRef<NiSkinData>(skinInstance->mDataRef);

        NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
        NiModelPtr skelModel = modelManager.getByName(skelName, "General");

        for(size_t i = 0; i < skinInstance->mBoneRefs.size(); ++i)
        {
            std::string nodeName = mModel.getRef<NiNode>(skinInstance->mBoneRefs[i])->getName();
            const std::map<std::string, NiAVObjectRef>& objPalette = skelModel->getObjectPalette();
            std::map<std::string, NiAVObjectRef>::const_iterator it = objPalette.find(nodeName);
            if (it == objPalette.end())
                throw std::runtime_error("NiTriBasedGeom: missing bone "+nodeName);

            Ogre::Matrix4 skelNodeTransform = skelModel->getRef<NiNode>(it->second)->getWorldTransform();
            Ogre::Matrix4 mat;
            mat.makeTransform(skinData->mBoneList[i].skinTransform.translation,
                              Ogre::Vector3(skinData->mBoneList[i].skinTransform.scale),
                              skinData->mBoneList[i].skinTransform.rotation);
            mat = skelNodeTransform * mat;

            Ogre::Vector3 skelPos, shapePos;
            Ogre::Quaternion skelRot, shapeRot;
            Ogre::Vector3 skelScale, shapeScale;

            mat.decomposition(skelPos, skelScale, skelRot);
            transform.decomposition(shapePos, shapeScale, shapeRot);

            float l = (skelPos - shapePos).length();
            if (l > 3.f)
                std::cout << mModel.getName() << ":" << nodeName << " pos " << l << std::endl;

            Ogre::Quaternion a = skelRot * shapeRot;
            Ogre::Degree d;
            Ogre::Vector3 axis;
            a.ToAngleAxis(d, axis);
            if (d.valueDegrees() > 10)
                std::cout << mModel.getName() << ":" << nodeName << " rot " << d.valueDegrees() << std::endl;
        }
    }

    // transform the vertices and normals into position.
    for (size_t i = 0;i < vertices.size();i++)
    {
        Ogre::Vector4 vec4(srcVerts[i].x, srcVerts[i].y, srcVerts[i].z, 1.0f);
        vec4 = transform*vec4;
        vertices[i] = Ogre::Vector3(&vec4[0]);
    }

    for (size_t i = 0;i < srcNorms.size();i++)
    {
        Ogre::Vector4 vec4(srcNorms[i].x, srcNorms[i].y, srcNorms[i].z, 0.0f);
        vec4 = transform*vec4;
        srcNorms[i] = Ogre::Vector3(&vec4[0]);
    }

    // update bounds including all the sub meshes
    bounds.add(&vertices[0][0], vertices.size());
    if(!bounds.isValid())
    {
        float v[3] = { 0.0f, 0.0f, 0.0f };
        bounds.add(&v[0], 1);
    }

    // This function is just one long stream of Ogre-barf, but it works great.
    Ogre::HardwareBufferManager *hwBufMgr = Ogre::HardwareBufferManager::getSingletonPtr();
    Ogre::HardwareVertexBufferSharedPtr vbuf;
    Ogre::HardwareIndexBufferSharedPtr ibuf;
    Ogre::VertexBufferBinding *bind;
    Ogre::VertexDeclaration *decl;
    int nextBuf = 0;

    std::string subMeshName = mModel.indexToString(NiObjectNET::mNameIndex); // TODO: are the names unique?
    Ogre::SubMesh *sub = mesh->createSubMesh(subMeshName);

    // get the correct subMesh index
    const Ogre::Mesh::SubMeshNameMap& subMeshNameMap = mesh->getSubMeshNameMap();
    Ogre::Mesh::SubMeshNameMap::const_iterator it = subMeshNameMap.find(subMeshName);
    if (it != subMeshNameMap.end())
        mSubMeshIndex = it->second;
    else
        throw std::runtime_error("NiTriBasedGeom: SubMesh index not found"); // shouldn't happen

    // Add vertices
    sub->useSharedVertices = false;
    sub->vertexData = new Ogre::VertexData(); // NOTE: Ogre::SubMesh owns vertexData and will delete in dtor
    sub->vertexData->vertexStart = 0;
    sub->vertexData->vertexCount = vertices.size();

    decl = sub->vertexData->vertexDeclaration;
    bind = sub->vertexData->vertexBufferBinding;
    if (vertices.size())
    {
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                            vertices.size(), vertUsage, vertShadowBuffer);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &vertices[0][0], true);

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex normals
    if (srcNorms.size())
    {
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                            srcNorms.size(), vertUsage, vertShadowBuffer);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &srcNorms[0][0], true);

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex colors
    const std::vector<Ogre::Vector4> &colors = data->mVertexColors;
    if (colors.size())
    {
        Ogre::RenderSystem *rs = Ogre::Root::getSingleton().getRenderSystem();
        std::vector<Ogre::RGBA> colorsRGB(colors.size());
        for (size_t i = 0; i < colorsRGB.size(); ++i)
        {
#if 0
            Ogre::ColourValue clr(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
            if (mModel.getName().find("air") != std::string::npos)
            {
                Ogre::Vector3 col(colors[i][0], colors[i][1], colors[i][2]);
                col += Ogre::Vector3(110/256, 110/256, 110/256);
                clr = Ogre::ColourValue(col.x, col.y, col.z, colors[i][3]);
            }
#else
            Ogre::ColourValue clr(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
#endif
            rs->convertColourValue(clr, &colorsRGB[i]);
        }
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                            colorsRGB.size(), Ogre::HardwareBuffer::HBU_STATIC);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &colorsRGB[0], true);
        decl->addElement(nextBuf, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Texture UV coordinates
    size_t numUVs = data->mUVSets.size();
    if (numUVs)
    {
        size_t elemSize = Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);

        for(unsigned short i = 0; i < numUVs; ++i)
            decl->addElement(nextBuf, (unsigned short)elemSize*i, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, i);

        vbuf = hwBufMgr->createVertexBuffer(decl->getVertexSize(nextBuf), vertices.size(),
                                            Ogre::HardwareBuffer::HBU_STATIC);

        std::vector<Ogre::Vector2> allUVs;
        allUVs.reserve(vertices.size()*numUVs);
        for (size_t vert = 0; vert < vertices.size(); ++vert)
            for (size_t i = 0; i < numUVs; i++)
                allUVs.push_back(data->mUVSets[i][vert]);

        vbuf->writeData(0, elemSize*vertices.size()*numUVs, &allUVs[0], true);

        bind->setBinding(nextBuf++, vbuf);
    }

    // Triangle faces
    const std::vector<uint16_t> *srcIdx;
    srcIdx = &static_cast<const NiTriBasedGeomData*>(data)->getTriangles();

    if (srcIdx->size())
    {
        ibuf = hwBufMgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, srcIdx->size(),
                                           Ogre::HardwareBuffer::HBU_STATIC);
        ibuf->writeData(0, ibuf->getSizeInBytes(), &(*srcIdx)[0], true);
        sub->indexData->indexBuffer = ibuf;
        sub->indexData->indexCount = srcIdx->size();
        sub->indexData->indexStart = 0;
    }

/*
 * **************************** DO NOT USE AS IS *******************************
 *
 * Current handling of skinned sub-meshes are flawed.
 *
 * 1. It cannot handle weights from a bone offset (i.e. take NiSkinData and Bone
 *    List SkinTransforms into account) which breaks many animations including
 *    the Spider Daedra.  I believe all the head models use this but currently
 *    worked around by attaching to the head bone.
 *
 *    However Skyrim's FaceGen head models won't work without a proper fix.
 *    e.g. actors\character\facegendata\facegeom\skyrim.esm\00013277.nif
 *
 * 2. It doesn't use Skin Partition. Fortunately most of Oblivion's models use
 *    only one partition so the impact is minor up to now, but Skyrim may be
 *    different.  The intention was to move to hardware skinning and pass up to
 *    4 bones to the shaders but never got around to it.
 *
 * *****************************************************************************
 */

    //$ find . -type f -print0 | xargs -0 strings -f | grep -E 'NiSkinData'
    //./architecture/arena/arenaspectatorf01.nif: NiSkinData    <-- ControllerSequence
    //./architecture/arena/arenaspectatorm01.nif: NiSkinData    <-- ControllerSequence
    //./architecture/ships/shipflag01.nif: NiSkinData
    //./architecture/ships/shipwrecksail01.nif: NiSkinData      <-- havok animation
    //./architecture/ships/shipwrecksail02.nif: NiSkinData      <-- havok animation
    //./architecture/ships/shipwrecksail03.nif: NiSkinData      <-- havok animation
    //./armor/amber/f/boots.nif: NiSkinData
    //./armor/amber/f/cuirass.nif: NiSkinData
    //...
    //./creatures/zombie/righttorso02.nif: NiSkinData
    //./dungeons/caves/clutter01/roperock01.nif: NiSkinData     <-- havok + ControllerSequence
    //./dungeons/caves/clutter01/rythorspendant01.nif: NiSkinData
    //./dungeons/caves/roperock01.nif: NiSkinData               <-- havok + ControllerSequence
    //./dungeons/chargen/ropebucket01.nif: NiSkinData           <-- havok animation
    //./dungeons/misc/mdtapestryskinned01.nif: NiSkinData       <-- havok animation
    //./dungeons/misc/necrotapestryskinned01.nif: NiSkinData    <-- havok animation
    //./dungeons/misc/roothavok01.nif: NiSkinData
    //./dungeons/misc/roothavok02.nif: NiSkinData
    //./dungeons/misc/roothavok04.nif: NiSkinData
    //./dungeons/misc/roothavok05.nif: NiSkinData
    //./dungeons/misc/roothavok06.nif: NiSkinData
    //./dungeons/misc/roothavok07.nif: NiSkinData
    //./dungeons/misc/ropeskull01.nif: NiSkinData
    //./oblivion/clutter/containers/clawstandcontainer.nif: NiSkinData
    //./oblivion/clutter/traps/oblivionclawtrap01.nif: NiSkinData
    //./oblivion/gate/oblivionarchgate01.nif: NiSkinData
    //./oblivion/gate/obliviongate_forming.nif: NiSkinData
    //./oblivion/gate/obliviongate_simple.nif: NiSkinData
    //
    // Skin seems to be present in things like headhuman, hand, boots, cuirass, gauntlets, greaves,
    // which are used for animation of characters with skeletons
    //
    // If there is a skin, a skeleton is needed (skeleton is at the NIF level, but skin is at a
    // sub mesh level)
    if (mSkinInstanceRef != -1)
        //&& mData.hasSkeleton()) // FIXME: Vvardenfellarmormod\cephalopod\helmet.nif
    {
        // ManualResourceLoaders for Mesh and Skeleton needs to be able to reload as necessary.
        // That means haveing to retrieve an NiModel from a cache and creating the resource
        // from it.  In other words NiModel needs all the info stored before being cached.
        //
        // In case of a Skeleton, a list of bones in the NiModel is required.
        // In case of a Mesh, a list of NiGeometry for that NiNode is required.

        // FIXME: move this to NiModel?
        mesh->setSkeletonName(mModel.getSkeleton()->getName());

        // build skeleton on demand; need to check for each mSkinInstanceRef
        //
        // start at Skeleton Root and build a tree? but that can result
        // in way too many bones (e.g. oblivion/gate/oblivionarchgate01.nif)
        //
        // best to reverse search from each of the affected bones

        // bone assignments
        //Ogre::SkeletonPtr skeleton = mData.mSkeleton;

        const NiSkinInstance *skinInstance = mModel.getRef<NiSkinInstance>(mSkinInstanceRef);
        const NiSkinData *skinData = mModel.getRef<NiSkinData>(skinInstance->mDataRef);

        for(size_t i = 0; i < skinInstance->mBoneRefs.size(); ++i)
        {
            Ogre::VertexBoneAssignment boneInf;
            std::string nodeName = mModel.getRef<NiNode>(skinInstance->mBoneRefs[i])->getName();

#if 0
            if (nodeName == "Bip01 L Finger0" || nodeName == "Bip01 R Finger0" ||
                nodeName == "Bip01 L Finger01" || nodeName == "Bip01 R Finger01" ||
                nodeName == "Bip01 L Finger1" || nodeName == "Bip01 R Finger1" ||
                nodeName == "Bip01 L Finger11" || nodeName == "Bip01 R Finger11" ||
                nodeName == "Bip01 L Finger2" || nodeName == "Bip01 R Finger2" ||
                nodeName == "Bip01 L Finger21" || nodeName == "Bip01 R Finger21" ||
                nodeName == "Bip01 L Finger31" || nodeName == "Bip01 R Finger31" ||
                nodeName == "Bip01 L Finger41" || nodeName == "Bip01 R Finger41")
                continue;
            if (nodeName == "Bip01 L ForeTwist" || nodeName == "Bip01 R ForeTwist")
                continue;
            if (nodeName == "Bip01 L Sholder" || nodeName == "Bip01 R Sholder")
                continue;
#endif
            if (nodeName == "NPC Spine2 [Spn2]")
                continue;

            boneInf.boneIndex = mModel.getSkeleton()->getBone(/*"#"+std::to_string(skinInstance->mBoneRefs[i])+"%"+*/nodeName)->getHandle();

            const std::vector<NiSkinData::SkinData::SkinWeight> &weights = skinData->mBoneList[i].vertexWeights;
            for(size_t j = 0; j < weights.size(); ++j)
            {
                boneInf.vertexIndex = weights[j].vertex;
                boneInf.weight = weights[j].weight;
                sub->addBoneAssignment(boneInf);
            }
        }
    } // mSkinInstanceRef != -1
    else if (mSkinInstanceRef == -1 && hasSkinnedSubMesh) // Dungeons\Misc\NecroTapestrySkinned01.NIF
    {
        std::string boneName = mParent->getName();

        Ogre::SkeletonPtr skeleton = mModel.getSkeleton();
        if (!skeleton)
            throw std::runtime_error("NiTriBasedGeom: no skeleton in a skinned mesh"); // shouldn't happen

        // RopeSkull01.NIF doesn't want an existing bone to be used like this?
        // And NecroTapestrySkinned01.NIF doesn't like a dummy name, either.
        // FIXME: Don't understand why though
        if (!skeleton->hasBone(boneName))
        {
            skeleton->createBone(boneName); // create a dummy bone

            Ogre::VertexBoneAssignment boneInf;
            boneInf.boneIndex = skeleton->getBone(boneName)->getHandle();

            for (std::size_t j = 0; j < vertices.size(); ++j)
            {
                boneInf.vertexIndex = j;
                boneInf.weight = 1.f;
                sub->addBoneAssignment(boneInf);
            }
        }
    }
    else if (mModel.hasSkeleton()
        && mModel.getSkeleton()->getName() == mModel.getName() // hack to avoid body parts
        && mModel.getSkeleton()->hasBone(mParent->getName())
        //&& (mModel.nifVer() < 0x14020007 || mParent.getName() == "HeadAnims") // not FO3 onwards

        //&& mModel.getName().find("geardoor") == std::string::npos
        )
    {
        // FIXME: for some reason this block is needed for npc animations to work but it
        // shouldn't

        // Definitely required for Storm Atronachs but not sure why // FIXME:

        // for node animation

        // FIXME: the issue seems to be that the child nodes are not being moved
        // (more bones were added but that doesn't seem to have helped)
        //
        // Architecture\Anvil\BenirusDoor01.NIF (0001D375)
        // COC "AnvilBenirusManorBasement"
        //
        //if (mParent.getName() != "gear 13")
        {


        // FIXME: I have no idea why "HeadAnims" is needed, but without it the NPCs don't show up
        //std::cout << "mystery " << mesh->getName() << " " << mModel.getName() << " " << mParent->getName() << std::endl;




        mesh->setSkeletonName(mModel.getName()); // FIXME: not the best place from a SubMesh?

        Ogre::VertexBoneAssignment boneInf;
        boneInf.boneIndex = mModel.getSkeleton()->getBone(/*"#"+std::to_string(mParent.selfRef())+"%"+*/mParent->getName())->getHandle();

        for (unsigned int j = 0; j < vertices.size(); ++j)
        {
            boneInf.vertexIndex = j;
            boneInf.weight = 1.f; // FIXME: hope this is correct
            sub->addBoneAssignment(boneInf);
        }





        }
    }

    // find and apply the material
    std::string materialName = getMaterial(); // NOTE: materialName may be different to subMesh name
    if(materialName.length() > 0)
        sub->setMaterialName(materialName);
    else
        throw std::runtime_error("NiTriBasedGeom: subMesh has no material");

    // End of code copied from components/nifogre/mesh.cpp

    // NOTE: NiUVController needs to be associated with a sub-entity, so the build/setup is
    //       compelted at a later point.  In comparison, NiGeomMorpherController adds animation
    //       to the sub-mesh so no further setup is required.
    NiTimeControllerRef controllerRef = NiObjectNET::mControllerRef;
    while (controllerRef != -1)
    {
        controllerRef = mModel.getRef<NiTimeController>(controllerRef)->build(mesh);
    }

    // if required build tangents at the mesh level
    return mOgreMaterial.needTangents();
}

void NiBtOgre::NiTriBasedGeom::buildFgPoses(Ogre::Mesh *mesh, const FgLib::FgTri *tri, bool rotate)
{
    const std::vector<Ogre::Vector3>& vertices = getVertices(true/*morphedIfAvailable*/);
    float endTime = 0.1f;
    unsigned short poseIndex = (unsigned short)mesh->getPoseCount(); // FIXME: is there a better way?

    const std::vector<std::string>& diffMorphs = tri->diffMorphs();
    for (std::size_t i = 0; i < diffMorphs.size(); ++i)
    {
        // need an animation for each emotion, and make the length configurable?
        Ogre::Animation *animation = mesh->createAnimation(diffMorphs[i], endTime/*totalAnimLength*/);
        Ogre::VertexAnimationTrack* track
            = animation->createVertexTrack((unsigned short)mSubMeshIndex+1, Ogre::VAT_POSE);

        // ------------------------- Base ------------------------
        Ogre::Pose* pose = mesh->createPose(Ogre::ushort(mSubMeshIndex + 1), "Base");
        for (std::size_t v = 0; v < vertices.size(); ++v) // vertices in headhuman.nif, etc
            pose->addVertex(v, Ogre::Vector3::ZERO);

        // see the comments on animation length above
        Ogre::VertexPoseKeyFrame* keyframe = track->createVertexPoseKeyFrame(0.f/*startTime*/);
        // influence value may require some experiments - maybe for lip sync go up to 1 but for others less?
        keyframe->addPoseReference(Ogre::ushort(poseIndex + 2*i + 0), 0.f/*influence*/);

        keyframe = track->createVertexPoseKeyFrame(endTime); // WARN: keyframe reused
        keyframe->addPoseReference(Ogre::ushort(poseIndex + 2*i + 1), 1.f/*influence*/);

        // --------------- pose, e.g. "Happy" --------------------
        const std::pair<float, std::vector<std::int16_t> >& diffMorphVertices
            = tri->diffMorphVertices(diffMorphs[i]);

        if (vertices.size()*3 != diffMorphVertices.second.size())
            throw std::runtime_error("NiTriBasedGeom: number of vertices in a pose differ to SubMesh");

        pose = mesh->createPose(Ogre::ushort(mSubMeshIndex + 1), diffMorphs[i]); // WARN: pose reused
        for (std::size_t v = 0; v < vertices.size(); ++v) // vertices in headhuman.nif, etc
        {
            float scale = diffMorphVertices.first;
            Ogre::Vector3 delta;

            if (rotate)
            {
                // x and z swapped
                delta = Ogre::Vector3(scale * diffMorphVertices.second[v * 3 + 2],
                                      scale * diffMorphVertices.second[v * 3 + 1],
                                      scale * diffMorphVertices.second[v * 3 + 0]);
            }
            else
            {
                delta = Ogre::Vector3(scale * diffMorphVertices.second[v * 3 + 0],
                                      scale * diffMorphVertices.second[v * 3 + 1],
                                      scale * diffMorphVertices.second[v * 3 + 2]);
            }

            pose->addVertex(v, delta);
        }

        keyframe = track->createVertexPoseKeyFrame(0.f/*startTime*/); // WARN: keyframe reused
        keyframe->addPoseReference(Ogre::ushort(poseIndex + 2*i + 2), 0.f/*influence*/);

        keyframe = track->createVertexPoseKeyFrame(endTime); // WARN: keyframe reused
        keyframe->addPoseReference(Ogre::ushort(poseIndex + 2*i + 3), 1.f/*influence*/);
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLODTriShape::BSLODTriShape(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTriBasedGeom(index, stream, model, data)
{
    stream->read(mLevel0Size);
    stream->read(mLevel1Size);
    stream->read(mLevel2Size);
}
