#ifndef GAME_MWCLASS_FOREIGNDOOR_H
#define GAME_MWCLASS_FOREIGNDOOR_H

#include <extern/esm4/door.hpp>

#include "../mwworld/class.hpp"

namespace MWClass
{
    class ForeignDoor : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;

            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;

            virtual boost::shared_ptr<MWWorld::Action> activate (const MWWorld::Ptr& ptr,
                const MWWorld::Ptr& actor) const;

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;

            static std::string getDestination (const MWWorld::LiveCellRef<ESM4::Door>& door);

            /// 0 = nothing, 1 = opening, 2 = closing
            virtual int getDoorState (const MWWorld::Ptr &ptr) const;

            virtual void setDoorState (const MWWorld::Ptr &ptr, int state) const;

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
                const;

            virtual void writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
                const;

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            static void registerSelf();
    };
}

#endif
