#ifndef OPENMW_MWWORLD_CELLREF_H
#define OPENMW_MWWORLD_CELLREF_H

#include <extern/esm4/formid.hpp>

#include <components/esm/cellref.hpp>

namespace ESM4
{
    struct Reference;
    struct ActorCreature;
    struct ActorCharacter;
}

namespace ESM
{
    struct ObjectState;
}

namespace MWWorld
{

    /// \brief Encapsulated variant of ESM::CellRef with change tracking
    class CellRef
    {
    public:

        CellRef (const ESM::CellRef& ref)
            : mCellRef(ref), mFormId(0), mIsLocked(false)
        {
            mChanged = false;
        }

        CellRef (const ESM4::Reference& ref);
        CellRef (const ESM4::ActorCreature& ref);
        CellRef (const ESM4::ActorCharacter& ref);

        // Note: Currently unused for items in containers
        const ESM::RefNum& getRefNum() const;

        ESM4::FormId getFormId() const;
        ESM4::FormId getParentFormId() const;
        std::uint32_t getFlags() const { return mFlags; }
        ESM4::FormId getBaseObj() const { return mBaseObj; } // FIXME: for testing visibly dist

        bool isLocked() const { return mIsLocked; }

        // teleport destination door
        ESM4::FormId getDestDoorId() const;

        // Set RefNum to its default state.
        void unsetRefNum();

        /// Does the RefNum have a content file?
        bool hasContentFile() const;

        // Id of object being referenced
        std::string getRefId() const;

        // For doors - true if this door teleports to somewhere else, false
        // if it should open through animation.
        bool getTeleport() const;

        // Teleport location for the door, if this is a teleporting door.
        ESM::Position getDoorDest() const;

        // Destination cell for doors (optional)
        std::string getDestCell() const;

        // Scale applied to mesh
        float getScale() const;
        void setScale(float scale);

        // The *original* position and rotation as it was given in the Construction Set.
        // Current position and rotation of the object is stored in RefData.
        ESM::Position getPosition() const;
        void setPosition (const ESM::Position& position);

        // Remaining enchantment charge. This could be -1 if the charge was not touched yet (i.e. full).
        float getEnchantmentCharge() const;

        void setEnchantmentCharge(float charge);

        // For weapon or armor, this is the remaining item health.
        // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
        // If this returns int(-1) it means full health.
        int getCharge() const;
        float getChargeFloat() const; // Implemented as union with int charge
        void setCharge(int charge);
        void setChargeFloat(float charge);

        // The NPC that owns this object (and will get angry if you steal it)
        std::string getOwner() const;
        void setOwner(const std::string& owner);

        // Name of a global variable. If the global variable is set to '1', using the object is temporarily allowed
        // even if it has an Owner field.
        // Used by bed rent scripts to allow the player to use the bed for the duration of the rent.
        std::string getGlobalVariable() const;

        void resetGlobalVariable();

        // ID of creature trapped in this soul gem
        std::string getSoul() const;
        void setSoul(const std::string& soul);

        // The faction that owns this object (and will get angry if
        // you take it and are not a faction member)
        std::string getFaction() const;
        void setFaction (const std::string& faction);

        // PC faction rank required to use the item. Sometimes is -1, which means "any rank".
        void setFactionRank(int factionRank);
        int getFactionRank() const;

        // Lock level for doors and containers
        // Positive for a locked door. 0 for a door that was never locked.
        // For an unlocked door, it is set to -(previous locklevel)
        int getLockLevel() const;
        void setLockLevel(int lockLevel);
         // Key and trap ID names, if any
        std::string getKey() const;
        std::string getTrap() const;
        void setTrap(const std::string& trap);

        // This is 5 for Gold_005 references, 100 for Gold_100 and so on.
        int getGoldValue() const;
        void setGoldValue(int value);

        // Write the content of this CellRef into the given ObjectState
        void writeState (ESM::ObjectState& state) const;

        // Has this CellRef changed since it was originally loaded?
        bool hasChanged() const;

    private:
        bool mChanged;
        ESM::CellRef mCellRef;
        ESM4::FormId mFormId;
        ESM4::FormId mParentFormId;
        ESM4::FormId mDestDoorId; // FIXME: don't want this here
        std::uint32_t mFlags;
        ESM4::FormId mBaseObj;

        bool mIsLocked;
    };

}

#endif
