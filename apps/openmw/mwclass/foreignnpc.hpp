#ifndef GAME_MWCLASS_FOREIGNNPC_H
#define GAME_MWCLASS_FOREIGNNPC_H

#include "../mwworld/class.hpp"

namespace MWWorld
{
    class InventoryStoreTES4;
    class InventoryStoreFO3;
    class InventoryStoreTES5;
}

namespace MWClass
{
    class ForeignNpc : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;
            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual bool hasInventoryStore(const MWWorld::Ptr &ptr) const { return true; }

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            virtual MWMechanics::NpcStats& getNpcStats (const MWWorld::Ptr& ptr) const;
            ///< Return NPC stats

            virtual MWWorld::ContainerStore& getContainerStore (const MWWorld::Ptr& ptr) const;
            ///< Return container store

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            MWWorld::InventoryStoreTES4& getInventoryStoreTES4 (const MWWorld::Ptr& ptr) const;
            MWWorld::InventoryStoreFO3& getInventoryStoreFO3 (const MWWorld::Ptr& ptr) const;
            MWWorld::InventoryStoreTES5& getInventoryStoreTES5 (const MWWorld::Ptr& ptr) const;
            ///< Return inventory store

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;

            virtual MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement.

            virtual Ogre::Vector3 getMovementVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired movement vector (determined based on movement settings,
            /// stance and stats).

            virtual Ogre::Vector3 getRotationVector (const MWWorld::Ptr& ptr) const;
            ///< Return desired rotations, as euler angles.

            virtual float getCapacity (const MWWorld::Ptr& ptr) const;
            ///< Return total weight that fits into the object. Throws an exception, if the object can't
            /// hold other objects.

            virtual float getEncumbrance (const MWWorld::Ptr& ptr) const;
            ///< Returns total weight of objects inside this object (including modifications from magic
            /// effects). Throws an exception, if the object can't hold other objects.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;
            ///< @return combined armor rating of this actor

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            virtual int getSkill(const MWWorld::Ptr& ptr, int skill) const;

            /// Get a blood texture suitable for \a ptr (see Blood Texture 0-2 in Morrowind.ini)
            virtual int getBloodTexture (const MWWorld::Ptr& ptr) const;

            virtual bool isActor() const {
                return true;
            }

            virtual bool isNpc() const {
                return true;
            }

            virtual bool isBipedal (const MWWorld::Ptr &ptr) const;

            virtual int getBaseFightRating (const MWWorld::Ptr& ptr) const;

            virtual std::string getPrimaryFaction(const MWWorld::Ptr &ptr) const;

            virtual int getPrimaryFactionRank(const MWWorld::Ptr &ptr) const;

            virtual void killActor(const MWWorld::Ptr& ptr, const std::string& actor = "");
    };
}

#endif
