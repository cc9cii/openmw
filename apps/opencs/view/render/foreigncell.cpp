#include "foreigncell.hpp"

#include <iostream>

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreEntity.h> // FIXME temp
#include <OgreMesh.h> // FIXME temp
#include <OgreSkeleton.h> // FIXME temp
#include <OgreSkeletonInstance.h> // FIXME temp
#include <OgreBone.h> // FIXME temp

#include <components/misc/stringops.hpp>
#include <components/esm/loadland.hpp>
//#include <components/nifogre/objectscene.hpp> // FIXME temp

#include "../../model/doc/document.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../../model/world/pathgrid.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/pathgridcommands.hpp"
#include "../../model/world/pathgridpointswrap.hpp"
#include "../../model/world/nestedtableproxymodel.hpp"

#include "../../model/foreign/cellgroupcollection.hpp"

#include "../world/physicssystem.hpp"

#include "../foreign/terrainstorage.hpp"

#include "elements.hpp"
#include "pathgridpoint.hpp"

namespace CSVRender
{
    // PLEASE NOTE: pathgrid edge code copied and adapted from mwrender/debugging
    static const std::string PG_LINE_MATERIAL = "pathgridLineMaterial";
    static const int POINT_MESH_BASE = 35;
    static const std::string DEBUGGING_GROUP = "debugging";
}

void CSVRender::ForeignCell::createGridMaterials()
{
    if(!Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
        Ogre::ResourceGroupManager::getSingleton().createResourceGroup(DEBUGGING_GROUP);

    if(!Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP))
    {
        Ogre::MaterialPtr lineMatPtr =
            Ogre::MaterialManager::getSingleton().create(PG_LINE_MATERIAL, DEBUGGING_GROUP);
        lineMatPtr->setReceiveShadows(false);
        lineMatPtr->getTechnique(0)->setLightingEnabled(true);
        lineMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,1,0,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,1,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,1,0);
    }
}

void CSVRender::ForeignCell::destroyGridMaterials()
{
    if(Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
    {
        if(Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP))
            Ogre::MaterialManager::getSingleton().remove(PG_LINE_MATERIAL, DEBUGGING_GROUP);

        Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(DEBUGGING_GROUP);
    }
}

Ogre::ManualObject *CSVRender::ForeignCell::createPathgridEdge(const std::string &name,
        const Ogre::Vector3 &start, const Ogre::Vector3 &end)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject(name);

    result->begin(PG_LINE_MATERIAL, Ogre::RenderOperation::OT_LINE_LIST);

    Ogre::Vector3 direction = (end - start);
    Ogre::Vector3 lineDisplacement = direction.crossProduct(Ogre::Vector3::UNIT_Z).normalisedCopy();
    // move lines up a little, so they will be less covered by meshes/landscape
    lineDisplacement = lineDisplacement * POINT_MESH_BASE + Ogre::Vector3(0, 0, 10);
    result->position(start + lineDisplacement);
    result->position(end + lineDisplacement);

    result->end();

    return result;
}

bool CSVRender::ForeignCell::removeObject (ESM4::FormId formId)
{
    std::map<ESM4::FormId, ForeignObject *>::iterator iter =
        mObjects.find (formId);

    if (iter == mObjects.end())
        return false;

    delete iter->second;
    mObjects.erase (iter);
    return true;
}

bool CSVRender::ForeignCell::addObjects (const std::vector<ESM4::FormId>& objects)
{
    bool modified = false;

    const CSMForeign::CellRefCollection<CSMForeign::CellRef>& refs = mDocument.getData().getForeignReferences();

    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        int index = refs.searchFormId(objects[i]);
        if (index == -1)
            continue; // FIXME: maybe not a ref but something else?

        const CSMWorld::Record<CSMForeign::CellRef>& record = refs.getRecord(index);

        if (record.mState != CSMWorld::RecordBase::State_Deleted)
        {
            ESM4::FormId id = objects[i];
            mObjects.insert(std::make_pair(id,
                        new ForeignObject(mDocument.getData(), mCellNode, id, false, mPhysics)));
            modified = true;

            // FIXME: temporarily add body parts here
            //
            // default NPC parts(?) for TES4
            //   femalefoot.nif       |   foot.nif
            //   femalehand.nif       |   hand.nif
            //   femalelowerbody.nif  |   lowerbody.nif
            //   femaleupperbody.nif  |   upperbody.nif
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Npc> >& npc
                                                 = mDocument.getData().getForeignNpcs();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Creature> >& crea
                                                 = mDocument.getData().getForeignCreatures();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LevelledCreature> >& lvlc
                                                 = mDocument.getData().getForeignLvlCreatures();
            //
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Armor> >& armor
                                                 = mDocument.getData().getForeignArmors();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Weapon> >& weap
                                                 = mDocument.getData().getForeignWeapons();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ammunition> >& ammo
                                                 = mDocument.getData().getForeignAmmos();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Clothing> >& cloth
                                                 = mDocument.getData().getForeignClothings();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Potion> >& potion
                                                 = mDocument.getData().getForeignPotions();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Apparatus> >& appa
                                                 = mDocument.getData().getForeignApparatuses();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ingredient> >& ingr
                                                 = mDocument.getData().getForeignIngredients();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SigilStone> >& sigil
                                                 = mDocument.getData().getForeignSigilStones();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SoulGem> >& soul
                                                 = mDocument.getData().getForeignSoulGems();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Key> >& keys
                                                 = mDocument.getData().getForeignKeys();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Hair> >& hair
                                                 = mDocument.getData().getForeignHairs();
            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Eyes> >& eyes
                                                 = mDocument.getData().getForeignEyesSet();

            // For TES3, MWRender::NpcAnimation::updateNpcBase()
            //
            //   - decides on head and hair bodypart model mesh (nif file) based on
            //     ESM::NPC::mHead and ESM::NPC::mHair, beast, vampire, etc
            //     --> TES4 needs to work with hair/eye formid's as well as facegen
            //
            //   - calls MWRender::Animation::setObjectRoot() with the (skin?) model nif as
            //     'base only' (e.g. base_anim.nif)
            //     --> TES4 uses skeleton.nif which does not have any skin...
            //
            //   - calls MWRender::Animation::addAnimSource() e.g. xbase_anim_female.nif
            //     note addAnimSource then adds xbase_anim_female.kf, etc
            //     --> TES4 specifies the kf files differently, possibly hard coded base ones
            //         plus additional ones through KFFZ subrecords
            //
            //   - removes existing parts by calling NpcAnimation::removeIndividualPart() for
            //     each ESM::PartReferenceType (i.e. 27 of them, see loadarmo.hpp)
            //
            //   - adds new parts by calling NpcAnimation::updateParts()
            //
            // For TES3, MWRender::NpcAnimation::updateParts()
            //
            //   - adjusts for gender race, etc before calling NpcAnimation::addOrReplaceIndividualPart()
            //     for each part (i.e. 27)
            //
            // For TES3, MWRender::NpcAnimation::addOrReplaceIndividualPart()
            //
            // For TES3, MWRender::NpcAnimation::insertBoundedPart()
            //
            //   - calls NifOgre::Loader::createObjects()
            //
            // It seems like MWRender::Animation::updateSkeletonInstance() updates the skinned
            // body parts' bones (e.g. robe)
            //
            // See MWRender::NpcAnimation::runAnimation(float timepassed) and
            // MWRender::CreatureWeaponAnimation::updatePart()
            ESM4::FormId baseObj = refs.getRecord(refs.searchFormId(id)).get().mBaseObj;
            if (lvlc.searchFormId(baseObj) != -1)
            {
                const CSMForeign::IdRecord<ESM4::LevelledCreature>& lcreature
                                                  = lvlc.getRecord(ESM4::formIdToString(baseObj)).get();
                ESM4::FormId templ = 0;
                for (unsigned int j = 0; j < lcreature.mLvlObject.size(); ++j)
                {
                    templ = lcreature.mLvlObject[j].item;
                    int extraIndex = -1;
                    extraIndex = crea.searchFormId(templ);
                    if (extraIndex != -1)
                    {
                        continue; // do nothing for now
                    }
                }

                int extraIndex = -1;
                for (unsigned int j = 0; j < lcreature.mLvlObject.size(); ++j)
                {
                    templ = lcreature.mLvlObject[j].item;
                    extraIndex = npc.searchFormId(templ);
                    if (extraIndex != -1)
                    {
                        break;
                    }
                }

                if (extraIndex != -1)
                {
#if 0
                    // check what skeleton bones we have
                    Ogre::SkeletonInstance *skelinst = mObjects[id]->getObject()->mSkelBase->getSkeleton();
                    Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
                    while(boneiter.hasMoreElements())
                        std::cout << "bone name " << boneiter.getNext()->getName() << std::endl;
#endif
                    CSMForeign::IdRecord<ESM4::Npc> npcRec = npc.getRecord(extraIndex).get();

                    // head
                    NifOgre::ObjectScenePtr objectH
                        = NifOgre::Loader::createObjects(mObjects[id]->getObject()->mSkelBase,
                                                         "Bip01", // not used for skinned
                                                         /*"Bip01"*/"", // ??
                                                         mObjects[id]->getSceneNode(),
                                                         "meshes\\characters\\imperial\\headhuman.nif");
                    objectH->setVisibilityFlags (Element_Reference);
                    mObjectParts.push_back(objectH); // FIXME put every npc's parts into this for now
#if 0
                    // hair
                    Ogre::SceneNode *node = mObjects[id]->getSceneNode()->createChildSceneNode();
                    NifOgre::ObjectScenePtr objectHR
                        = NifOgre::Loader::createObjects(node,
                                                         "meshes\\characters\\hair\\bretonmaletonsure.nif");
                                                         //"meshes\\characters\\imperial\\male\\hair01.nif");
                                                         //"meshes\\characters\\Ren\\Hair\\Ren_Hair01F.nif");
                    objectHR->setVisibilityFlags (Element_Reference);
                    //node->roll(Ogre::Degree(-90));
                    mObjectParts.push_back(objectHR); // FIXME put every npc's parts into this for now
#endif
                    // hand
                    Ogre::SceneNode *nodeHand = mObjects[id]->getSceneNode()->createChildSceneNode();
                    NifOgre::ObjectScenePtr objectHand
                        = NifOgre::Loader::createObjects(nodeHand,
                                                         "meshes\\characters\\_male\\hand.nif");
                    objectHand->setVisibilityFlags (Element_Reference);
                    //nodeL->roll(Ogre::Degree(-90));
                    mObjectParts.push_back(objectHand); // FIXME put every npc's parts into this for now


                    // FIXME: the eyes are rotated 90 degrees, possibly an issue with the
                    // "Bip01 Head" bone?

                    // left eye
                    NifOgre::ObjectScenePtr objectL
                        = NifOgre::Loader::createObjects(mObjects[id]->getObject()->mSkelBase,
                                                         "Bip01 Head", // not used for skinned
                                                         /*"Bip01"*/"", // ??
                                                         mObjects[id]->getSceneNode(),
                                                         "meshes\\characters\\imperial\\eyelefthuman.nif");
                    objectL->setVisibilityFlags (Element_Reference);
                    mObjectParts.push_back(objectL); // FIXME put every npc's parts into this for now

                    // right eye
                    NifOgre::ObjectScenePtr objectR
                        = NifOgre::Loader::createObjects(mObjects[id]->getObject()->mSkelBase,
                                                         "Bip01 Head", // not used for skinned
                                                         /*"Bip01"*/"", // ??
                                                         mObjects[id]->getSceneNode(),
                                                         "meshes\\characters\\imperial\\eyerighthuman.nif");
                    objectR->setVisibilityFlags (Element_Reference);
                    mObjectParts.push_back(objectR); // FIXME put every npc's parts into this for now

//#if 0
                    // hair
                    NifOgre::ObjectScenePtr objectHR
                        = NifOgre::Loader::createObjects(mObjects[id]->getObject()->mSkelBase,
                                                         "Bip01 Head", // not used for skinned
                                                         /*"Bip01"*/"", // ??
                                                         mObjects[id]->getSceneNode(),
                                                         //"meshes\\characters\\imperial\\male\\hair01.nif");
                                                         "meshes\\characters\\hair\\bretonmaletonsure.nif");
                    objectHR->setVisibilityFlags (Element_Reference);
                    mObjectParts.push_back(objectHR); // FIXME put every npc's parts into this for now
//#endif
                    for (unsigned int j = 0; j < npcRec.mInventory.size(); ++j)
                    {
                        int invIndex = cloth.searchFormId(npcRec.mInventory[j].item);
                        if (invIndex != -1)
                        {
                            const CSMForeign::IdRecord<ESM4::Clothing>& clothRec = cloth.getRecord(invIndex).get();
                            //std::cout << clothRec.mEditorId << std::endl; // FIXME

// NifOgre::ObjectScenePtr objects = NifOgre::Loader::createObjects(mSkelBase, bonename, bonefilter, mInsert, model);
//ObjectScenePtr Loader::createObjects (Ogre::Entity *parent, const std::string &bonename,
                                      //const std::string& bonefilter,
                                      //Ogre::SceneNode *parentNode,
                                      //std::string name, const std::string &group)


                            // need Entity
                            // which bone to attach a robe?
                            // bonefilter - guess, use the same as bone
                            // SceneNode is prob. ok to use mCellNode
                            // model - ok
                            // group - use defaut
                            NifOgre::ObjectScenePtr object
                                = NifOgre::Loader::createObjects(mObjects[id]->getObject()->mSkelBase,
                                                                 "Bip01", // not used for skinned
                                                                 /*"Bip01"*/"", // ??
                                                                 mObjects[id]->getSceneNode(),
                                                                 "meshes\\"+clothRec.mModel);
                            object->setVisibilityFlags (Element_Reference);
                            mObjectParts.push_back(object); // FIXME put every npc's parts into this for now
//#if 0
                            // check what entities we got
                            for (unsigned int k = 0; k < object->mEntities.size(); ++k)
                                std::cout << "cloth entities " << object->mEntities[k]->getName() << std::endl;
//#endif
// FIXME: just a test
#if 0
                            Ogre::SceneNode *tmp = mCellNode->createChildSceneNode();
                            NifOgre::ObjectScenePtr dummy
                                = NifOgre::Loader::createObjects (//mObjects[id]->getSceneNode(),
                                                                 tmp,
                                                                 "meshes\\"+clothRec.mModel);

                            if (dummy)
                                dummy->setVisibilityFlags (Element_Reference);
                            tmp->setPosition (Ogre::Vector3 (
                                record.get().mPos.pos[0], record.get().mPos.pos[1], record.get().mPos.pos[2]));
                            //tmp->setOrientation (zr*yr*xr);
                            tmp->setScale (record.get().mScale, record.get().mScale, record.get().mScale);
                            std::cout << "record " << record.get().mPos.pos[0] << ", "
                                << record.get().mPos.pos[1] << ", " << record.get().mPos.pos[2] << std::endl;
                            Ogre::Vector3 c = tmp->getPosition();
                            std::cout << "dummy pos " << c.x << ", " << c.y << ", " << c.z << std::endl;


                            //mObjects[id]->getObject()->mSkelBase->attachObjectToBone("Bip01", dummy->mSkelBase);
#endif
                            //Ogre::Entity* entity
                            //    = mObjects[id]->getSceneNode()->getCreator()->createEntity(
                            //                                                     Ogre::SceneManager::PT_CUBE);
                            //entity->setMaterialName("BaseWhite");
                            //entity->setVisibilityFlags (Element_Reference);
                            //mObjects[id]->getObject()->mSkelBase->attachObjectToBone("Bip01", entity);

                        }
#if 0
                        else
                            std::cout << "not cloth" << std::endl;
#endif
                        //std::cout << "inventory " << ESM4::formIdToString(npcRec.mInventory[i].item)
                        //<< ", " << npcRec.mInventory[i].count << std::endl;
                        // FIXME: attach to bones and add to mObjects
                    }
                }
            }

#if 0
            // sanity check z position
            float diff = record.get().mPos.pos[2] - getTerrainHeightAt(Ogre::Vector3(record.get().mPos.pos[0],
                                                                                     record.get().mPos.pos[1],
                                                                                     record.get().mPos.pos[2]));
            if (fabs(diff) > 500.f)
                std:: cout << "z diff: "<< diff << " formId " << ESM4::formIdToString(id) << std::endl;
#endif
        }
    }

    return modified;
}

CSVRender::ForeignCell::ForeignCell (CSMDoc::Document& document, Ogre::SceneManager *sceneManager,
    ESM4::FormId formId, ESM4::FormId worldId, boost::shared_ptr<CSVWorld::PhysicsSystem> physics,
    const Ogre::Vector3& origin)
: mDocument (document), mFormId (formId), mWorld(worldId)
, mProxyModel(0), mModel(0), mPgIndex(-1)//, mHandler(new CSMWorld::SignalHandler(this))
, mPhysics(physics), mSceneMgr(sceneManager), mX(0), mY(0)
{
    if (worldId) // no worldId for internal cells
    {
        const CSMForeign::WorldCollection& worlds = mDocument.getData().getForeignWorlds();
        const CSMForeign::World& world = worlds.getForeignRecord(worldId).get();
        if (world.mParent != 0)
            mWorld = world.mParent; // yes, really (but needs more testing - maybe have both or merge?)
    }

    mCellNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    mCellNode->setPosition (origin);

    const CSMForeign::CellCollection& cells = mDocument.getData().getForeignCells();
    const CSMForeign::Cell& cell = cells.getForeignRecord(formId).get();

    const CSMForeign::CellGroupCollection& cellGroups = mDocument.getData().getForeignCellGroups();
    const CSMForeign::CellGroup& cellGroup = cellGroups.getForeignRecord(formId).get();

    const CSMForeign::LandCollection& lands = mDocument.getData().getForeignLands();
    int landIndex = lands.searchFormId(cellGroup.mLand);

    std::istringstream stream (cell.mCellId.c_str());
    char ignore; // '#'
    int x = 0;
    int y = 0;
    stream >> ignore >> x >> y;
    mX = x;
    mY = y;

    if (landIndex != -1)
    {
        const CSMForeign::Land& esmLand = lands.getRecord(landIndex).get();

        if (esmLand.getLandData (ESM4::Land::LAND_VHGT))
        {
            mTerrain.reset(new ESM4Terrain::TerrainGrid(sceneManager,
                                                    new CSVForeign::TerrainStorage(mDocument.getData(), mWorld),
                                                    Element_Terrain,
                                                    true,
                                                    Terrain::Align_XY,
                                                    mWorld));
            mTerrain->loadCell(/*esmLand.mX*/x,
                               /*esmLand.mY*/y);

            float verts = ESM4::Land::VERTS_PER_SIDE;
            float worldsize = ESM4::Land::REAL_SIZE;
            //mX = esmLand.mX;
            //mY = esmLand.mY;

            mPhysics->addHeightField(sceneManager,
                esmLand.getLandData(ESM4::Land::LAND_VHGT)->mHeights, mX, mY, 0, worldsize / (verts-1), verts);
        }
        else
            std::cerr << "Heightmap for " << cell.mCellId << " not found" << std::endl;
    }
    else if (mWorld != 0) // FIXME need a better check for interiors
        std::cerr << "Land record for " << cell.mCellId << " not found, land formId " <<
            ESM4::formIdToString(cellGroup.mLand) << " cell formId " <<
            ESM4::formIdToString(cell.mFormId)
            << std::endl;

    // Moved from before terrain to after to get terrain heights for z position sanity check
    addObjects(cellGroup.mTemporary); // FIXME: ignore visible distant and persistent children for now
    addObjects(cellGroup.mPersistent); // FIXME: ignore visible distant children for now
    addObjects(cellGroup.mVisibleDist);

// FIXME: debugging
#if 0
    Ogre::SceneNode::ChildNodeIterator it2 = mCellNode->getChildIterator();
    while (it2.hasMoreElements())
    {
        Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(it2.getNext());
        if (node)
        {
            std::cout << "scenenode " << node->getName() << std::endl;

            Ogre::SceneNode::ObjectIterator it = node->getAttachedObjectIterator();
            while (it.hasMoreElements())
            {
                Ogre::Entity* entity = static_cast<Ogre::Entity*>(it.getNext());
                if (entity)
                {
                    std::cout << "obj " << entity->getMesh()->getName()
                        << ", " << (entity->getVisible()? "visible" : "not visible")
                        << ", " << (entity->isVisible()? "is visible" : "not is visible")
                        << std::endl;

                    Ogre::Entity::ChildObjectListIterator it3 = entity->getAttachedObjectIterator();
                    while (it3.hasMoreElements())
                    {
                        Ogre::MovableObject* entity3 = static_cast<Ogre::MovableObject*>(it3.getNext());
                        if (entity3)
                            std::cout << "child obj " << entity3->getName()
                                << ", " << (entity3->getVisible()? "visible" : "not visible")
                                << ", " << (entity3->isVisible()? "is visible" : "not is visible")
                                << std::endl;
                    }
                }
            }
        }
    }
#endif

    createGridMaterials();

    setupPathgrid();
    buildPathgrid();

    //setupNavMesh();
    //buildNavMesh();
}

CSVRender::ForeignCell::~ForeignCell()
{
    //clearNavMesh();

    clearPathgrid();

    destroyGridMaterials();

    delete mProxyModel;
    //delete mHandler;

    if (mTerrain.get())
        mPhysics->removeHeightField(mSceneMgr, mX, mY);

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
        delete iter->second;

    mCellNode->getCreator()->destroySceneNode (mCellNode);
}

bool CSVRender::ForeignCell::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    bool modified = false;

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            modified = true;

    return modified;
}

bool CSVRender::ForeignCell::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    bool modified = false;

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            modified = true;

    return modified;
}

bool CSVRender::ForeignCell::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
//#if 0
    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_ForeignReferences));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int cellColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
    int stateColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Modification);

    const CSMForeign::CellCollection& cells = mDocument.getData().getForeignCells();

    // list IDs in cell
    std::map<ESM4::FormId, bool> ids; // id, deleted state

    // NOTE: these are foreign references table indices
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
    {
        std::string cell = Misc::StringUtils::lowerCase (references.data (
            references.index (row, cellColumn)).toString().toUtf8().constData());

        std::istringstream stream (cell.c_str());
        char ignore;
        int x = 0;
        int y = 0;
        stream >> ignore >> x >> y;

        std::uint32_t formId = cells.searchCoord (x, y, mWorld);

        // check if the cell matches for this foreign reference
        if (formId == mFormId)
        {
            // get the foreign ref's formid
            ESM4::FormId refId = ESM4::stringToFormId(
                    references.data(references.index(row, idColumn)).toString().toUtf8().constData());

            // check its state
            int state = references.data (references.index (row, stateColumn)).toInt();

            ids.insert (std::make_pair (refId, state==CSMWorld::RecordBase::State_Deleted));
        }
    }

    // perform update and remove where needed
    bool modified = false;

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
    {
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            modified = true;

        std::map<ESM4::FormId, bool>::iterator iter2 = ids.find (iter->first);

        if (iter2 != ids.end())
        {
            if (iter2->second)
            {
                removeObject (iter->first);
                modified = true;
            }

            ids.erase (iter2);
        }
    }

    // add new objects
    for (std::map<ESM4::FormId, bool>::iterator iter (ids.begin()); iter != ids.end(); ++iter)
    {
        mObjects.insert (std::make_pair (
            iter->first, new ForeignObject (mDocument.getData(), mCellNode, iter->first, false, mPhysics)));

        modified = true;
    }

    return modified;
//#endif
//    return false;
}

bool CSVRender::ForeignCell::referenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    bool modified = false;

    for (int row = start; row <= end; ++row)
    {
        // FIXME: this assumes formid as string will be returned
        if (removeObject(ESM4::stringToFormId(
                    references.data(references.index(row, idColumn)).toString().toUtf8().constData())))
        {
            modified = true;
        }
    }

    return modified;
}

bool CSVRender::ForeignCell::referenceAdded (const QModelIndex& parent, int start, int end)
{
    if (parent.isValid())
        return false;

    std::vector<ESM4::FormId> objects;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_ForeignReferences));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    for (int row = start; row <= end; ++row)
    {
        // FIXME: this assumes formid as string will be returned
        objects.push_back(ESM4::stringToFormId(
                references.data(references.index(row, idColumn)).toString().toUtf8().constData()));
    }

    return addObjects(objects);
}

float CSVRender::ForeignCell::getTerrainHeightAt(const Ogre::Vector3 &pos) const
{
    if(mTerrain.get() != NULL)
        return mTerrain->getHeightAt(pos);
    else
        return -std::numeric_limits<float>::max();
}

void CSVRender::ForeignCell::pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    CSMWorld::IdTree *pathgrids = dynamic_cast<CSMWorld::IdTree *>(
                mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_Pathgrid));

    int idColumn = pathgrids->findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    int colPaths = pathgrids->findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);
    //int colEdges = pathgrids->findColumnIndex(CSMWorld::Columns::ColumnId_PathgridEdges);

    // FIXME: how to detect adds/deletes/modifies?

    for (int i=topLeft.row(); i<=bottomRight.row(); ++i)
    {
        std::string cell = Misc::StringUtils::lowerCase (pathgrids->data (
            pathgrids->index (i, idColumn)).toString().toUtf8().constData());

        if (/*cell==mId && */colPaths >= topLeft.column() && colPaths <= bottomRight.column())
        {
            if (!mModel)
                setupPathgrid();

            // FIXME: pathgird not yet supported
            //mHandler->rebuildPathgrid();
        }
    }
}

// FIXME:
//  - adding edges (need the ability to select a pathgrid and highlight)
//  - repainting edges while moving
void CSVRender::ForeignCell::setupPathgrid()
{
    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    int index = pathgrids.searchId(ESM4::formIdToString(mFormId)); // FIXME: this is going to fail since pathgrids expect Morrowind style mId
    if(index != -1)
    {
        int col = pathgrids.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);

        mPgIndex = index; // keep a copy to save from searching mId all the time

        mModel = dynamic_cast<CSMWorld::IdTree *>(
                mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_Pathgrid));

        mProxyModel = new CSMWorld::NestedTableProxyModel (mModel->index(mPgIndex, col),
                CSMWorld::ColumnBase::Display_NestedHeader, mModel);

    }
}

void CSVRender::ForeignCell::clearPathgrid()
{
    // destroy manual objects (edges)
    for(std::map<std::pair<int, int>, std::string>::iterator iter = mPgEdges.begin();
        iter != mPgEdges.end(); ++iter)
    {
        if(mSceneMgr->hasManualObject((*iter).second))
        {
            Ogre::ManualObject *manual = mSceneMgr->getManualObject((*iter).second);
            Ogre::SceneNode *node = manual->getParentSceneNode();
            mSceneMgr->destroyManualObject((*iter).second);
            if(mSceneMgr->hasSceneNode(node->getName()))
                mSceneMgr->destroySceneNode(node);
        }
    }
    mPgEdges.clear();

    // destroy points
    for(std::map<std::string, PathgridPoint *>::iterator iter (mPgPoints.begin());
        iter!=mPgPoints.end(); ++iter)
    {
        delete iter->second;
    }
    mPgPoints.clear();
}

// NOTE: getName() generates a string representation of mId+index to uniquely identify a
// pathgrid point.  The trouble is that the index can change when a pathgrid point is deleted.
// Need a new way of uniquely identifying a pathgrid point.
//
// A workaround is to re-generate the pathgrids and edges each time a point is deleted or
// undo() is called (probably via a signal)
void CSVRender::ForeignCell::buildPathgrid()
{
    if (!mModel)
        return;

    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    const CSMWorld::Pathgrid &pathgrid = pathgrids.getRecord(mPgIndex).get();

    int worldsize = ESM::Land::REAL_SIZE;

    std::vector<ESM::Pathgrid::Point>::const_iterator iter = pathgrid.mPoints.begin();
    for(int index = 0; iter != pathgrid.mPoints.end(); ++iter, ++index)
    {
        std::string name = PathgridPoint::getName(pathgrid.mId, index);

        Ogre::Vector3 pos =
            Ogre::Vector3(worldsize*mX+(*iter).mX, worldsize*mY+(*iter).mY, (*iter).mZ);

        mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
    }

    for(ESM::Pathgrid::EdgeList::const_iterator it = pathgrid.mEdges.begin();
        it != pathgrid.mEdges.end();
        ++it)
    {
        Ogre::SceneNode *node = mCellNode->createChildSceneNode();
        const ESM::Pathgrid::Edge &edge = *it;
        const ESM::Pathgrid::Point &p0 = pathgrid.mPoints[edge.mV0];
        const ESM::Pathgrid::Point &p1 = pathgrid.mPoints[edge.mV1];

        std::ostringstream stream;
        stream << pathgrid.mId << "_" << edge.mV0 << " " << edge.mV1;
        std::string name = stream.str();

        Ogre::ManualObject *line = createPathgridEdge(name,
            Ogre::Vector3(worldsize*mX+p0.mX, worldsize*mY+p0.mY, p0.mZ),
            Ogre::Vector3(worldsize*mX+p1.mX, worldsize*mY+p1.mY, p1.mZ));
        line->setVisibilityFlags(Element_Pathgrid);
        node->attachObject(line);

        mPgEdges.insert(std::make_pair(std::make_pair(edge.mV0, edge.mV1), name));
    }
}

// NOTE: pos is in world coordinates
void CSVRender::ForeignCell::pathgridPointAdded(const Ogre::Vector3 &pos, bool interior)
{
    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    CSMWorld::Pathgrid pathgrid = pathgrids.getRecord(mPgIndex).get();

    // FIXME: this is going to fail since pathgrids expect Morrowind style mId
    std::string name = PathgridPoint::getName(ESM4::formIdToString(mFormId), pathgrid.mPoints.size()); // generate a new name

    mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));

    // store to document
    int worldsize = ESM::Land::REAL_SIZE;

    int x = pos.x;
    int y = pos.y;
    if(!interior)
    {
        x = x - (worldsize * mX);
        y = y - (worldsize * mY);
    }

    ESM::Pathgrid::Point point(x, y, (int)pos.z);
    point.mConnectionNum = 0;
    pathgrid.mPoints.push_back(point);
    // FIXME: update other scene managers

    pathgrid.mData.mS2 += 1; // increment the number of points

    // TODO: check for possible issue if this cell is deleted and undo() is actioned afterwards
    CSMWorld::ModifyPathgridCommand *cmd = new CSMWorld::ModifyPathgridCommand(*mModel,
            mProxyModel->getParentId(), mProxyModel->getParentColumn(),
            new CSMWorld::PathgridPointsWrap(pathgrid));
    // FIXME: pathgird not yet supported
    //mHandler->connectToCommand(cmd);
    mDocument.getUndoStack().push(cmd);
    // emit signal here?
}

void CSVRender::ForeignCell::pathgridPointRemoved(const std::string &name)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(name);
    if(result.first == "")
        return;

    std::string pathgridId = result.first;
    int index = result.second;

    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    CSMWorld::Pathgrid pathgrid = pathgrids.getRecord(mPgIndex).get();

    // check if the point exists
    if(index < 0 || (unsigned int)index >= pathgrid.mPoints.size())
        return;

    int numToDelete = pathgrid.mPoints[index].mConnectionNum * 2; // for sanity check later
    int deletedEdgeCount = 0;

    // update edge indices to account for the deleted pathgrid point
    std::vector<ESM::Pathgrid::Edge>::iterator iter = pathgrid.mEdges.begin();
    for (; iter != pathgrid.mEdges.end();)
    {
        if (((*iter).mV0 == index) || ((*iter).mV1 == index))
        {
            iter = pathgrid.mEdges.erase(iter);
            pathgrid.mPoints[index].mConnectionNum -= 1;
            deletedEdgeCount++; // for sanity check later
        }
        else
        {
            if ((*iter).mV0 > index)
                (*iter).mV0--;

            if ((*iter).mV1 > index)
                (*iter).mV1--;

            ++iter;
        }
    }
    pathgrid.mPoints.erase(pathgrid.mPoints.begin()+index);
    pathgrid.mData.mS2 -= 1; // decrement the number of points

    if(deletedEdgeCount != numToDelete)
    {
        // WARNING: continue anyway?  Or should this be an exception?
        std::cerr << "The no of edges del does not match the no of conn for: "
            << pathgridId + "_" + QString::number(index).toStdString() << std::endl;
    }

    // TODO: check for possible issue if this cell is deleted and undo() is actioned afterwards
    CSMWorld::ModifyPathgridCommand *cmd = new CSMWorld::ModifyPathgridCommand(*mModel,
            mProxyModel->getParentId(), mProxyModel->getParentColumn(),
            new CSMWorld::PathgridPointsWrap(pathgrid));
    // FIXME: pathgird not yet supported
    //mHandler->connectToCommand(cmd);
    mDocument.getUndoStack().push(cmd);

    clearPathgrid();
    buildPathgrid();
}

// NOTE: newPos is in world coordinates
void CSVRender::ForeignCell::pathgridPointMoved(const std::string &name,
        const Ogre::Vector3 &newPos, bool interior)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(name);
    if(result.first == "")
        return;

    std::string pathgridId = result.first;
    int index = result.second;

    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    CSMWorld::Pathgrid pathgrid = pathgrids.getRecord(mPgIndex).get();

    // check if the point exists
    if(index < 0 || (unsigned int)index >= pathgrid.mPoints.size())
        return;

    int worldsize = ESM::Land::REAL_SIZE;

    int x = newPos.x;
    int y = newPos.y;
    if(!interior)
    {
        x = x - (worldsize * mX);
        y = y - (worldsize * mY);
    }

    pathgrid.mPoints[index].mX = x;
    pathgrid.mPoints[index].mY = y;
    pathgrid.mPoints[index].mZ = newPos.z;

    // TODO: check for possible issue if this cell is deleted and undo() is actioned afterwards
    CSMWorld::ModifyPathgridCommand *cmd = new CSMWorld::ModifyPathgridCommand(*mModel,
            mProxyModel->getParentId(), mProxyModel->getParentColumn(),
            new CSMWorld::PathgridPointsWrap(pathgrid));
    // FIXME: pathgird not yet supported
    //mHandler->connectToCommand(cmd);
    mDocument.getUndoStack().push(cmd);

    clearPathgrid();
    buildPathgrid();
}

// FIXME: save to the document
void CSVRender::ForeignCell::addPathgridEdge()
{
    // check if the points exist
    // update the edges
    // store to document
    // FIXME: update other scene managers
}

// FIXME: save to the document
void CSVRender::ForeignCell::removePathgridEdge()
{
}

CSMWorld::SignalHandler *CSVRender::ForeignCell::getSignalHandler()
{
    // FIXME: pathgird not yet supported
    return 0; //mHandler;
}
void CSVRender::ForeignCell::setupNavMesh()
{
    // FIXME: index throws exception
    //const CSMForeign::NavMeshCollection& navmeshes = mDocument.getData().getNavMeshes();
    int index = -1;// navmeshes.searchId(mId);
    if(index != -1)
    {
        std::cout << "CSVRender::ForeignCell: found navmesh" << std::endl;
    }
}

void CSVRender::ForeignCell::clearNavMesh()
{
    // FIXME
}

void CSVRender::ForeignCell::buildNavMesh()
{
#if 0 // no navmesh stuff yet
    const CSMForeign::NavigationCollection& navmeshes = mDocument.getData().getNavigation();

    const std::map<std::string, std::vector<std::string> >& cellmap = navmeshes.cellToFormIds();

    if (cellmap.find(mId) == cellmap.end())
        return;

    const std::vector<std::string>& formids = cellmap.find(mId)->second;

    for (std::vector<std::string>::const_iterator iter = formids.begin(); iter != formids.end(); ++iter)
    {
        int index = navmeshes.searchId(*iter);
        const CSMForeign::NavMeshInfo &navmesh = navmeshes.getRecord(index).get();

        if (navmesh.islandInfo.empty())
            continue;

        for (std::vector<ESM4::Navigation::IslandInfo>::const_iterator it = navmesh.islandInfo.begin();
                it != navmesh.islandInfo.end(); ++it)
        {
            //std::cout << "CSVRender MW: X "
                //<< std::dec << (*it).cellGrid.grid.x << ", Y " << (*it).cellGrid.grid.y << std::endl;

            for (std::vector<ESM4::Vertex>::const_iterator i = (*it).verticies.begin();
                    i != (*it).verticies.end(); ++i)
            {
                std::string name = PathgridPoint::getName(mId, index);

                Ogre::Vector3 pos = Ogre::Vector3((*i).x, (*i).y, (*i).z);

                mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
            }
        }
    }
#endif
}
