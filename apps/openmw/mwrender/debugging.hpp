#ifndef GAME_RENDER_MWSCENE_H
#define GAME_RENDER_MWSCENE_H

#include <utility>
#include <openengine/ogre/renderer.hpp>

#include <vector>
#include <string>

namespace ESM
{
    struct Pathgrid;
}

namespace ESM4
{
    struct Pathgrid;
    struct Road;
}

namespace OEngine
{
    namespace Physic
    {
        class PhysicEngine;
    }
}

namespace Ogre
{
    class Camera;
    class Viewport;
    class SceneManager;
    class SceneNode;
    class RaySceneQuery;
    class Quaternion;
    class Vector3;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWRender
{
    class Debugging
    {
        OEngine::Physic::PhysicEngine* mEngine;
        Ogre::SceneManager *mSceneMgr;

        // Path grid stuff
        bool mPathgridEnabled;
        bool mRoadEnabled;

        void togglePathgrid();
        void toggleRoad();

        typedef std::vector<MWWorld::CellStore *> CellList;
        CellList mActiveCells;
        const ESM4::Road* mRoad; // assumes max one road per world

        Ogre::SceneNode *mRootNode;

        Ogre::SceneNode *mPathGridRoot;

        typedef std::map<std::pair<int,int>, Ogre::SceneNode *> ExteriorPathgridNodes;
        ExteriorPathgridNodes mExteriorPathgridNodes;
        Ogre::SceneNode *mInteriorPathgridNode;

        Ogre::SceneNode *mRoadRoot;
        Ogre::SceneNode *mRoadNode;

        void enableCellPathgrid(MWWorld::CellStore *store);
        void disableCellPathgrid(MWWorld::CellStore *store);

        void enableRoad();
        void disableRoad();

        // utility
        void destroyCellPathgridNode(Ogre::SceneNode *node);
        void destroyRoadNode(Ogre::SceneNode *node);
        void destroyAttachedObjects(Ogre::SceneNode *node);

        // materials
        bool mGridMatsCreated;
        void createGridMaterials();
        void destroyGridMaterials();
        bool mRoadGridMatsCreated;
        void createRoadGridMaterials();
        void destroyRoadGridMaterials();

        // path grid meshes
        Ogre::ManualObject *createPathgridLines(const ESM::Pathgrid *pathgrid);
        Ogre::ManualObject *createPathgridPoints(const ESM::Pathgrid *pathgrid);

        Ogre::ManualObject *createTES4PathgridLines(const ESM4::Pathgrid *pathgrid);
        Ogre::ManualObject *createTES4PathgridConnections(const ESM4::Pathgrid *pathgrid);
        Ogre::ManualObject *createTES4PathgridPoints(const ESM4::Pathgrid *pathgrid);

        Ogre::ManualObject *createTES4RoadPoints(const ESM4::Road *road);
        Ogre::ManualObject *createTES4RoadLines(const ESM4::Road *road);
    public:
        Debugging(Ogre::SceneNode* root, OEngine::Physic::PhysicEngine *engine);
        ~Debugging();
        bool toggleRenderMode (int mode);

        void cellAdded(MWWorld::CellStore* store);
        void cellRemoved(MWWorld::CellStore* store);

        void roadAdded(const ESM4::Road* road);
    };


}

#endif
