#ifndef GAME_RENDER_FOREIGNNPCANIMATION_H
#define GAME_RENDER_FOREIGNNPCANIMATION_H

#include "animation.hpp"

#include "../mwworld/inventorystore.hpp"

#include "weaponanimation.hpp"

namespace ESM4
{
    struct Npc;
    struct Race;
}

namespace MWWorld
{
    class ESMStore;
}

namespace MWRender
{
class ForeignHeadAnimationTime : public Ogre::ControllerValue<Ogre::Real>
{
private:
    MWWorld::Ptr mReference;
    float mTalkStart;
    float mTalkStop;
    float mBlinkStart;
    float mBlinkStop;

    float mBlinkTimer;

    bool mEnabled;

    float mValue;
private:
    void resetBlinkTimer();
public:
    ForeignHeadAnimationTime(MWWorld::Ptr reference);

    void update(float dt);

    void setEnabled(bool enabled);

    void setTalkStart(float value);
    void setTalkStop(float value);
    void setBlinkStart(float value);
    void setBlinkStop(float value);

    virtual Ogre::Real getValue() const;
    virtual void setValue(Ogre::Real value)
    { }
};

class ForeignNpcAnimation : public Animation, public WeaponAnimation, public MWWorld::InventoryStoreListener
{
public:
    virtual void equipmentChanged();
    virtual void permanentEffectAdded(const ESM::MagicEffect *magicEffect, bool isNew, bool playSound);

public:
    typedef std::map<ESM::PartReferenceType,std::string> PartBoneMap;

    enum ViewMode {
        VM_Normal,
        VM_FirstPerson,
        VM_HeadOnly
    };

private:
    static const PartBoneMap sPartList;

    bool mListenerDisabled;

// FIXME: temp testing
//#if 0
    float mPoseDuration;
    float mStartTimer;
    std::string mCurrentAnim;
    Ogre::AnimationStateSet *mHeadASSet;
    Ogre::AnimationStateSet *mTongueASSet;
    Ogre::AnimationStateSet *mMouthASSet;
    Ogre::AnimationStateSet *mTeethLASSet;
//#endif
    bool mIsTES4;
    bool mIsFO3;
    bool mIsFONV;
    bool mIsTES5;

    std::vector<NifOgre::ObjectScenePtr> mHeadParts;
    Ogre::TexturePtr mTextureHead;      // humans only
    Ogre::TexturePtr mTextureUpperBody; // humans only
    Ogre::TexturePtr mTextureLowerBody; // humans only
    Ogre::TexturePtr mTextureEars;
    std::map<Ogre::SubEntity*, Ogre::MaterialPtr> mClonedMaterials;

    std::map<std::uint32_t, NifOgre::ObjectScenePtr> mObjectParts; // TES4 has 16, FO3/FONV has 20 and TES5 has 32
    std::string mSoundIds[ESM::PRT_Count];

    const ESM4::Npc *mNpc;
    const ESM4::Race *mRace;
    std::string    mHeadModel;
    std::string    mHairModel;
    ViewMode       mViewMode;
    bool mShowWeapons;
    bool mShowCarriedLeft;

    enum NpcType
    {
        Type_Normal,
        Type_Werewolf,
        Type_Vampire
    };
    NpcType mNpcType;

    int mVisibilityFlags;

    int mPartslots[ESM::PRT_Count];  //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[ESM::PRT_Count];

    Ogre::Vector3 mFirstPersonOffset;

    Ogre::SharedPtr<ForeignHeadAnimationTime> mHeadAnimationTime;
    Ogre::SharedPtr<WeaponAnimationTime> mWeaponAnimationTime;

    float mAlpha;
    bool mSoundsDisabled;

    Ogre::Radian mHeadYaw;
    Ogre::Radian mHeadPitch;

    void updateTES4NpcBase();
    void updateFO3NpcBase();
    void updateTES5NpcBase();

    std::string getSkeletonModel(const MWWorld::ESMStore& store) const;

    NifOgre::ObjectScenePtr createSkinnedObject(NifOgre::ObjectScenePtr scene,
            const std::string& meshName, const std::string& group, NiModelPtr skeletonModel);

    NifOgre::ObjectScenePtr createMorphedObject(const std::string& meshName,
            const std::string& group, NiModelPtr skeletonModel, const Ogre::String& texture = "");

    NifOgre::ObjectScenePtr createObject(const std::string& meshName,
            const std::string& group , NiModelPtr skeletonModel, const std::string& texture = "");

    bool equipArmor(const ESM4::Armor* armor, bool isFemale);
    bool equipTES5Armor(const ESM4::Armor* armor, bool isFemale);
    bool equipClothes(const ESM4::Clothing* cloth, bool isFemale);
    void replaceMeshTexture(NifOgre::ObjectScenePtr scene, const std::string& npcTextureName);
    void replaceSkinTexture(NifOgre::ObjectScenePtr scene, const std::string& npcTextureName);
    void hideDismember(NifOgre::ObjectScenePtr scene);
    Ogre::MaterialPtr createClonedMaterials(Ogre::SubEntity *subEntity);
    void deleteClonedMaterials();

    NifOgre::ObjectScenePtr insertBoundedPart(const std::string &model, int group, const std::string &bonename,
                                              const std::string &bonefilter,
                                          bool enchantedGlow, Ogre::Vector3* glowColor=NULL);

    void removeParts(ESM::PartReferenceType type);
    void removeIndividualPart(ESM::PartReferenceType type);
    void reserveIndividualPart(ESM::PartReferenceType type, int group, int priority);

    bool addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh,
                                    bool enchantedGlow=false, Ogre::Vector3* glowColor=NULL);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts,
                                    bool enchantedGlow=false, Ogre::Vector3* glowColor=NULL);

    void applyAlpha(float alpha, Ogre::Entity* ent, NifOgre::ObjectScenePtr scene);

public:
    /**
     * @param ptr
     * @param node
     * @param visibilityFlags
     * @param disableListener  Don't listen for equipment changes and magic effects. InventoryStore only supports
     *                         one listener at a time, so you shouldn't do this if creating several NpcAnimations
     *                         for the same Ptr, eg preview dolls for the player.
     *                         Those need to be manually rendered anyway.
     * @param disableSounds    Same as \a disableListener but for playing items sounds
     * @param viewMode
     */
    ForeignNpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, int visibilityFlags, bool disableListener = false,
                 bool disableSounds = false, ViewMode viewMode=VM_Normal);
    virtual ~ForeignNpcAnimation();

    // TES4 kf files are derived differently
    virtual void addAnimSource(const std::string &model);
    void addForeignAnimSource(const std::string& model, const std::string &anim);
    NiModelPtr getSkeletonModel() const;
    Ogre::Entity *getSkelBase() const { return mSkelBase; }
    // FIXME: for temp testing
#if 1
        NifOgre::ObjectScenePtr apple;
#endif

    /** Plays an animation.
     * \param groupname Name of the animation group to play.
     * \param priority Priority of the animation. The animation will play on
     *                 bone groups that don't have another animation set of a
     *                 higher priority.
     * \param groups Bone groups to play the animation on.
     * \param autodisable Automatically disable the animation when it stops
     *                    playing.
     * \param speedmult Speed multiplier for the animation.
     * \param start Key marker from which to start.
     * \param stop Key marker to stop at.
     * \param startpoint How far in between the two markers to start. 0 starts
     *                   at the start marker, 1 starts at the stop marker.
     * \param loops How many times to loop the animation. This will use the
     *              "loop start" and "loop stop" markers if they exist,
     *              otherwise it may fall back to "start" and "stop", but only if
     *              the \a loopFallback parameter is true.
     * \param loopFallback Allow looping an animation that has no loop keys, i.e. fall back to use
     *                     the "start" and "stop" keys for looping?
     */
    virtual void play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback=false);

    virtual void enableHeadAnimation(bool enable);

    virtual void setWeaponGroup(const std::string& group) { mWeaponAnimationTime->setGroup(group); }

    virtual Ogre::Vector3 runAnimation(float timepassed);

    /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
    /// to indicate the facing orientation of the character.
    virtual void setPitchFactor(float factor) { mPitchFactor = factor; }

    virtual void setHeadPitch(Ogre::Radian pitch);
    virtual void setHeadYaw(Ogre::Radian yaw);
    virtual Ogre::Radian getHeadPitch() const;
    virtual Ogre::Radian getHeadYaw() const;

    virtual void showWeapons(bool showWeapon);
    virtual void showCarriedLeft(bool show);

    virtual void attachArrow();
    virtual void releaseArrow();

    // WeaponAnimation
    virtual NifOgre::ObjectScenePtr getWeapon() { return mObjectParts[ESM::PRT_Weapon]; }
    virtual void showWeapon(bool show) { showWeapons(show); }
    virtual void configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot);

    void setViewMode(ViewMode viewMode);

    void updateParts();

    /// \brief Applies a translation to the arms and hands.
    /// This may be called multiple times before the animation
    /// is updated to add additional offsets.
    void addFirstPersonOffset(const Ogre::Vector3 &offset);

    /// Rebuilds the NPC, updating their root model, animation sources, and equipment.
    void rebuild();

    /// Make the NPC only partially visible
    virtual void setAlpha(float alpha);

    virtual void setVampire(bool vampire);

    /// Prepare this animation for being rendered with \a camera (rotates billboard nodes)
    virtual void preRender (Ogre::Camera* camera);
};

}

#endif
