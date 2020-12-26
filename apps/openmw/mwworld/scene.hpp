#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include "../mwrender/renderingmanager.hpp"

#include "../mwbase/soundmanager.hpp"

#include "ptr.hpp"
#include "globals.hpp"

namespace Ogre
{
    class Vector3;
}

namespace ESM
{
    struct Position;
}

namespace Files
{
    class Collections;
}

namespace Loading
{
    class Listener;
}

namespace Render
{
    class OgreRenderer;
}

namespace MWRender
{
    class SkyManager;
    class CellRender;
}

namespace MWWorld
{
    class PhysicsSystem;
    class Player;
    class CellStore;

    class Scene
    {
        public:

            typedef std::set<CellStore *> CellStoreCollection;

        private:

            //OEngine::Render::OgreRenderer& mRenderer;
            CellStore* mCurrentCell; // the cell the player is in
            CellStoreCollection mActiveCells;
            CellStoreCollection mVisDistCells;
            bool mCellChanged;
            PhysicsSystem *mPhysics;
            MWRender::RenderingManager& mRendering;

            bool mNeedMapUpdate;

            void insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener);
            void insertVisibleDistCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener);

            // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
            void changeCellGrid (int X, int Y);

            // returns the current CellStore for grid x, y
            CellStore *updateWorldCellsAtGrid (ESM4::FormId worldId, int x, int y);

            void getGridCenter(int& cellX, int& cellY);

            std::string mCurrentAmbientSoundId;
            std::string mCurrentRadioSoundId;
            MWBase::SoundPtr mCurrentAmbientSound;
            MWBase::SoundPtr mCurrentRadioSound;

            std::vector<std::pair<int, int> > mCurrentLandscapes;
            void updateTES4LODLandscapeAtGrid (ESM4::FormId worldId, int X, int Y);

        public:

            Scene (MWRender::RenderingManager& rendering, PhysicsSystem *physics);

            ~Scene();

            void unloadCell (CellStoreCollection::iterator iter);

            void loadCell (CellStore *cell, Loading::Listener* loadingListener);

            void loadForeignCell (CellStore *cell, Loading::Listener* loadingListener, uint32_t worldId = 0);
            void updateDummyCell (CellStore *cell, int x, int y, Loading::Listener* loadingListener);
            void loadVisibleDist (CellStore *cell, Loading::Listener* loadingListener);

            void playerMoved (const Ogre::Vector3& pos);

            void changePlayerCell (CellStore* newCell, const ESM::Position& position, bool adjustPlayerPos);

            CellStore *getCurrentCell();

            const CellStoreCollection& getActiveCells () const;

            bool hasCellChanged() const;
            ///< Has the set of active cells changed, since the last frame?

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position);
            ///< Move to interior cell.
            void changeToForeignInteriorCell (const std::string& cellName, const ESM::Position& position);

            void changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos);
            ///< Move to exterior cell.
            void changeToWorldCell (ESM4::FormId worldId, const ESM::Position& position, bool adjustPlayerPos);

            void changeToVoid();
            ///< Change into a void

            void markCellAsUnchanged();

            void update (float duration, bool paused);

            void addObjectToScene (const Ptr& ptr);
            ///< Add an object that already exists in the world model to the scene.

            void removeObjectFromScene (const Ptr& ptr);
            ///< Remove an object from the scene, but not from the world model.

            void updateObjectLocalRotation (const Ptr& ptr);

            void moveSubObjectLocalPosition (const MWWorld::Ptr& ptr,
                    const std::string& boneName, const Ogre::Vector3& position);
            void rotateSubObjectLocalRotation (const MWWorld::Ptr& ptr,
                    const std::string& boneName, const Ogre::Quaternion& rotation);

            void updateObjectRotation (const Ptr& ptr);

            bool isCellActive(const CellStore &cell);

            Ptr searchPtrViaHandle (const std::string& handle);

            Ptr searchPtrViaActorId (int actorId);

            Ptr searchPtrViaFormId(ESM4::FormId formId);

            Ptr searchPtrViaEditorId (const std::string& editorId);
    };
}

#endif
