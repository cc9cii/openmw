#include "debugging.hpp"

#include <cassert>
#include <iostream> // FIXME

#include <OgreNode.h>
#include <OgreSceneManager.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreManualObject.h>
#include <OgreTechnique.h>
#include <OgreSceneNode.h>

#include <openengine/bullet/physic.hpp>

#include <components/esm/loadstat.hpp>
#include <components/esm/loadpgrd.hpp>

//#include <extern/esm4/land.hpp>
#include <extern/esm4/cell.hpp>
#include <extern/esm4/pgrd.hpp>

#include "../mwbase/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/foreigncell.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/pathfinding.hpp"

#include "renderconst.hpp"

using namespace Ogre;

namespace MWRender
{

static const std::string PATHGRID_POINT_MATERIAL = "pathgridPointMaterial";
static const std::string PATHGRID_LINE_MATERIAL = "pathgridLineMaterial";
static const std::string PATHGRID_ALT_POINT_MATERIAL = "pathgridPriorityPointMaterial";
static const std::string PATHGRID_ALT_LINE_MATERIAL = "pathgridAltLineMaterial";
static const std::string PATHGRID_ROAD_POINT_MATERIAL = "pathgridRoadPointMaterial";
static const std::string PATHGRID_ROAD_LINE_MATERIAL = "pathgridRoadLineMaterial";
static const std::string DEBUGGING_GROUP = "debugging";
static const float POINT_MESH_BASE = 35.f;

void Debugging::createGridMaterials()
{
    if (mGridMatsCreated) return;

    if (!MaterialManager::getSingleton().getByName(PATHGRID_LINE_MATERIAL, DEBUGGING_GROUP))
    {
        MaterialPtr lineMatPtr = MaterialManager::getSingleton().create(PATHGRID_LINE_MATERIAL, DEBUGGING_GROUP);
        lineMatPtr->setReceiveShadows(false);
        lineMatPtr->getTechnique(0)->setLightingEnabled(true);
        lineMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,1,0,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,1,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,1,0);
    }

    if (!MaterialManager::getSingleton().getByName(PATHGRID_ALT_LINE_MATERIAL, DEBUGGING_GROUP))
    {
        MaterialPtr lineMatPtr = MaterialManager::getSingleton().create(PATHGRID_ALT_LINE_MATERIAL, DEBUGGING_GROUP);
        lineMatPtr->setReceiveShadows(false);
        lineMatPtr->getTechnique(0)->setLightingEnabled(true);
        lineMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1.f,0.45f,0.f,0.f);
        lineMatPtr->getTechnique(0)->getPass(0)->setAmbient(1.f,0.45f,0.f);
        lineMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1.f,0.45f,0.f);
    }

    if (!MaterialManager::getSingleton().getByName(PATHGRID_POINT_MATERIAL, DEBUGGING_GROUP))
    {
        MaterialPtr pointMatPtr = MaterialManager::getSingleton().create(PATHGRID_POINT_MATERIAL, DEBUGGING_GROUP);
        pointMatPtr->setReceiveShadows(false);
        pointMatPtr->getTechnique(0)->setLightingEnabled(true);
        pointMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,0,0,0);
        pointMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,0,0);
        pointMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,0,0);
    }

    if (!MaterialManager::getSingleton().getByName(PATHGRID_ALT_POINT_MATERIAL, DEBUGGING_GROUP))
    {
        MaterialPtr pointMatPtr = MaterialManager::getSingleton().create(PATHGRID_ALT_POINT_MATERIAL, DEBUGGING_GROUP);
        pointMatPtr->setReceiveShadows(false);
        pointMatPtr->getTechnique(0)->setLightingEnabled(true);
        pointMatPtr->getTechnique(0)->getPass(0)->setDiffuse(0.f,0.f,1.f,0.f);
        pointMatPtr->getTechnique(0)->getPass(0)->setAmbient(0.f,0.f,1.f);
        pointMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(0.f,0.f,1.f);
    }
    mGridMatsCreated = true;
}

void Debugging::destroyGridMaterials()
{
    if (mGridMatsCreated)
    {
        MaterialManager::getSingleton().remove(PATHGRID_POINT_MATERIAL, DEBUGGING_GROUP);
        MaterialManager::getSingleton().remove(PATHGRID_LINE_MATERIAL, DEBUGGING_GROUP);
        mGridMatsCreated = false;
    }
}

ManualObject *Debugging::createPathgridLines(const ESM::Pathgrid *pathgrid)
{
    ManualObject *result = mSceneMgr->createManualObject();

    result->begin(PATHGRID_LINE_MATERIAL, RenderOperation::OT_LINE_LIST);
    for(ESM::Pathgrid::EdgeList::const_iterator it = pathgrid->mEdges.begin();
        it != pathgrid->mEdges.end();
        ++it)
    {
        const ESM::Pathgrid::Edge &edge = *it;
        const ESM::Pathgrid::Point &p1 = pathgrid->mPoints[edge.mV0], &p2 = pathgrid->mPoints[edge.mV1];
        Vector3 direction = (MWMechanics::PathFinder::MakeOgreVector3(p2) - MWMechanics::PathFinder::MakeOgreVector3(p1));
        Vector3 lineDisplacement = direction.crossProduct(Vector3::UNIT_Z).normalisedCopy();
        lineDisplacement = lineDisplacement * POINT_MESH_BASE +
                                Vector3(0.f, 0.f, 10.f); // move lines up a little, so they will be less covered by meshes/landscape
        result->position(MWMechanics::PathFinder::MakeOgreVector3(p1) + lineDisplacement);
        result->position(MWMechanics::PathFinder::MakeOgreVector3(p2) + lineDisplacement);
    }
    result->end();

    result->setVisibilityFlags (RV_Debug);

    return result;
}

ManualObject *Debugging::createPathgridPoints(const ESM::Pathgrid *pathgrid)
{
    ManualObject *result = mSceneMgr->createManualObject();
    const float height = POINT_MESH_BASE * sqrtf(2);

    result->begin(PATHGRID_POINT_MATERIAL, RenderOperation::OT_TRIANGLE_STRIP);

    bool first = true;
    uint32 startIndex = 0;
    for(ESM::Pathgrid::PointList::const_iterator it = pathgrid->mPoints.begin();
        it != pathgrid->mPoints.end();
        ++it, startIndex += 6)
    {
        Vector3 pointPos(MWMechanics::PathFinder::MakeOgreVector3(*it));

        if (!first)
        {
            // degenerate triangle from previous octahedron
            result->index(startIndex - 4); // 2nd point of previous octahedron
            result->index(startIndex); // start point of current octahedron
        }

        Ogre::Real pointMeshBase = static_cast<Ogre::Real>(POINT_MESH_BASE);

        result->position(pointPos + Vector3(0, 0, height)); // 0
        result->position(pointPos + Vector3(-pointMeshBase, -pointMeshBase, 0)); // 1
        result->position(pointPos + Vector3(pointMeshBase, -pointMeshBase, 0)); // 2
        result->position(pointPos + Vector3(pointMeshBase, pointMeshBase, 0)); // 3
        result->position(pointPos + Vector3(-pointMeshBase, pointMeshBase, 0)); // 4
        result->position(pointPos + Vector3(0, 0, -height)); // 5

        result->index(startIndex + 0);
        result->index(startIndex + 1);
        result->index(startIndex + 2);
        result->index(startIndex + 5);
        result->index(startIndex + 3);
        result->index(startIndex + 4);
        // degenerates
        result->index(startIndex + 4);
        result->index(startIndex + 5);
        result->index(startIndex + 5);
        // end degenerates
        result->index(startIndex + 1);
        result->index(startIndex + 4);
        result->index(startIndex + 0);
        result->index(startIndex + 3);
        result->index(startIndex + 2);

        first = false;
    }

    result->end();

    result->setVisibilityFlags (RV_Debug);

    return result;
}

Ogre::ManualObject *Debugging::createTES4PathgridLines(const ESM4::Pathgrid *pathgrid)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject();

    result->begin(PATHGRID_LINE_MATERIAL, RenderOperation::OT_LINE_LIST);

    std::vector<ESM4::Pathgrid::PGRP> nodes = pathgrid->mNodes;
    std::vector<ESM4::Pathgrid::PGRR> links = pathgrid->mLinks;
    for (std::size_t i = 0; i < links.size(); ++i)
    {
        const ESM4::Pathgrid::PGRP& p1 = nodes[links[i].startNode];
        const ESM4::Pathgrid::PGRP& p2 = nodes[links[i].endNode];

        Ogre::Vector3 start(p1.x, p1.y, p1.z+10.f);  // raise a little for visibility
        Ogre::Vector3 end(p2.x, p2.y, p2.z+10.f);    // raise a little for visibility

        result->position(start);
        result->position(end);
    }

    result->end();
    result->setVisibilityFlags (RV_Debug);

    return result;
}

Ogre::ManualObject *Debugging::createTES4PathgridConnections(const ESM4::Pathgrid *pathgrid)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject();

    result->begin(PATHGRID_ALT_LINE_MATERIAL, RenderOperation::OT_LINE_LIST);

    std::vector<ESM4::Pathgrid::PGRP> nodes = pathgrid->mNodes;
    std::vector<ESM4::Pathgrid::PGRI> conns = pathgrid->mForeign;

    for (std::size_t i = 0; i < conns.size(); ++i)
    {
        const ESM4::Pathgrid::PGRP& p1 = nodes[(conns[i].localNode & 0xffff)]; // sometimes junk bits

        Ogre::Vector3 start(p1.x, p1.y, p1.z+10.f);                 // raise a little for visibility
        Ogre::Vector3 end(conns[i].x, conns[i].y, conns[i].z+10.f); // raise a little for visibility

        result->position(start);
        result->position(end);
    }

    result->end();
    result->setVisibilityFlags (RV_Debug);

    return result;
}

Ogre::ManualObject *Debugging::createTES4PathgridPoints(const ESM4::Pathgrid *pathgrid)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject();
    const float height = POINT_MESH_BASE * sqrtf(2);

    bool priority = false;
    bool first = true;
    uint32 startIndex = 0;
    std::vector<ESM4::Pathgrid::PGRP> nodes = pathgrid->mNodes;
    for (std::size_t i = 0; i < nodes.size(); ++i, startIndex += 6)
    {
        if (first)
        {
            if (nodes[i].priority == 1)
            {
                result->begin(PATHGRID_ALT_POINT_MATERIAL, RenderOperation::OT_TRIANGLE_STRIP);
                priority = true;
            }
            else
                result->begin(PATHGRID_POINT_MATERIAL, RenderOperation::OT_TRIANGLE_STRIP);
        }
        else
        {
            if (priority && nodes[i].priority == 0) // changed
            {
                result->end();

                result->begin(PATHGRID_POINT_MATERIAL, RenderOperation::OT_TRIANGLE_STRIP);
                priority = false;
                first = true;
                startIndex = 0;
            }
            else if (!priority && nodes[i].priority == 1) // changed
            {
                result->end();

                result->begin(PATHGRID_ALT_POINT_MATERIAL, RenderOperation::OT_TRIANGLE_STRIP);
                priority = true;
                first = true;
                startIndex = 0;
            }
        }

        Ogre::Vector3 pointPos(nodes[i].x, nodes[i].y, nodes[i].z);
        if (!first)
        {
            // degenerate triangle from previous octahedron
            result->index(startIndex - 4); // 2nd point of previous octahedron
            result->index(startIndex); // start point of current octahedron
        }

        Ogre::Real pointMeshBase = static_cast<Ogre::Real>(POINT_MESH_BASE);

        result->position(pointPos + Vector3(             0,              0,  height)); // 0
        result->position(pointPos + Vector3(-pointMeshBase, -pointMeshBase,       0)); // 1
        result->position(pointPos + Vector3( pointMeshBase, -pointMeshBase,       0)); // 2
        result->position(pointPos + Vector3( pointMeshBase,  pointMeshBase,       0)); // 3
        result->position(pointPos + Vector3(-pointMeshBase,  pointMeshBase,       0)); // 4
        result->position(pointPos + Vector3(             0,              0, -height)); // 5

        result->index(startIndex + 0);
        result->index(startIndex + 1);
        result->index(startIndex + 2);
        result->index(startIndex + 5);
        result->index(startIndex + 3);
        result->index(startIndex + 4);
        // degenerates
        result->index(startIndex + 4);
        result->index(startIndex + 5);
        result->index(startIndex + 5);
        // end degenerates
        result->index(startIndex + 1);
        result->index(startIndex + 4);
        result->index(startIndex + 0);
        result->index(startIndex + 3);
        result->index(startIndex + 2);

        first = false;
    }

    result->end();
    result->setVisibilityFlags (RV_Debug);

    return result;
}

Debugging::Debugging(SceneNode *root, OEngine::Physic::PhysicEngine *engine) :
    mEngine(engine), mSceneMgr(root->getCreator()),
    mPathgridEnabled(false),
    mRootNode(root),
    mPathGridRoot(NULL), mInteriorPathgridNode(NULL),
    mGridMatsCreated(false)
{
    ResourceGroupManager::getSingleton().createResourceGroup(DEBUGGING_GROUP);
}

Debugging::~Debugging()
{
    if (mPathgridEnabled)
    {
        togglePathgrid();
    }

    ResourceGroupManager::getSingleton().destroyResourceGroup(DEBUGGING_GROUP);
}


bool Debugging::toggleRenderMode (int mode){
    switch (mode)
    {
        case MWBase::World::Render_CollisionDebug:

            return mEngine->toggleDebugRendering();

        case MWBase::World::Render_Pathgrid:
            togglePathgrid();
            return mPathgridEnabled;
    }

    return false;
}

void Debugging::cellAdded(MWWorld::CellStore *store)
{
    mActiveCells.push_back(store);
    if (mPathgridEnabled)
        enableCellPathgrid(store);
}

void Debugging::cellRemoved(MWWorld::CellStore *store)
{
    mActiveCells.erase(std::remove(mActiveCells.begin(), mActiveCells.end(), store), mActiveCells.end());
    if (mPathgridEnabled)
        disableCellPathgrid(store);
}

void Debugging::togglePathgrid()
{
    mPathgridEnabled = !mPathgridEnabled;
    if (mPathgridEnabled)
    {
        createGridMaterials();

        // add path grid meshes to already loaded cells
        mPathGridRoot = mRootNode->createChildSceneNode();
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); ++it)
        {
            enableCellPathgrid(*it);
        }
    }
    else
    {
        // remove path grid meshes from already loaded cells
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); ++it)
        {
            disableCellPathgrid(*it);
        }
        mPathGridRoot->removeAndDestroyAllChildren();
        mSceneMgr->destroySceneNode(mPathGridRoot);
        mPathGridRoot = NULL;
        destroyGridMaterials();
    }
}

void Debugging::enableCellPathgrid(MWWorld::CellStore *store)
{
    if (store->isForeignCell())
    {
        // TODO: there can be more than one?
        const ESM4::Pathgrid *pathgrid = store->getTES4Pathgrid();
        if (!pathgrid) return;


        Ogre::Vector3 cellPathGridPos = Ogre::Vector3::ZERO;
        std::int32_t gridX, gridY;
        if (store->getCell()->isExterior())
        {
            gridX = static_cast<const MWWorld::ForeignCell*>(store->getCell())->mCell->mX;
            gridY = static_cast<const MWWorld::ForeignCell*>(store->getCell())->mCell->mY;

            // NOTE: since the pathgrid points are already in world space, no need to convert them
            //cellPathGridPos.x = Ogre::Real(gridX * ESM4::Land::REAL_SIZE);
            //cellPathGridPos.y = Ogre::Real(gridY * ESM4::Land::REAL_SIZE);
        }
        SceneNode *cellPathGrid = mPathGridRoot->createChildSceneNode(cellPathGridPos);
        cellPathGrid->attachObject(createTES4PathgridPoints(pathgrid));
        // FIXME: The Best Defence basement doesn't have any links?
        cellPathGrid->attachObject(createTES4PathgridLines(pathgrid));
        cellPathGrid->attachObject(createTES4PathgridConnections(pathgrid));

        if (store->getCell()->isExterior())
        {
            // FIXME: this won't work with worldspaces and will be overwritten
            mExteriorPathgridNodes[std::make_pair(gridX, gridY)] = cellPathGrid;
        }
        else
        {
            mInteriorPathgridNode = cellPathGrid;
        }

        return;
    }

    MWBase::World* world = MWBase::Environment::get().getWorld();
    const ESM::Pathgrid *pathgrid =
        world->getStore().get<ESM::Pathgrid>().search(*store->getCell());
    if (!pathgrid) return;

    Vector3 cellPathGridPos(0, 0, 0);
    if (store->getCell()->isExterior())
    {
        cellPathGridPos.x = static_cast<Ogre::Real>(store->getCell()->mData.mX * ESM::Land::REAL_SIZE);
        cellPathGridPos.y = static_cast<Ogre::Real>(store->getCell()->mData.mY * ESM::Land::REAL_SIZE);
    }
    SceneNode *cellPathGrid = mPathGridRoot->createChildSceneNode(cellPathGridPos);
    cellPathGrid->attachObject(createPathgridLines(pathgrid));
    cellPathGrid->attachObject(createPathgridPoints(pathgrid));

    if (store->getCell()->isExterior())
    {
        mExteriorPathgridNodes[std::make_pair(store->getCell()->getGridX(), store->getCell()->getGridY())] = cellPathGrid;
    }
    else
    {
        assert(mInteriorPathgridNode == NULL);
        mInteriorPathgridNode = cellPathGrid;
    }
}

void Debugging::disableCellPathgrid(MWWorld::CellStore *store)
{
    if (store->getCell()->isExterior())
    {
        ExteriorPathgridNodes::iterator it =
                mExteriorPathgridNodes.find(std::make_pair(store->getCell()->getGridX(), store->getCell()->getGridY()));
        if (it != mExteriorPathgridNodes.end())
        {
            destroyCellPathgridNode(it->second);
            mExteriorPathgridNodes.erase(it);
        }
    }
    else
    {
        if (mInteriorPathgridNode)
        {
            destroyCellPathgridNode(mInteriorPathgridNode);
            mInteriorPathgridNode = NULL;
        }
    }
}

void Debugging::destroyCellPathgridNode(SceneNode *node)
{
    mPathGridRoot->removeChild(node);
    destroyAttachedObjects(node);
    mSceneMgr->destroySceneNode(node);
}

void Debugging::destroyAttachedObjects(SceneNode *node)
{
    SceneNode::ObjectIterator objIt = node->getAttachedObjectIterator();
    while (objIt.hasMoreElements())
    {
        MovableObject *mesh = static_cast<MovableObject *>(objIt.getNext());
        mSceneMgr->destroyMovableObject(mesh);
    }
}

}
