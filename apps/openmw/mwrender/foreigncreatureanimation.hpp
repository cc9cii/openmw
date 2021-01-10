#ifndef GAME_RENDER_FOREIGNCREATUREANIMATION_H
#define GAME_RENDER_FOREIGNCREATUREANIMATION_H

#include "animation.hpp"
#include "weaponanimation.hpp"
#include "../mwworld/inventorystore.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class ForeignCreatureAnimation : public Animation
    {
        std::vector<NifOgre::ObjectScenePtr> mObjectParts;
        void addForeignAnimSource(const std::string& model, const std::string &animName);
        NifOgre::ObjectScenePtr createObject(const std::string& meshName,
            const std::string& group, NiModelPtr skeletonModel);

        void hideDismember(NifOgre::ObjectScenePtr scene);

    public:
        ForeignCreatureAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~ForeignCreatureAnimation() {}

        NiModelPtr getSkeletonModel() const;

        Ogre::Vector3 runAnimation(float timepassed);
        void addAnimSource(const std::string &skeletonModel);

        void play(const std::string &groupname, int priority, int groups, bool autodisable,
            float speedmult, const std::string &start, const std::string &stop,
            float startpoint, size_t loops, bool loopfallback);
    };

    // For creatures with weapons and shields
    // Animation is already virtual anyway, so might as well make a separate class.
    // Most creatures don't need weapons/shields, so this will save some memory.
    class ForeignCreatureWeaponAnimation : public Animation, public WeaponAnimation, public MWWorld::InventoryStoreListener
    {
    public:
        ForeignCreatureWeaponAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~ForeignCreatureWeaponAnimation() {}

        virtual void equipmentChanged() { updateParts(); }

        virtual void showWeapons(bool showWeapon);
        virtual void showCarriedLeft(bool show);

        void updateParts();

        void updatePart(NifOgre::ObjectScenePtr& scene, int slot);

        virtual void attachArrow();
        virtual void releaseArrow();

        virtual Ogre::Vector3 runAnimation(float duration);

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character.
        virtual void setPitchFactor(float factor) { mPitchFactor = factor; }

        virtual void setWeaponGroup(const std::string& group) { mWeaponAnimationTime->setGroup(group); }

        // WeaponAnimation
        virtual NifOgre::ObjectScenePtr getWeapon() { return mWeapon; }
        virtual void showWeapon(bool show) { showWeapons(show); }
        virtual void configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot);

    private:
        NifOgre::ObjectScenePtr mWeapon;
        NifOgre::ObjectScenePtr mShield;
        bool mShowWeapons;
        bool mShowCarriedLeft;

        Ogre::SharedPtr<WeaponAnimationTime> mWeaponAnimationTime;
    };
}

#endif
