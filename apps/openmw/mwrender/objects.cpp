#include "objects.hpp"

#include <cmath>
#include <iostream> // FIXME: testing only

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreNode.h>
#include <OgreLight.h>
#include <OgreSubEntity.h>
#include <OgreParticleSystem.h>
#include <OgreParticleEmitter.h>
#include <OgreStaticGeometry.h>
#include <OgreTechnique.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreTextureManager.h>

#include <extern/esm4/formid.hpp> // mainly for debugging
#include <extern/nibtogre/btogreinst.hpp>
#include <extern/nibtogre/nimodelmanager.hpp>

#include <components/esm/loadligh.hpp>
#include <components/esm/loadstat.hpp>

//#include <components/nifogre/ogrenifloader.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "renderconst.hpp"
#include "animation.hpp"
#include "foreignactivatoranimation.hpp"

using namespace MWRender;

int Objects::uniqueID = 0;

Objects::~Objects()
{
    // delete scene from mLandscapeScene
    //std::map<ESM4::FormId, std::map<std::pair<int, int>, NifOgre::ObjectScenePtr> >::iterator
    //    worldIter = mLandscapeScene.begin();

    //for (;worldIter != mLandscapeScene.end(); ++worldIter)
    //{
    //    std::map<std::pair<int, int>, NifOgre::ObjectScenePtr>::iterator sceneIter
    //        = worldIter->second.begin();
    //    for (; sceneIter != worldIter->second.end(); ++sceneIter)
    //    {
    //        // FIXME: need to delete shared pointer? (NifOgre::ObjectScenePtr)
    //        worldIter->second.erase(sceneIter);
    //    }
    //}
}

void Objects::setRootNode(Ogre::SceneNode* root)
{
    mRootNode = root;
}

void Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    Ogre::SceneNode* root = mRootNode;
    Ogre::SceneNode* cellnode;
    if(mCellSceneNodes.find(ptr.getCell()) == mCellSceneNodes.end())
    {
        //Create the scenenode and put it in the map
        cellnode = root->createChildSceneNode();
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
    {
        cellnode = mCellSceneNodes[ptr.getCell()];
    }

    Ogre::SceneNode* insert = cellnode->createChildSceneNode();
    const float *f = ptr.getRefData().getPosition().pos;

    insert->setPosition(f[0], f[1], f[2]);
    insert->setScale(ptr.getCellRef().getScale(), ptr.getCellRef().getScale(), ptr.getCellRef().getScale());


    // Convert MW rotation to a quaternion:
    f = ptr.getCellRef().getPosition().rot;

    // Rotate around X axis
    Ogre::Quaternion xr(Ogre::Radian(-f[0]), Ogre::Vector3::UNIT_X);

    // Rotate around Y axis
    Ogre::Quaternion yr(Ogre::Radian(-f[1]), Ogre::Vector3::UNIT_Y);

    // Rotate around Z axis
    Ogre::Quaternion zr(Ogre::Radian(-f[2]), Ogre::Vector3::UNIT_Z);

    // Rotates first around z, then y, then x
    insert->setOrientation(xr*yr*zr);

    ptr.getRefData().setBaseNode(insert);

    //if (ptr.getTypeName() == typeid(ESM4::Door).name()) // FIXME: debugging only
        //std::cout << "got basenode " << ptr.getBase()->mRef.getRefId() << std::endl;
}

void Objects::insertLight(const MWWorld::Ptr &ptr)
{
    insertBegin(ptr);

    std::auto_ptr<ObjectAnimation> anim = std::auto_ptr<ObjectAnimation>(new ObjectAnimation(ptr, ""));

    if(anim.get() != NULL)
    {
        anim->addLight();
        mObjects.insert(std::make_pair(ptr, anim.release()));
    }
}

const std::map<std::int32_t, Ogre::SceneNode*> *Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool batch)
{
    insertBegin(ptr);
    //if (mesh.find("TorchTall01") != std::string::npos)
        //std::cout << "stop" << std::endl;

    std::auto_ptr<ObjectAnimation> anim;
    if(ptr.getTypeName() == typeid(ESM4::Activator).name())
        anim = std::auto_ptr<ObjectAnimation>(new ForeignActivatorAnimation(ptr, mesh));
    else if(ptr.getTypeName() == typeid(ESM4::Door).name())
        anim = std::auto_ptr<ObjectAnimation>(new ForeignActivatorAnimation(ptr, mesh));
    else
        anim = std::auto_ptr<ObjectAnimation>(new ObjectAnimation(ptr, mesh));

    if(ptr.getTypeName() == typeid(ESM4::Light).name())
        anim->addLight();

    if (!mesh.empty())
    {
        Ogre::AxisAlignedBox bounds = anim->getWorldBounds();
        Ogre::Vector3 extents = bounds.getSize();
        extents *= ptr.getRefData().getBaseNode()->getScale();
        float size = std::max(std::max(extents.x, extents.y), extents.z);

        bool small = (size < Settings::Manager::getInt("small object size", "Viewing distance")) &&
                     Settings::Manager::getBool("limit small object distance", "Viewing distance");
        // do not fade out doors. that will cause holes and look stupid
        if(ptr.getTypeName().find("Door") != std::string::npos)
            small = false;

        if (mBounds.find(ptr.getCell()) == mBounds.end())
            mBounds[ptr.getCell()] = Ogre::AxisAlignedBox::BOX_NULL;
        mBounds[ptr.getCell()].merge(bounds);

        if(batch &&
           Settings::Manager::getBool("use static geometry", "Objects") &&
           anim->canBatch())
        {
            Ogre::StaticGeometry* sg = 0;

            if (small)
            {
                if(mStaticGeometrySmall.find(ptr.getCell()) == mStaticGeometrySmall.end())
                {
                    uniqueID = uniqueID+1;
                    sg = mRenderer.getScene()->createStaticGeometry("sg" + Ogre::StringConverter::toString(uniqueID));
                    sg->setOrigin(ptr.getRefData().getBaseNode()->getPosition());
                    mStaticGeometrySmall[ptr.getCell()] = sg;

                    sg->setRenderingDistance(static_cast<Ogre::Real>(Settings::Manager::getInt("small object distance", "Viewing distance")));
                }
                else
                    sg = mStaticGeometrySmall[ptr.getCell()];
            }
            else
            {
                if(mStaticGeometry.find(ptr.getCell()) == mStaticGeometry.end())
                {
                    uniqueID = uniqueID+1;
                    sg = mRenderer.getScene()->createStaticGeometry("sg" + Ogre::StringConverter::toString(uniqueID));
                    sg->setOrigin(ptr.getRefData().getBaseNode()->getPosition());
                    mStaticGeometry[ptr.getCell()] = sg;
                }
                else
                    sg = mStaticGeometry[ptr.getCell()];
            }

            // This specifies the size of a single batch region.
            // If it is set too high:
            //  - there will be problems choosing the correct lights
            //  - the culling will be more inefficient
            // If it is set too low:
            //  - there will be too many batches.
            if(ptr.getCell()->isExterior())
                sg->setRegionDimensions(Ogre::Vector3(2048,2048,2048));
            else
                sg->setRegionDimensions(Ogre::Vector3(1024,1024,1024));

            sg->setVisibilityFlags(small ? RV_StaticsSmall : RV_Statics);

            sg->setCastShadows(true);

            sg->setRenderQueueGroup(RQG_Main);

            anim->fillBatch(sg);
            /* TODO: We could hold on to this and just detach it from the scene graph, so if the Ptr
             * ever needs to modify we can reattach it and rebuild the StaticGeometry object without
             * it. Would require associating the Ptr with the StaticGeometry. */
            anim.reset();
        }
    }

    if(anim.get() != NULL)
    {
        const std::map<std::int32_t, Ogre::SceneNode*> *res = &anim->getPhysicsNodeMap();
        mObjects.insert(std::make_pair(ptr, anim.release()));
        return res;
    }
    else
        return nullptr;
}

void Objects::insertLandscapeModel(ESM4::FormId worldId, int x, int y, const std::string& mesh)
{
    Ogre::SceneNode *root = mRootNode;
    Ogre::SceneNode *cellnode = root->createChildSceneNode(); // child for cell level

    Ogre::SceneNode *insert = cellnode->createChildSceneNode(); // grandchild for object level

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    NiModelPtr landscape = modelManager.getOrLoadByName(mesh, "General");

    NifOgre::ObjectScenePtr scene
        = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(insert->getCreator()));

#if defined(__GNUC__) && __GNUC__ < 8
    scene->mForeignObj
        = std::unique_ptr<NiBtOgre::BtOgreInst>(new NiBtOgre::BtOgreInst(landscape, insert->createChildSceneNode()));
#else
    scene->mForeignObj
        = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(landscape, insert->createChildSceneNode()));
#endif
    scene->mForeignObj->instantiate();

    std::map<int32_t, Ogre::Entity*>::iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        // FIXME: have no idea why this is needed
        Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(it->second);

        uniqueID = uniqueID+1;
        Ogre::StaticGeometry *sg = mRenderer.getScene()->createStaticGeometry("sg" + Ogre::StringConverter::toString(uniqueID));
        sg->setOrigin(Ogre::Vector3::ZERO);

        std::map<ESM4::FormId, std::map<std::pair<int, int>, Ogre::StaticGeometry*> >::iterator
            sgIter = mStaticGeometryLandscape.find(worldId);
        if (sgIter != mStaticGeometryLandscape.end())
        {
            std::map<std::pair<int, int>, Ogre::StaticGeometry*>& lodMap
                = mStaticGeometryLandscape[worldId];

            lodMap[std::pair<int,int>(x,y)] = sg;
        }
        else
        {
            std::map<std::pair<int, int>, Ogre::StaticGeometry*> lodMap;
            lodMap[std::pair<int,int>(x,y)] = sg;

            mStaticGeometryLandscape[worldId] = lodMap;
        }

        //std::cout << "adding landscape model " << ESM4::formIdToString(worldId) << " at " // FIXME
                  //<< x << "," << y << std::endl;

        // TODO: test with different values
        sg->setRegionDimensions(Ogre::Vector3(2048,2048,2048));
        sg->setVisibilityFlags(RV_Statics);
        sg->setCastShadows(true);
        sg->setRenderQueueGroup(RQG_Main);

        Ogre::Node *node = (it->second)->getParentNode();
        sg->addEntity(it->second, Ogre::Vector3::ZERO);
        sg->build();
    }

    std::map<ESM4::FormId, std::map<std::pair<int, int>, NifOgre::ObjectScenePtr> >::iterator
        sceneIter = mLandscapeScene.find(worldId);
    if (sceneIter != mLandscapeScene.end())
    {
        std::map<std::pair<int, int>, NifOgre::ObjectScenePtr>& sceneMap
            = mLandscapeScene[worldId];

        sceneMap[std::pair<int, int>(x, y)] = scene;
    }
    else
    {
        std::map<std::pair<int, int>, NifOgre::ObjectScenePtr> sceneMap;
        sceneMap[std::pair<int,int>(x,y)] = scene;

        mLandscapeScene[worldId] = sceneMap;
    }
}

// FIXME: FO3/FONV support
// FIXME: there is a strange bug where after the first time the landscape LOD are added the
// "unhiding" seems to done to the wrong cell(s).  But once the "faulty" cell is unloaded then
// loaded again, the issue no longer occurs.
void Objects::updateLandscapeTexture(ESM4::FormId worldId, int x, int y, bool hide)
{
    std::map<ESM4::FormId, std::map<std::pair<int, int>, NifOgre::ObjectScenePtr> >::iterator
        iter = mLandscapeScene.find(worldId);

    if (iter != mLandscapeScene.end())
    {
        // convert x, y to landscape co-ordinates
        int X = int((x >= 0 ? x /*+ 32*/ : x - 32) / 32) * 32; // FIXME: check FO3/FONV (4x4 blocks?)
        int Y = int((y >= 0 ? y /*+ 32*/ : y - 32) / 32) * 32;

        int newX = std::abs(x - X);
        int newY = std::abs(y - Y);

        std::map<std::pair<int, int>, NifOgre::ObjectScenePtr>::iterator sceneIter
            = iter->second.find(std::pair<int, int>(X, Y));
        if (sceneIter != iter->second.end())
        {
            //std::cout << "updating landscape model " << ESM4::formIdToString(worldId) << " at "
                      //<< X << "," << Y << " (" << newX << "," << newY << ")" << std::endl; // FIXME

            NifOgre::ObjectScenePtr& scene = sceneIter->second;

            std::map<int32_t, Ogre::Entity*>::iterator it(scene->mForeignObj->mEntities.begin());
            for (; it != scene->mForeignObj->mEntities.end(); ++it)
            {
                Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(it->second);
                Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
                while(techIter.hasMoreElements())
                {
                    Ogre::Technique *tech = techIter.getNext();
                    Ogre::Technique::PassIterator passes = tech->getPassIterator();
                    while(passes.hasMoreElements())
                    {
                        Ogre::Pass *pass = passes.getNext();

                        // actually means alpha value less than 192 will be rejected (confusing)
                        pass->setAlphaRejectSettings(Ogre::CMPF_GREATER_EQUAL, 0xC0/*192*/, true);

                        // e.g. TES4 textures\landscapelod\generated\60.00.00.32.dds
                        // FIXME: how to support FO3/FONV file names?
                        //        get it from mForeignObj->mModel?
                        //        get it from ESM4::Cell as a parameter to this method?
                        std::string modelName = sceneIter->second->mForeignObj->mModel->getName();
                        std::size_t pos = modelName.find_last_of("\\");
                        std::size_t pos2 = modelName.find_last_of(".");
                        std::string texName
                            = "textures\\landscapelod\\generated" + modelName.substr(pos, pos2-pos) + ".dds";

                        // base texture
                        Ogre::TextureUnitState *tex = pass->getTextureUnitState(0);

                        // FIXME: no idea what is adding another technique;
                        //        just workaround for now by avoiding it
                        if (mat->getNumTechniques() != 2 && tex->getName() != "hide")
                            continue; // FIXME: temp testing

                        std::string textureName = tex->getTextureName();
                        Ogre::TexturePtr texAlpha = Ogre::TextureManager::getSingleton().getByName(
                               textureName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                        texAlpha->load();

                        //std::cout << (hide ? "hiding " : "unhiding ")
                            //<< x << "," << y << " " << texName << std::endl; // FIXME

                        // try to retrieve previously created texture
                        Ogre::TexturePtr alphaTexture = Ogre::TextureManager::getSingleton().getByName(
                               "alpha_" + texName,
                               Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                        if (!alphaTexture)
                        {
                            // create a blank one
                            alphaTexture = Ogre::TextureManager::getSingleton().createManual(
                                "alpha_" + texName, // name
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                Ogre::TEX_TYPE_2D,  // type
                                texAlpha->getWidth(), texAlpha->getHeight(), // width & height
                                1,
                                0,                  // number of mipmaps;
                                Ogre::PF_BYTE_RGBA,
                                Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                                    // textures updated very often (e.g. each frame)
                        }

                        // FIXME: one of the unlock() calls throws an exception in linux

                        // src
                        Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc
                            = Ogre::static_pointer_cast<Ogre::Texture>(texAlpha)->getBuffer();
                        //pixelBufferSrc->unlock(); // prepare for blit()

                        // dest
                        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = alphaTexture->getBuffer();
                        //pixelBuffer->unlock(); // prepare for blit()

                        // if source and destination dimensions don't match, scaling is done
                        pixelBuffer->blit(pixelBufferSrc);

                        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
                        uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

                        //std::cout << "x range " << newX << " to " << (newX+1) << std::endl; // FIXME
                        //std::cout << "y range " << newY << " to " << (newY+1) << std::endl; // FIXME

                        for (size_t i = 0; i < texAlpha->getWidth(); ++i) // y axis
                        {
                            for (size_t j = 0; j < texAlpha->getHeight(); ++j) // x axis
                            {
                                if (i >= (newY * 32) && i < (newY+1) * 32
                                        && j >= (newX * 32) && j < (newX+1) * 32)
                                {
                                    if (hide)
                                        *(pDest+3) = 5; // some value less than 192
                                    else
                                        *(pDest+3) = 193;
                                }

                                pDest += 4;
                            }
                        }
                        pixelBuffer->unlock();

                        pass->removeTextureUnitState(0);
                        Ogre::TextureUnitState *newTUS = pass->createTextureUnitState("alpha_"+texName);
                        newTUS->setName("hide"); // NOTE: keeps re-setting it :-(
                        //newTUS->setColourOperation(Ogre::LBO_ALPHA_BLEND);
                    }
                }
            }
        }
    }
}

// IDEA: put world in a FIFO and keep the last one (so that if one pops into a store to offload
// some loot and come back out we don't have to reload all the landscape for that world)
//
// FIXME: linux crash
void Objects::removeLandscapeModel(ESM4::FormId worldId, int x, int y)
{
    std::map<ESM4::FormId, std::map<std::pair<int, int>, Ogre::StaticGeometry*> >::iterator
        iter = mStaticGeometryLandscape.find(worldId);

    if (iter != mStaticGeometryLandscape.end())
    {
        //std::map<std::pair<int, int>, Ogre::StaticGeometry*>& lodMap = iter->second;
        std::map<std::pair<int, int>, Ogre::StaticGeometry*>::iterator gridIter
            = iter->second.find(std::pair<int, int>(x, y));
        if (gridIter != iter->second.end())
        {
            std::cout << "removing landscape model " << ESM4::formIdToString(worldId) << " at " // FIXME
                      << x << "," << y << std::endl;

            Ogre::StaticGeometry *sg = gridIter->second;

            iter->second.erase(gridIter);
            mRenderer.getScene()->destroyStaticGeometry(sg);
        }
    }

    // delete scene from mLandscapeScene
    std::map<ESM4::FormId, std::map<std::pair<int, int>, NifOgre::ObjectScenePtr> >::iterator
        worldIter = mLandscapeScene.find(worldId);

    if (worldIter != mLandscapeScene.end())
    {
        std::map<std::pair<int, int>, NifOgre::ObjectScenePtr>::iterator sceneIter
            = worldIter->second.find(std::pair<int, int>(x, y));
        if (sceneIter != worldIter->second.end())
        {
            worldIter->second.erase(sceneIter);
        }
    }
}

// FIXME: this code ia almost the same as above
void Objects::removeLandscapeModel(ESM4::FormId worldId)
{
    std::map<ESM4::FormId, std::map<std::pair<int, int>, Ogre::StaticGeometry*> >::iterator
        iter = mStaticGeometryLandscape.find(worldId);

    if (iter != mStaticGeometryLandscape.end())
    {
        //std::map<std::pair<int, int>, Ogre::StaticGeometry*>& lodMap = iter->second;
        std::map<std::pair<int, int>, Ogre::StaticGeometry*>::iterator gridIter
            = iter->second.begin();
        for (; gridIter != iter->second.end(); ++gridIter)
        {
            int x = gridIter->first.first;
            int y = gridIter->first.second;

            std::cout << "removing landscape model " << ESM4::formIdToString(worldId) << " at "
                      << x << "," << y << std::endl;

            Ogre::StaticGeometry *sg = gridIter->second;
            if (sg)
                mRenderer.getScene()->destroyStaticGeometry(sg);

            iter->second.erase(gridIter);
        }
    }

    // delete scene from mLandscapeScene
    std::map<ESM4::FormId, std::map<std::pair<int, int>, NifOgre::ObjectScenePtr> >::iterator
        worldIter = mLandscapeScene.find(worldId);

    if (worldIter != mLandscapeScene.end())
    {
        std::map<std::pair<int, int>, NifOgre::ObjectScenePtr>::iterator sceneIter
            = worldIter->second.begin();
        for (; sceneIter != worldIter->second.end(); ++sceneIter)
        {
            worldIter->second.erase(sceneIter);
        }
    }
}

bool Objects::deleteObject (const MWWorld::Ptr& ptr)
{
    if(!ptr.getRefData().getBaseNode())
        return true;

    PtrAnimationMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        delete iter->second;
        mObjects.erase(iter);

        mRenderer.getScene()->destroySceneNode(ptr.getRefData().getBaseNode());
        ptr.getRefData().setBaseNode(0);
        return true;
    }

    return false;
}

// FIXME: can't remove landscape LOD each time a cell is removed, but then when do we remove it?
//        maybe when world changes?  how about when someone enters an interior cell then comes
//        back out?
void Objects::removeCell(MWWorld::CellStore* store)
{
    for(PtrAnimationMap::iterator iter = mObjects.begin();iter != mObjects.end();)
    {
        if(iter->first.getCell() == store)
        {
            delete iter->second;
            mObjects.erase(iter++);
        }
        else
            ++iter;
    }

    std::map<MWWorld::CellStore*,Ogre::StaticGeometry*>::iterator geom = mStaticGeometry.find(store);
    if(geom != mStaticGeometry.end())
    {
        Ogre::StaticGeometry *sg = geom->second;
        mStaticGeometry.erase(geom);
        mRenderer.getScene()->destroyStaticGeometry(sg);
    }

    geom = mStaticGeometrySmall.find(store);
    if(geom != mStaticGeometrySmall.end())
    {
        Ogre::StaticGeometry *sg = geom->second;
        mStaticGeometrySmall.erase(store);
        mRenderer.getScene()->destroyStaticGeometry(sg);
    }

    mBounds.erase(store);

    std::map<MWWorld::CellStore*,Ogre::SceneNode*>::iterator cell = mCellSceneNodes.find(store);
    if(cell != mCellSceneNodes.end())
    {
        cell->second->removeAndDestroyAllChildren();
        mRenderer.getScene()->destroySceneNode(cell->second);
        mCellSceneNodes.erase(cell);
    }
}

void Objects::buildStaticGeometry(MWWorld::CellStore& cell)
{
    if(mStaticGeometry.find(&cell) != mStaticGeometry.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometry[&cell];
        sg->build();
    }
    if(mStaticGeometrySmall.find(&cell) != mStaticGeometrySmall.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometrySmall[&cell];
        sg->build();
    }
}

Ogre::AxisAlignedBox Objects::getDimensions(MWWorld::CellStore* cell)
{
    return mBounds[cell];
}

void Objects::update(float dt, Ogre::Camera* camera)
{
    PtrAnimationMap::const_iterator it = mObjects.begin();
    for(;it != mObjects.end();++it)
        it->second->runAnimation(dt);

    it = mObjects.begin();
    for(;it != mObjects.end();++it)
        it->second->preRender(camera);

}

void Objects::rebuildStaticGeometry()
{
    for (std::map<MWWorld::CellStore *, Ogre::StaticGeometry*>::iterator it = mStaticGeometry.begin(); it != mStaticGeometry.end(); ++it)
    {
        it->second->destroy();
        it->second->build();
    }

    for (std::map<MWWorld::CellStore *, Ogre::StaticGeometry*>::iterator it = mStaticGeometrySmall.begin(); it != mStaticGeometrySmall.end(); ++it)
    {
        it->second->destroy();
        it->second->build();
    }
}

void Objects::updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    Ogre::SceneNode *node;
    MWWorld::CellStore *newCell = cur.getCell();

    if(mCellSceneNodes.find(newCell) == mCellSceneNodes.end()) {
        node = mRootNode->createChildSceneNode();
        mCellSceneNodes[newCell] = node;
    } else {
        node = mCellSceneNodes[newCell];
    }

    node->addChild(cur.getRefData().getBaseNode());

    PtrAnimationMap::iterator iter = mObjects.find(old);
    if(iter != mObjects.end())
    {
        ObjectAnimation *anim = iter->second;
        mObjects.erase(iter);
        anim->updatePtr(cur);
        mObjects[cur] = anim;
    }
}

ObjectAnimation* Objects::getAnimation(const MWWorld::Ptr &ptr)
{
    PtrAnimationMap::const_iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        return iter->second;
    return NULL;
}

