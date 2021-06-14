#include "actionteleportforeign.hpp"

#include <extern/esm4/cell.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/esmstore.hpp"

#include "foreigncell.hpp"
#include "foreignworld.hpp"
#include "player.hpp"

namespace
{

    void getFollowers (const MWWorld::Ptr& actor, std::set<MWWorld::Ptr>& out)
    {
        std::list<MWWorld::Ptr> followers = MWBase::Environment::get().getMechanicsManager()->getActorsFollowing(actor);
        for(std::list<MWWorld::Ptr>::iterator it = followers.begin();it != followers.end();++it)
        {
            if (out.insert(*it).second)
            {
                getFollowers(*it, out);
            }
        }
    }

}

namespace MWWorld
{
    ActionTeleportForeign::ActionTeleportForeign (const std::string& cellName, ESM4::FormId cellId,
        const ESM::Position& position, bool teleportFollowers)
    : Action (true), mCellName (cellName), mCellId(cellId), mPosition (position), mTeleportFollowers(teleportFollowers)
    {
    }

    void ActionTeleportForeign::executeImp (const Ptr& actor)
    {
        if (mTeleportFollowers)
        {
            //find any NPC that is following the actor and teleport him too
            std::set<MWWorld::Ptr> followers;
            getFollowers(actor, followers);
            for(std::set<MWWorld::Ptr>::iterator it = followers.begin();it != followers.end();++it)
            {
                MWWorld::Ptr follower = *it;
                if (Ogre::Vector3(follower.getRefData().getPosition().pos).squaredDistance(
                            Ogre::Vector3( actor.getRefData().getPosition().pos))
                        <= 800*800)
                    teleport(*it);
            }
        }

        teleport(actor);
    }

    void ActionTeleportForeign::teleport(const Ptr &actor)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        if(actor == world->getPlayerPtr())
        {
            world->getPlayer().setTeleported(true);
            const ForeignCell *cell = world->getStore().getForeign<ForeignCell>().search(mCellId);
            if (cell)
            {
                if (cell->isExterior())
                {
                        ESM4::FormId worldId = cell->mCell->mParent;
                        const ForeignWorld * foreignWorld = world->getStore().getForeign<ForeignWorld>().find(worldId);
                        world->changeToForeignWorldCell(foreignWorld->mEditorId, mPosition);
                }
                else
                    world->changeToForeignInteriorCell (mCellName, mPosition);
            }
        }
        else
        {
            if (mCellName.empty())
            {
                const ForeignCell *cell = world->getStore().getForeign<ForeignCell>().search(mCellId);
                if (cell)
                {
                    ESM4::FormId worldId = cell->mCell->mParent;

                    int cellX;
                    int cellY;
                    world->positionToIndex(mPosition.pos[0], mPosition.pos[1], cellX, cellY);
                    world->moveObject(actor,world->getWorldCell(worldId, cellX, cellY),
                        mPosition.pos[0], mPosition.pos[1], mPosition.pos[2]);
                }
            }
            else
                world->moveObject(actor,
                        world->getForeignInterior(mCellName), mPosition.pos[0], mPosition.pos[1], mPosition.pos[2]);
        }
    }
}
