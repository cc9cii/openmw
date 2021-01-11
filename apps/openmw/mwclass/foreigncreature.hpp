#ifndef GAME_MWCLASS_FOREIGNCREATURE_H
#define GAME_MWCLASS_FOREIGNCREATURE_H

#include "../mwworld/class.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWWorld
{
    class InventoryStoreTES4;
}

namespace MWClass
{
    class ForeignCreature : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;
            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            MWWorld::InventoryStoreTES4& getInventoryStoreTES4 (const MWWorld::Ptr& ptr) const;
            MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;

            virtual int getSkill(const MWWorld::Ptr &ptr, int skill) const;

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual bool isActor() const {
                return true; // FIXME: FO3 treats turrets as "creatures"
            }

            //virtual bool isNpc() const {
            //    return false;
            //}

            virtual void killActor(const MWWorld::Ptr& ptr, const std::string& actor = "");
    };
}

#endif
