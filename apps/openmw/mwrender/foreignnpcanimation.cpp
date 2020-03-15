#include "foreignnpcanimation.hpp"

#include <memory>
#include <iostream>
#include <iomanip> // FIXME: for debugging only setprecision

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSubEntity.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreSceneNode.h>
#include <OgreBone.h>
#include <OgreMesh.h>
#include <OgreTechnique.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreTexturemanager.h>
#include <OgrePixelFormat.h>
#include <OgreCommon.h> // Ogre::Box
#include <OgreMaterialManager.h>

#include <extern/shiny/Main/Factory.hpp>

#include <extern/esm4/lvlc.hpp>
#include <extern/esm4/formid.hpp> // mainly for debugging
#include <extern/nibtogre/btogreinst.hpp>
#include <extern/nibtogre/nimodelmanager.hpp>
#include <extern/nibtogre/ninode.hpp>
#include <extern/fglib/fgsam.hpp>
#include <extern/fglib/fgfile.hpp>
#include <extern/fglib/fgegt.hpp>

#include <components/misc/rng.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <components/nifogre/ogrenifloader.hpp> // ObjectScenePtr

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "renderconst.hpp"
#include "camera.hpp"

namespace
{

std::string getVampireHead(const std::string& race, bool female)
{
    static std::map <std::pair<std::string,int>, const ESM::BodyPart* > sVampireMapping;

    std::pair<std::string, int> thisCombination = std::make_pair(race, int(female));

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();
        for(MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            const ESM::BodyPart& bodypart = *it;
            if (!bodypart.mData.mVampire)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;
            if (bodypart.mData.mPart != ESM::BodyPart::MP_Head)
                continue;
            if (female != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            if (!Misc::StringUtils::ciEqual(bodypart.mRace, race))
                continue;
            sVampireMapping[thisCombination] = &*it;
        }
    }

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
        sVampireMapping[thisCombination] = NULL;

    const ESM::BodyPart* bodyPart = sVampireMapping[thisCombination];
    if (!bodyPart)
        return std::string();
    return "meshes\\" + bodyPart->mModel;
}

bool isSkinned (NifOgre::ObjectScenePtr scene)
{
    if (scene->mSkelBase == NULL)
        return false;
    std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        Ogre::Entity *ent = it->second;
        if(/*scene->mSkelBase != ent &&*/ ent->hasSkeleton())// FIXME: not sure why commented out check is needed
        {
            return true;
        }
    }
    return false;
}

// for adding skinned body parts, we will need to create different models for each of the
// skeleton types; the model names have to be different for each because they are cached in
// Ogre::ResourceManager
std::string bodyPartNameExt(const std::string& skeletonModel)
{
    size_t pos = skeletonModel.find_last_of('\\');
    if (pos == std::string::npos)
        return ""; // FIXME: should throw here

    std::string lowerName = Misc::StringUtils::lowerCase(skeletonModel.substr(pos+1)); // +1 for '\\'
    if (lowerName == "skeleton.nif")
        return "_skeleton";
    else if (lowerName == "skeleton_female.nif") // TES5 only
        return "_skeleton_female";
    else if (lowerName == "skeletonbeast_female.nif") // TES5 only
        return "_skeletonbeat_female";
    else if (lowerName == "skeletonbeast.nif")
        return "_beast";
    else // skeletonsesheogorath
        return "_sesheogorath";
}

}


namespace MWRender
{
ForeignHeadAnimationTime::ForeignHeadAnimationTime(MWWorld::Ptr reference)
    : mReference(reference), mTalkStart(0), mTalkStop(0), mBlinkStart(0), mBlinkStop(0), mEnabled(true), mValue(0)
{
    resetBlinkTimer();
}

void ForeignHeadAnimationTime::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

void ForeignHeadAnimationTime::resetBlinkTimer()
{
    mBlinkTimer = -(2.0f + Misc::Rng::rollDice(6));
}

void ForeignHeadAnimationTime::update(float dt)
{
    if (!mEnabled)
        return;

    if (MWBase::Environment::get().getSoundManager()->sayDone(mReference))
    {
        mBlinkTimer += dt;

        float duration = mBlinkStop - mBlinkStart;

        if (mBlinkTimer >= 0 && mBlinkTimer <= duration)
        {
            mValue = mBlinkStart + mBlinkTimer;
        }
        else
            mValue = mBlinkStop;

        if (mBlinkTimer > duration)
            resetBlinkTimer();
    }
    else
    {
        mValue = mTalkStart +
            (mTalkStop - mTalkStart) *
            std::min(1.f, MWBase::Environment::get().getSoundManager()->getSaySoundLoudness(mReference)*2); // Rescale a bit (most voices are not very loud)
    }
}

float ForeignHeadAnimationTime::getValue() const
{
    return mValue;
}

void ForeignHeadAnimationTime::setTalkStart(float value)
{
    mTalkStart = value;
}

void ForeignHeadAnimationTime::setTalkStop(float value)
{
    mTalkStop = value;
}

void ForeignHeadAnimationTime::setBlinkStart(float value)
{
    mBlinkStart = value;
}

void ForeignHeadAnimationTime::setBlinkStop(float value)
{
    mBlinkStop = value;
}

static ForeignNpcAnimation::PartBoneMap createPartListMap()
{
    ForeignNpcAnimation::PartBoneMap result;
    result.insert(std::make_pair(ESM::PRT_Head, "Head"));
    result.insert(std::make_pair(ESM::PRT_Hair, "Head")); // note it uses "Head" as attach bone, but "Hair" as filter
    result.insert(std::make_pair(ESM::PRT_Neck, "Neck"));
    result.insert(std::make_pair(ESM::PRT_Cuirass, "Chest"));
    result.insert(std::make_pair(ESM::PRT_Groin, "Groin"));
    result.insert(std::make_pair(ESM::PRT_Skirt, "Groin"));
    result.insert(std::make_pair(ESM::PRT_RHand, "Right Hand"));
    result.insert(std::make_pair(ESM::PRT_LHand, "Left Hand"));
    result.insert(std::make_pair(ESM::PRT_RWrist, "Right Wrist"));
    result.insert(std::make_pair(ESM::PRT_LWrist, "Left Wrist"));
    result.insert(std::make_pair(ESM::PRT_Shield, "Shield Bone"));
    result.insert(std::make_pair(ESM::PRT_RForearm, "Right Forearm"));
    result.insert(std::make_pair(ESM::PRT_LForearm, "Left Forearm"));
    result.insert(std::make_pair(ESM::PRT_RUpperarm, "Right Upper Arm"));
    result.insert(std::make_pair(ESM::PRT_LUpperarm, "Left Upper Arm"));
    result.insert(std::make_pair(ESM::PRT_RFoot, "Right Foot"));
    result.insert(std::make_pair(ESM::PRT_LFoot, "Left Foot"));
    result.insert(std::make_pair(ESM::PRT_RAnkle, "Right Ankle"));
    result.insert(std::make_pair(ESM::PRT_LAnkle, "Left Ankle"));
    result.insert(std::make_pair(ESM::PRT_RKnee, "Right Knee"));
    result.insert(std::make_pair(ESM::PRT_LKnee, "Left Knee"));
    result.insert(std::make_pair(ESM::PRT_RLeg, "Right Upper Leg"));
    result.insert(std::make_pair(ESM::PRT_LLeg, "Left Upper Leg"));
    result.insert(std::make_pair(ESM::PRT_RPauldron, "Right Clavicle")); // used for ear in TES4
    result.insert(std::make_pair(ESM::PRT_LPauldron, "Left Clavicle"));  // used for eye in TES4
    result.insert(std::make_pair(ESM::PRT_Weapon, "Weapon Bone"));
    result.insert(std::make_pair(ESM::PRT_Tail, "Tail"));                // used for tail in TES4
    return result;
}
const ForeignNpcAnimation::PartBoneMap ForeignNpcAnimation::sPartList = createPartListMap();

ForeignNpcAnimation::~ForeignNpcAnimation()
{
    deleteClonedMaterials();

    if (!mListenerDisabled
            // No need to getInventoryStore() to reset, if none exists
            // This is to avoid triggering the listener via ensureCustomData()->autoEquip()->fireEquipmentChanged()
            // all from within this destructor. ouch!
           && mPtr.getRefData().getCustomData())
        mPtr.getClass().getInventoryStore(mPtr).setListener(NULL, mPtr);
}


ForeignNpcAnimation::ForeignNpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, int visibilityFlags, bool disableListener, bool disableSounds, ViewMode viewMode)
  : Animation(ptr, node),
    mListenerDisabled(disableListener),
    mViewMode(viewMode),
    mShowWeapons(false),
    mShowCarriedLeft(true),
    mNpcType(Type_Normal),
    mVisibilityFlags(visibilityFlags),
    mFirstPersonOffset(0.f, 0.f, 0.f),
    mAlpha(1.f),
    mSoundsDisabled(disableSounds),
    mHeadYaw(0.f),
    mHeadPitch(0.f)
{
    mNpc = mPtr.get<ESM4::Npc>()->mBase;
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    mRace = store.getForeign<ESM4::Race>().search(mNpc->mRace); // WARN: might be null

    mHeadAnimationTime = Ogre::SharedPtr<ForeignHeadAnimationTime>(new ForeignHeadAnimationTime(mPtr));
    mWeaponAnimationTime = Ogre::SharedPtr<WeaponAnimationTime>(new WeaponAnimationTime(this));

    // FIXME for foreign
    for(size_t i = 0;i < ESM::PRT_Count;i++)
    {
        mPartslots[i] = -1;  //each slot is empty
        mPartPriorities[i] = 0;
    }

    mHeadParts.clear();

    updateNpcBase();

    if (!disableListener)
        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);
}

void ForeignNpcAnimation::setViewMode(ForeignNpcAnimation::ViewMode viewMode)
{
    assert(viewMode != VM_HeadOnly);
    if(mViewMode == viewMode)
        return;

    mViewMode = viewMode;
    rebuild();
}

void ForeignNpcAnimation::rebuild()
{
    updateNpcBase();

    MWBase::Environment::get().getMechanicsManager()->forceStateUpdate(mPtr);
}

// clearAnimSources() - FIXME may be need to cache them rather than clearing each time?
// get the npc model based on race, sex, etc
// setObjectRoot() - use the model to setup:
//    NifOgre::ObjectScenePtr mObjectRoot and Ogre::Entity *mSkelBase
// addAnimSource() - FIXME need to figure out how
// updateParts() - get the correct body parts based on inventory
//
// NOTE: maybe updateParts needs to come before updating the NPC models - this is due to the high
// cost of updating FaceGen models for head, hair, eyes, ears, etc.  Or at least consider if the
// relevant slot is already occupied by the equipped items.
void ForeignNpcAnimation::updateNpcBase()
{
    // create and store morphed parts here
    // * head is always needed
    // * upper body & lower body textures may be used for exposed skin areas of clothes/armor
    // * ears' textures need to be morphed
    //
    // eyes/hands/feet textures are not morphed - just use those specified in the RACE records
    //
    // hair textures?
    //
    // remember to destroy any cloned materials

    if (!mNpc || !mRace)
        return;

    clearAnimSources(); // clears *all* animations

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    std::string skeletonModel = getSkeletonModel(store);
    if (skeletonModel.empty())
        return; // FIXME: perhaps should log an error first

    Misc::StringUtils::lowerCaseInPlace(skeletonModel);
    size_t pos = skeletonModel.find_last_of('\\');
    if (pos == std::string::npos)
        throw std::runtime_error(mNpc->mEditorId + "NPC skeleton.nif path could not be derived.");

    std::string skeletonPath = skeletonModel.substr(0, pos+1); // +1 is for '\\'
    bool isFemale = (mNpc->mBaseConfig.flags & 0x1) != 0; // FIXME: move to constructor?

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    std::string group("General");

    if (mInsert->getScale().x != 1.0f) // WARN: assumed uniform scaling
        std::cout << "scale not 1.0 " << skeletonModel << std::endl;

    setForeignObjectRootBase(skeletonModel); // create the skeleton
    if (mObjectRoot->mForeignObj)
    {
        mObjectRoot->mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot; // Ogre::Entity*
        mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
    }

    if (mSkelBase == nullptr) // FIXME: FO3
        return;

    mSkelBase->getSkeleton()->reset(true); // seems to fix the twisted torso
    //mSkelBase->getMesh()->getSkeleton()->reset(true); // but this doesn't work

    //Ogre::Bone *fBone = mSkelBase->getSkeleton()->getBone("Bip01 L Forearm");
    //Ogre::Bone *tBone = mSkelBase->getSkeleton()->getBone("Bip01 L ForeTwist");
    //Ogre::Quaternion q = fBone->getOrientation();
    //tBone->setOrientation(q);//Ogre::Quaternion::IDENTITY);
    //tBone->setPosition(Ogre::Vector3(0.f,0.f, 0.f));
    //q.w = 0.f;
    //tBone->setOrientation(fBone->getOrientation());
    //fBone = mSkelBase->getSkeleton()->getBone("Bip01 R Forearm");
    //tBone = mSkelBase->getSkeleton()->getBone("Bip01 R ForeTwist");
    //tBone->setOrientation(fBone->getOrientation());
        //tBone->rotate(Ogre::Quaternion(Ogre::Degree(-45), Ogre::Vector3::UNIT_Z) ,Ogre::Node::TS_WORLD);


    // Animation at 90 deg issue:
    //
    // Apparently the TES4 engine does things a little differently to what the NIF fies suggest.  See:
    // https://forums.nexusmods.com/index.php?/topic/278149-animation-rotates-90-degrees-to-the-right/
    // http://wiki.tesnexus.com/index.php?title=How_to_fix_your_animation_FAQ
    // http://wiki.tesnexus.com/index.php/Avoiding_Blender_animation_pitfalls
    //
    // A quick workaround might be two swap the rotation values of "Bip01" and "Bip01 NonAccum".
    // Note that simple swapping causes walk forward animation to walk backwards.
    //
    // A better one might be not to apply rotations to "Bip01" (only apply to "Bip01 NonAccum")
    // but that doesn't seem to work?!
    Ogre::Bone* b = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01");
    Ogre::Bone* bna = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01 NonAccum");
    if (b && bna)
    {
        //Ogre::Quaternion qb = b->getOrientation(); // skeleton.nif has -90 roll
        //Ogre::Quaternion qbna = bna->getOrientation(); // has 0 roll
        //b->setOrientation(qbna);
        //bna->setOrientation(qb);






        // FIXME: all the skinned meshes seems to be offset by this, probably something to do
        // with the binding position
        Ogre::Vector3 vb = b->getPosition();
        Ogre::Vector3 vbna = bna->getPosition();
        Ogre::Vector3 ins = mInsert->getPosition();

        b->setPosition(vbna+ Ogre::Vector3(0,0,0.f)); // TEMP testing for FO3
//      bna->setPosition(vb);

        //Ogre::Bone* rootBone = mObjectRoot->mSkelBase->getSkeleton()->getBone("Scene Root");
        //Ogre::Vector3 vRoot = rootBone->convertLocalToWorldPosition(rootBone->_getDerivedPosition());
        //Ogre::Vector3 vBase = mSkelBase->getParentSceneNode()->_getDerivedPosition();
        //std::cout << vRoot.x << " " << vBase.x << std::endl;
        //std::cout << vRoot.y << " " << vBase.y << std::endl;
        //std::cout << vRoot.z << " " << vBase.z << std::endl;






    }

    if(mViewMode != VM_FirstPerson)
    {
        addAnimSource(skeletonModel);
#if 0
        if(!isWerewolf)
        {
            if(Misc::StringUtils::lowerCase(mNpc->mRace).find("argonian") != std::string::npos)
                addAnimSource("meshes\\xargonian_swimkna.nif");
            else if(!mNpc->isMale() && !isBeast)
                addAnimSource("meshes\\xbase_anim_female.nif");
            if(mNpc->mModel.length() > 0)
                addAnimSource("meshes\\x"+mNpc->mModel);
        }
#endif
    }
    else
    {
        bool isFemale = (mNpc->mBaseConfig.flags & 0x000001) != 0; // 0x1 means female
#if 0
        if(isWerewolf)
            addAnimSource(skeletonModel);
        else
        {
            /* A bit counter-intuitive, but unlike third-person anims, it seems
             * beast races get both base_anim.1st.nif and base_animkna.1st.nif.
             */
            addAnimSource("meshes\\xbase_anim.1st.nif");
            if(isBeast)
                addAnimSource("meshes\\xbase_animkna.1st.nif");
            if(isFemale && !isBeast)
                addAnimSource("meshes\\xbase_anim_female.1st.nif");
        }
#endif
    }

    //MWRender::Animation
    // Ogre::Entity    *mSkelBase
    // Ogre::SceneNode *mInsert
    // ObjectScenePtr   mObjectRoot
    std::string modelName;


    bool isTES4 = true;
    std::string meshName;
    std::string textureName;

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator invHeadGear = inv.getSlot(MWWorld::InventoryStore::Slot_ForeignHair);
    if (invHeadGear == inv.end())
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Hair *hair = store.getForeign<ESM4::Hair>().search(mNpc->mHair);
        if (!hair)
        {
            // try to use the Race defaults
            hair = store.getForeign<ESM4::Hair>().search(mRace->mDefaultHair[isFemale ? 1 : 0]);
            if (!hair)
            {
                // NOTE "TG05Dremora" does not have a hair record nor race defaults
                // FIXME: we should remember our random selection?
                int hairChoice = Misc::Rng::rollDice(int(mRace->mHairChoices.size()-1));
                hair = store.getForeign<ESM4::Hair>().search(mRace->mHairChoices[hairChoice]);

                if (!hair)
                    throw std::runtime_error(mNpc->mEditorId + " - cannot find the hair.");
            }
        }
        meshName = "meshes\\" + hair->mModel;
        textureName = "textures\\" + hair->mIcon;

        NiModelPtr model = modelManager.getByName(mNpc->mEditorId+"_"+meshName, group);
        if (!model)
            model = modelManager.createMorphedModel(meshName,
                    group, mNpc, mRace, mObjectRoot->mForeignObj->mModel.get(), textureName, NiBtOgre::NiModelManager::BP_Hair);

        NifOgre::ObjectScenePtr scene
            = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj
            = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(model, mInsert->createChildSceneNode()));
        scene->mForeignObj->instantiate();

        std::string targetBone = model->getTargetBone();
        // Characters\Hair\ArgonianSpines.NIF does not have a targetBone
        if (targetBone == "")
            targetBone = "Bip01 Head"; // HACK: give a guessed default

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone(targetBone);
        Ogre::Quaternion heOrientation // fix helmet issue
            = heBone->getOrientation() * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y);

        // FIXME non-zero for FO3 to make hair fit better
        //Ogre::Vector3 hePosition = Ogre::Vector3(/*up*/0.2f, /*forward*/1.7f, 0.f);
        Ogre::Vector3 hePosition = Ogre::Vector3(0.5f, -0.2f, 0.f);
        //Ogre::Vector3 hePosition = Ogre::Vector3(0.f, -0.2f, 0.f);

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(it->second);
            Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
            while(techIter.hasMoreElements())
            {
                Ogre::Technique *tech = techIter.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    //Ogre::TextureUnitState *tex = pass->getTextureUnitState(0);
                    //tex->setColourOperation(Ogre::LBO_ALPHA_BLEND);
                    //tex->setColourOperation(Ogre::LBO_REPLACE);
                    //tex->setBlank(); // FIXME: testing
                    Ogre::ColourValue ambient = pass->getAmbient();
                    ambient.r = (float)mNpc->mHairColour.red / 256.f;
                    ambient.g = (float)mNpc->mHairColour.green / 256.f;
                    ambient.b = (float)mNpc->mHairColour.blue / 256.f;
                    ambient.a = 1.f;
                    pass->setSceneBlending(Ogre::SBT_REPLACE);
                    pass->setAmbient(ambient);
                    pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_AMBIENT);
                }
            }

            if (targetBone != "")
                mSkelBase->attachObjectToBone(targetBone, it->second, heOrientation, hePosition);
        }
        mObjectParts[ESM4::Armor::TES4_Hair] = scene;
    }

    for (int index = ESM4::Race::Head; index < ESM4::Race::NumHeadParts; ++index)
    {
        // skip 2 if male, skip 1 if female (ears)
        if ((isFemale && index == ESM4::Race::EarMale) || (!isFemale && index == ESM4::Race::EarFemale))
            continue;

        // FIXME: ears need texture morphing
        // skip ears if wearing a helmet - check for the head slot
        if ((index == ESM4::Race::EarMale || index == ESM4::Race::EarFemale) && (invHeadGear != inv.end()))
            continue;

        // FIXME: skip mouth, teeth (upper/lower) and tongue for now
        if (index >= ESM4::Race::Mouth && index <= ESM4::Race::Tongue)
            continue;

        // FIXME: we do head elsewhere for now
        // FIXME: if we retrieve a morphed head the morphing is no longer present!
        if (index == ESM4::Race::Head)
            continue;

        if (mRace->mHeadParts[index].mesh == "") // FIXME: horrible workaround
        {
            std::string missing;
            switch (index)
            {
            case 1: missing = "Ear Male"; break;
            case 2: missing = "Ear Female"; break;
            default: break;
            }
            std::cout << mNpc->mEditorId <<", a " << (isFemale ? "female," : "male,")
                << " does not have headpart \"" << missing <<"\"." << std::endl;

            continue;
        }

        // Get mesh and texture from RACE except eye textures which are specified in Npc::mEyes
        // (NOTE: Oblivion.esm NPC_ records all have valid mEyes formid if one exists)

        meshName = "meshes\\" + mRace->mHeadParts[index].mesh;

        if (index == ESM4::Race::EyeLeft || index == ESM4::Race::EyeRight)
        {
            const ESM4::Eyes* eyes = store.getForeign<ESM4::Eyes>().search(mNpc->mEyes);
            if (!eyes)
            {
                // NOTE: "ClaudettePerrick" does not have an eye record, for example
                // FIXME: we should remember our random selection?
                int eyeChoice = Misc::Rng::rollDice(int(mRace->mEyeChoices.size()-1));
                eyes = store.getForeign<ESM4::Eyes>().search(mRace->mEyeChoices[eyeChoice]);

                if (!eyes)
                    throw std::runtime_error(mNpc->mEditorId + " - cannot find the eye texture.");
            }

            textureName = "textures\\" + eyes->mIcon;
        }
        else
            textureName = "textures\\" + mRace->mHeadParts[index].texture;

        // TODO: if the texture doesn't exist, then grab it from the mesh (but shouldn't happen, so
        // log an error before proceeding with the fallback)


        // FaceGen:
        // dependency: model and texture files found
        //
        // Find the corresponding EGM file in the same directory as the NIF file. If it doesn't
        // exist, log an error and abandon any morphs for this mesh.
        //
        // Find the corresponding TRI file in the same directory as the NIF file. In a few cases
        // they don't exist so construct a dummy one from the NIF file.
        //
        // Morph the vertices in the TRI file using the RACE and NPC_ morph coefficients and the EGM
        // file.
        //
        // Find the corresponding EGT file in same directory as the NIF file. If it doesn't
        // exist, log an error and abandon any morphs for this texture.
        //
        // detail modulation: need to find the age from NPC_ symmetric morph coefficients.
        //
        // Morph the texture using the NPC_ morph coefficients, detail modulation and the EGT file.
        //
        // Find the detai map texture from "textures\\faces\\oblivion.esm\\" for the Npc::mFormId.
        //
        // Create the object using the morphed vertices, morphed texture and the detail map.
        //
        // TODO: to save unnecessary searches for the resources, these info should be persisted

        if (index == 1 || index == 2) // ears use morphed textures
        {
            mHeadParts.push_back(
                createMorphedObject(meshName, "General", mObjectRoot->mForeignObj->mModel));

            FgLib::FgSam sam;
            if (sam.getMorphedTexture(mTextureEars, meshName, textureName, mNpc->mEditorId,
                                      mRace->mSymTextureModeCoefficients,
                                      mNpc->mSymTextureModeCoefficients))
            {
                replaceMeshTexture(mHeadParts.back(), mTextureEars->getName());
            }
        }
        else
            mHeadParts.push_back(
                createMorphedObject(meshName, "General", mObjectRoot->mForeignObj->mModel, textureName));
    }

    // default meshes for upperbody /lower body/hands/feet are in the same directory as skeleton.nif
    const std::vector<ESM4::Race::BodyPart>& bodyParts
        = (isFemale ? mRace->mBodyPartsFemale : mRace->mBodyPartsMale);

    for (int index = ESM4::Race::UpperBody; index < ESM4::Race::NumBodyParts; ++index)
    {
        meshName = bodyParts[index].mesh;
        textureName = "textures\\" + bodyParts[index].texture; // TODO: can it be empty string?

        // need a mapping of body parts to equipment slot, which are different for each game
        // FIXME: for now just implement TES4
        if (!isTES4)
            throw std::runtime_error("ForeignNpcAnimation: not TES4");

        int type = 0;
        int invSlot = MWWorld::InventoryStore::Slots;
        switch (index)
        {
            case(ESM4::Race::UpperBody):
            {
                // FIXME: human upperbody need texture morphing
                type = ESM4::Armor::TES4_UpperBody;
                invSlot = MWWorld::InventoryStore::Slot_ForeignUpperBody;
                meshName = skeletonPath + (isFemale ? "female" : "") + "upperbody.nif";
                break;
            }
            case(ESM4::Race::LowerBody):
            {
                // FIXME: human lowerbody need texture morphing
                type = ESM4::Armor::TES4_LowerBody;
                invSlot = MWWorld::InventoryStore::Slot_ForeignLowerBody;
                meshName = skeletonPath + (isFemale ? "female" : "") + "lowerbody.nif";
                break;
            }
            case(ESM4::Race::Hands):
            {
                type = ESM4::Armor::TES4_Hands;
                invSlot = MWWorld::InventoryStore::Slot_ForeignHands;
                meshName = skeletonPath + (isFemale ? "female" : "") + "hand.nif";
                break;
            }
            case(ESM4::Race::Feet):
            {
                type = ESM4::Armor::TES4_Feet;
                invSlot = MWWorld::InventoryStore::Slot_ForeignFeet;
                meshName = skeletonPath + (isFemale ? "female" : "") + "foot.nif";
                break;
            }
            case(ESM4::Race::Tail):
            {
                type = ESM4::Armor::TES4_Tail;
                invSlot = MWWorld::InventoryStore::Slot_ForeignTail;
                if (meshName != "") meshName = "meshes\\" + meshName;
                break;
            }
            default: break;
        }

        // FIXME: group "General"
        MWWorld::ContainerStoreIterator invChest = inv.getSlot(invSlot);
        if (index == ESM4::Race::UpperBody || index == ESM4::Race::LowerBody) // build always
        {
            removeIndividualPart((ESM::PartReferenceType)type);

            Ogre::TexturePtr& bodyTexturePtr
                = ((index == ESM4::Race::UpperBody) ?  mTextureUpperBody : mTextureLowerBody);

            // morphed texture only for humans which we detect by whether we're using headhuman.nif
            std::string headMeshName = mRace->mHeadParts[ESM4::Race::Head].mesh;
            Misc::StringUtils::lowerCaseInPlace(headMeshName);
            if (headMeshName.find("headhuman") != std::string::npos)
            {
                mObjectParts[type] = createObject(meshName, "General", mObjectRoot->mForeignObj->mModel);

                FgLib::FgSam sam;
                std::string npcTextureName;
                if (sam.getMorphedBodyTexture(bodyTexturePtr, meshName, textureName, mNpc->mEditorId,
                                          mRace->mSymTextureModeCoefficients,
                                          mNpc->mSymTextureModeCoefficients))
                {
                    npcTextureName = bodyTexturePtr->getName();
                    replaceMeshTexture(mObjectParts[type], npcTextureName);
                }
            }
            else // probably argonian or khajiit
            {
                mObjectParts[type] =
                        createObject(meshName, "General", mObjectRoot->mForeignObj->mModel, textureName);

                Ogre::ResourcePtr baseTexture = Ogre::TextureManager::getSingleton().createOrRetrieve(
                       textureName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
                if (!baseTexture)
                    return; // FIXME: throw?

                bodyTexturePtr = Ogre::static_pointer_cast<Ogre::Texture>(baseTexture);
            }
        }
        else if (invChest == inv.end()  // body part only if it is not occupied by some equipment
                && meshName != "") // tail may be empty
        {
            removeIndividualPart((ESM::PartReferenceType)type);

            mObjectParts[type] =
                    createObject(meshName, "General", mObjectRoot->mForeignObj->mModel, textureName);
        }
    }


    meshName = "meshes\\" + mRace->mHeadParts[ESM4::Race::Head].mesh;
    if (meshName.empty())
    {
        if (mRace->mEditorId == "Imperial" || mRace->mEditorId == "Nord"     ||
            mRace->mEditorId == "Breton"   || mRace->mEditorId == "Redguard" ||
            mRace->mEditorId == "HighElf"  || mRace->mEditorId == "DarkElf"  ||
            mRace->mEditorId == "WoodElf"  || mRace->mEditorId == "Dremora")
            meshName = "meshes\\Characters\\Imperial\\headhuman.nif";
        else if (mRace->mEditorId == "Argonian")
            meshName = "meshes\\Characters\\Argonian\\headargonian.nif";
        else if (mRace->mEditorId == "Orc")
            meshName = "meshes\\Characters\\Orc\\headorc.nif";
        else if (mRace->mEditorId == "Khajiit")
            meshName = "meshes\\Characters\\Khajiit\\headkhajiit.nif";
        else if (0) // FO3
        {
            isTES4 = false;
            isFemale = (mNpc->mActorBaseConfig.flags & 0x1) != 0;

            // FIXME: can be female, ghoul, child, old, etc
            if (mRace->mEditorId.find("Old") != std::string::npos)
            {
                if (isFemale)
                    meshName = "meshes\\Characters\\head\\headold.nif";
                else
                    meshName = "meshes\\Characters\\head\\headoldfemale.nif";
            }
            else if (mRace->mEditorId.find("Child") != std::string::npos)
            {
                if (isFemale)
                    meshName = "meshes\\Characters\\head\\headchildfemale.nif";
                else
                    meshName = "meshes\\Characters\\head\\headchild.nif";
            }
            else
            {
                if (isFemale)
                    meshName = "meshes\\Characters\\head\\headhumanfemale.nif";
                else
                    meshName = "meshes\\Characters\\head\\headhuman.nif";
            }
            // FO3 races
            // CaucasianOldAged
            // AfricanAmericanOldAged
            // AsianOldAged
            // HispanicOldAged
            // AfricanAmericanRaider
            // AsianRaider
            // HispanicRaider
            // CaucasianRaider
            // TestQACaucasian
            // HispanicOld
            // HispanicChild
            // CaucasianOld
            // CaucasianChild
            // AsianOld
            // AsianChild
            // AfricanAmericanOld
            // AfricanAmericanChild
            // AfricanAmerican
            // Ghoul
            // Asian
            // Hispanic
            // Caucasian
            //std::cout << "missing head " << mRace->mEditorId << std::endl;
            //else
                //return;
        }
    }

    textureName = "textures\\" + mRace->mHeadParts[ESM4::Race::Head].texture;

    // deprecated
    const std::vector<float>& sRaceCoeff = mRace->mSymShapeModeCoefficients;
    //const std::vector<float>& aRaceCoeff = mRace->mAsymShapeModeCoefficients;
    const std::vector<float>& sRaceTCoeff = mRace->mSymTextureModeCoefficients;
    const std::vector<float>& sCoeff = mNpc->mSymShapeModeCoefficients;
    //const std::vector<float>& aCoeff = mNpc->mAsymShapeModeCoefficients;
    const std::vector<float>& sTCoeff = mNpc->mSymTextureModeCoefficients;

    FgLib::FgSam sam;
    Ogre::Vector3 sym;
    //Ogre::Vector3 asym;

    // aged texture only for humans which we detect by whether we're using headhuman.nif
    std::string headMeshName = mRace->mHeadParts[ESM4::Race::Head].mesh;
    Misc::StringUtils::lowerCaseInPlace(headMeshName);
    bool hasAgedTexture = headMeshName.find("headhuman") != std::string::npos;

    std::string ageTextureFile;
    if (hasAgedTexture)
    {
        ageTextureFile
            = sam.getHeadHumanDetailTexture(meshName, sam.getAge(sRaceCoeff, sCoeff), isFemale);

        // find the corresponding normal texture
        /*std::size_t*/ pos = ageTextureFile.find_last_of(".");
        if (pos == std::string::npos)
            return; // FIXME: should throw
    }

    std::string faceDetailFile
        = sam.getNpcDetailTexture_0(ESM4::formIdToString(mNpc->mFormId));


    NiModelPtr model = modelManager.getByName(mNpc->mEditorId + "_" + meshName, group);
    if (!model)
        model = modelManager.createMorphedModel(meshName, group, mNpc, mRace,
                        mObjectRoot->mForeignObj->mModel.get(), textureName, NiBtOgre::NiModelManager::BP_Head);

    NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    scene->mForeignObj
        = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(model, mInsert->createChildSceneNode()));
    scene->mForeignObj->instantiate();

    //std::string targetBone = model->getTargetBone();

    // get the texture from mRace
    // FIXME: for now, get if from Ogre material

    std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        if (mRace->mEditorId != "Dremora") // don't morph Dremora textures
        {
        Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(it->second);
        Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
        while(techIter.hasMoreElements())
        {
            Ogre::Technique *tech = techIter.getNext();
            //tech->removeAllPasses();
            Ogre::Technique::PassIterator passes = tech->getPassIterator();
            while(passes.hasMoreElements())
            {
                Ogre::Pass *pass = passes.getNext();

#if 0
                Ogre::TextureUnitState *tus = pass->getTextureUnitState(0);
                Ogre::PixelFormat pixelFormat = tus->_getTexturePtr()->getFormat();

                FgLib::FgFile<FgLib::FgEgt> egtFile;
                const FgLib::FgEgt *egt = egtFile.getOrLoadByName(meshName);

                // From: http://wiki.ogre3d.org/Creating+dynamic+textures
                // Create a target texture
                Ogre::TexturePtr texFg = Ogre::TextureManager::getSingleton().getByName(
                       "FaceGen" + mNpc->mEditorId, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (texFg.isNull())
                {
                    texFg = Ogre::TextureManager::getSingleton().createManual(
                        "FaceGen"+mNpc->mEditorId, // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,  // type
                        egt->numRows(), egt->numColumns(), // width & height
                        0,                  // number of mipmaps; FIXME: should be 2? or 1?
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                            // textures updated very often (e.g. each frame)
                }

                Ogre::TexturePtr texFg2 = Ogre::TextureManager::getSingleton().getByName(
                       "FaceGen"+mNpc->mEditorId+"2", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (texFg2.isNull())
                {
                    texFg2 = Ogre::TextureManager::getSingleton().createManual(
                        "FaceGen"+mNpc->mEditorId+"2", // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,  // type
                        egt->numRows(), egt->numColumns(), // width & height
                        0,                  // number of mipmaps; FIXME: should be 2? or 1?
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                            // textures updated very often (e.g. each frame)
                }
                std::string textureFile = "textures\\faces\\oblivion.esm\\" + ESM4::formIdToString(mNpc->mFormId) + "_0.dds";
                if (mNpc->mEditorId == "UrielSeptim")
                    textureFile = "textures\\characters\\imperial\\headhumanm60.dds"; // male
                else if (mNpc->mEditorId == "Rohssan")
                    textureFile = "textures\\characters\\imperial\\headhumanf60.dds"; // female
                else if (mNpc->mFormId == 0x0001C458) // BeggarICMarketSimplicia
                    textureFile = "textures\\characters\\imperial\\headhumanf60.dds"; // female

                Ogre::TexturePtr texDetail = Ogre::TextureManager::getSingleton().getByName(
                        textureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (texDetail.isNull())
                {
                    // textureFile is the detailed texture in textures/faces/oblivion.esm/<formid>_.dds
                    texDetail = Ogre::TextureManager::getSingleton().create(
                        textureFile, // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
                    );
                    texDetail->load();
                }

                // src: can be 128x128
                Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc = tus->_getTexturePtr()->getBuffer();
                pixelBufferSrc->unlock(); // prepare for blit()

                // dest: usually 256x256 (egt.numRows()*egt.numColumns())
                Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texFg->getBuffer();
                pixelBuffer->unlock(); // prepare for blit()

                Ogre::HardwarePixelBufferSharedPtr pixelBuffer2 = texFg2->getBuffer();
                pixelBuffer2->unlock();

                // if source and destination dimensions don't match, scaling is done
                pixelBuffer->blit(pixelBufferSrc);

                // Lock the pixel buffer and get a pixel box
                pixelBufferSrc->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!

                const Ogre::PixelBox& pixelBoxSrc = pixelBufferSrc->getCurrentLock();
                const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

                uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

                Ogre::HardwarePixelBufferSharedPtr pixelBufferDetail = texDetail->getBuffer();
                if (pixelBufferDetail.isNull())
                    std::cout << "detail texture null" << std::endl;

                pixelBufferDetail->unlock();
                pixelBuffer2->blit(pixelBufferDetail);

                pixelBuffer2->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                pixelBufferDetail->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                const Ogre::PixelBox& pixelBoxDetail = pixelBuffer2->getCurrentLock();
                uint8_t *pDetail = static_cast<uint8_t*>(pixelBoxDetail.data);
                if (!pDetail)
                    std::cout << "null detail" << std::endl;

                const std::vector<Ogre::Vector3>& symTextureModes = egt->symTextureModes();

                // update the pixels with SCM and detail texture
                // NOTE: mSymTextureModes is assumed to have the image pixels in row-major order
                for (size_t i = 0; i < egt->numRows()*egt->numColumns(); ++i) // height*width, should be 256*256
                {
                    // FIXME: for some reason adding the race coefficients makes it look worse
                    //        even though it is clear that for shapes they are needed
                    // sum all the symmetric texture modes for a given pixel i
                    sym = Ogre::Vector3::ZERO; // WARN: sym reused
                    // CheydinhalGuardCityPostNight03 does not have any symmetric texture coeff
                    for (size_t j = 0; j < 50/*mNumSymTextureModes*/; ++j)
                        sym += (sRaceTCoeff[j] + (sTCoeff.empty() ? 0.f : sTCoeff[j])) * symTextureModes[50*i + j];

                    // Detail texture is applied after reconstruction of the colour map from the SCM.
                    // Using an average of the 3 colors makes the resulting texture less blotchy. Also see:
                    // "Each such factor is coded as a single unsigned byte in the range [0,255]..."
                    int t = *(pDetail+0) + *(pDetail+1) + *(pDetail+2);
                    float ft = t/192.f; // 64 * 3 = 192

                    int r = *(pDetail+0);
                    float fr = r/64.f;
                    fr = ft;
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        fr = 1.f; // ignore age for now
                    r = std::min(int((*(pDest+0)+sym.x) * fr), 255);
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        r = std::min(int(*(pDetail+0) * 2 *r /255.f), 255);

                    int g = *(pDetail+1);
                    float fg = g/64.f;
                    fg = ft;
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        fg = 1.f;
                    g = std::min(int((*(pDest+1)+sym.y) * fg), 255);
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        g = std::min(int(*(pDetail+1) * 2 * g /255.f), 255);

                    int b = *(pDetail+2);
                    float fb = b/64.f;
                    fb = ft;
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        fb = 1.f;
                    b = std::min(int((*(pDest+2)+sym.z) * fb), 255);
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        b = std::min(int(*(pDetail+2) * 2 * b /255.f), 255);

#if 0 // {{{
                    // Verify the byte order for PF_BYTE_RGBA using Ogre::PixelBox::getColourAt()
                    // NOTE: probably only works for this particular host (e.g. little endian)
                    if (mNpc->mEditorId == "UrielSeptim") // COC "ImperialDungeon01"
                    {
                        int ir = (int)i / 256;
                        int ic = i % 256;

                        //      dest, sym 0 0
                        //      198.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      225.000
                        //      198 142 107 225.000
                        //      dest, sym 0 1
                        //      197.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      229.000
                        //      198 142 107 230.000
                        //      dest, sym 0 2
                        //      198.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      237.000
                        //      198 142 107 240.000
                        //      dest, sym 0 3
                        //      197.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      240.000
                        //      198 142 107 243.000
                        //      dest, sym 0 4
                        //      198.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      241.000
                        //      198 142 107 242.000
                        //      dest, sym 0 5
                        // R -> 197.000 63.674 255.000
                        // G -> 142.000 56.210 201.000
                        // B -> 107.000 51.185 163.000
                        // A -> 240.000
                        //      198 142 107 240.000
                        //       ^   ^   ^   ^
                        //       |   |   |   |
                        //       R   G   B   A
                        std::cout << std::fixed << std::setprecision(3)
                            << "dest, sym " << ir << " " << ic << " " << std::endl;
                        std::cout << float(*(pDest+0)) << " " << sym.x << " " << float(r) << std::endl;
                        std::cout << float(*(pDest+1)) << " " << sym.y << " " << float(g) << std::endl;
                        std::cout << float(*(pDest+2)) << " " << sym.z << " " << float(b) << std::endl;
                        std::cout << float(*(pDest+3)) << std::endl;
                        Ogre::ColourValue cv = pixelBox.getColourAt(ir, ic, 0); // [0.0..1.0]
                        cv *= 255;                                              // [0..255]
                        std::cout << std::dec <<
                            (int)cv.r << " " << int(cv.g) << " " << int(cv.b) << " " << cv.a << std::endl;
                    }

                    if (mNpc->mEditorId == "UrielSeptim") // press <F12> to get a screenshot
                    {
                        std::cout << std::fixed << std::setprecision(3) << "detail " << i << " | " <<
                            *(pDetail+0)/64.f << " " <<
                            *(pDetail+1)/64.f << " " <<
                            *(pDetail+2)/64.f << " " <<
                            *(pDetail+3)/1.f << std::endl;
                    }
#endif // }}}

                    *(pDest+0) = r;
                    *(pDest+1) = g;
                    *(pDest+2) = b;
                    pDest += 4;
                    pDetail += 4;
                }

                // Unlock the pixel buffers
                pixelBufferSrc->unlock();
                pixelBuffer->unlock();
                pixelBuffer2->unlock();

                pass->removeTextureUnitState(0);
                Ogre::TextureUnitState *newTUS = pass->createTextureUnitState("FaceGen"+mNpc->mEditorId);
#else
#    if 1
                FgLib::FgFile<FgLib::FgEgt> egtFile;
                const FgLib::FgEgt *egt = egtFile.getOrLoadByName(meshName);

                if (egt == nullptr)
                    return; // FIXME: throw?

                // try to regtrieve previously created morph texture
                Ogre::TexturePtr morphTexture = Ogre::TextureManager::getSingleton().getByName(
                       mNpc->mEditorId+"_"+textureName,
                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (!morphTexture)
                {
                    // create a blank one
                    morphTexture = Ogre::TextureManager::getSingleton().createManual(
                        mNpc->mEditorId+"_"+textureName, // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,  // type
                        egt->numRows(), egt->numColumns(), // width & height
                        0,                  // number of mipmaps; FIXME: should be 2? or 1?
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                            // textures updated very often (e.g. each frame)
                }

                // we need the base texture
                Ogre::ResourcePtr baseTexture = Ogre::TextureManager::getSingleton().createOrRetrieve(
                       textureName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
                if (!baseTexture)
                    return; // FIXME: throw?

                // dest: usually 256x256 (egt.numRows()*egt.numColumns())
                Ogre::HardwarePixelBufferSharedPtr pixelBuffer = morphTexture->getBuffer();
                pixelBuffer->unlock(); // prepare for blit()
                // src: can be 128x128
                //Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc = tus->_getTexturePtr()->getBuffer();
                Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc
                    = Ogre::static_pointer_cast<Ogre::Texture>(baseTexture)->getBuffer();
                pixelBufferSrc->unlock(); // prepare for blit()
                // if source and destination dimensions don't match, scaling is done
                pixelBuffer->blit(pixelBufferSrc);

                pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
                uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

                uint8_t *pAge;
                Ogre::HardwarePixelBufferSharedPtr pixelBufferAge;
                if (hasAgedTexture)
                {
                    // the age detail modulation texture
                    Ogre::TexturePtr ageTexture = Ogre::TextureManager::getSingleton().getByName(
                           mNpc->mEditorId+"_"+ageTextureFile,
                           Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    if (!ageTexture)
                    {
                        ageTexture = Ogre::TextureManager::getSingleton().createManual(
                            mNpc->mEditorId+"_"+ageTextureFile,
                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                            Ogre::TEX_TYPE_2D,
                            egt->numRows(), egt->numColumns(),
                            0,
                            Ogre::PF_BYTE_RGBA,
                            Ogre::TU_DEFAULT);
                    }

                    // we need the age texture src
                    Ogre::ResourcePtr ageTextureSrc = Ogre::TextureManager::getSingleton().createOrRetrieve(
                           ageTextureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
                    if (!ageTextureSrc)
                        return; // FIXME: throw?

                    // age dest:
                    pixelBufferAge = ageTexture->getBuffer();
                    pixelBufferAge->unlock(); // prepare for blit()
                    // age src:
                    Ogre::HardwarePixelBufferSharedPtr pixelBufferAgeSrc
                        = Ogre::static_pointer_cast<Ogre::Texture>(ageTextureSrc)->getBuffer();
                    //if (!pixelBufferAgeSrc)
                        //std::cout << "detail texture null" << std::endl;
                    pixelBufferAgeSrc->unlock(); // prepare for blit()
                    // if source and destination dimensions don't match, scaling is done
                    pixelBufferAge->blit(pixelBufferAgeSrc); // FIXME: can't we just use the src?

                    pixelBufferAge->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                    const Ogre::PixelBox& pixelBoxAge = pixelBufferAge->getCurrentLock();
                    pAge = static_cast<uint8_t*>(pixelBoxAge.data);
                    if (!pAge)
                        std::cout << "null age detail" << std::endl;
                }

                // Lock the pixel buffer and get a pixel box
                //pixelBufferSrc->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                //const Ogre::PixelBox& pixelBoxSrc = pixelBufferSrc->getCurrentLock();

                uint8_t* pDetail;
                Ogre::HardwarePixelBufferSharedPtr pixelBufferDetail;
                if (!faceDetailFile.empty())
                {
                    Ogre::TexturePtr detailTexture = Ogre::TextureManager::getSingleton().getByName(
                           mNpc->mEditorId+"_"+faceDetailFile,
                           Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    if (!detailTexture)
                    {
                        detailTexture = Ogre::TextureManager::getSingleton().createManual(
                            mNpc->mEditorId+"_"+faceDetailFile,
                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                            Ogre::TEX_TYPE_2D,
                            egt->numRows(), egt->numColumns(),
                            0,
                            Ogre::PF_BYTE_RGBA,
                            Ogre::TU_DEFAULT);
                    }
                    // FIXME: this one should be passed to a shader, along with the "_1" variant
                    Ogre::TexturePtr faceDetailTexture = Ogre::TextureManager::getSingleton().getByName(
                            faceDetailFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    if (faceDetailTexture.isNull())
                    {
                        faceDetailTexture = Ogre::TextureManager::getSingleton().create(
                            faceDetailFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                        faceDetailTexture->load();
                    }
                    pixelBufferDetail = detailTexture->getBuffer();
                    pixelBufferDetail->unlock(); // prepare for blit()
                    Ogre::HardwarePixelBufferSharedPtr pixelBufferDetailSrc
                        = Ogre::static_pointer_cast<Ogre::Texture>(faceDetailTexture)->getBuffer();
                    pixelBufferDetailSrc->unlock(); // prepare for blit()
                    // if source and destination dimensions don't match, scaling is done
                    pixelBufferDetail->blit(pixelBufferDetailSrc); // FIXME: can't we just use the src?

                    pixelBufferDetail->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                    const Ogre::PixelBox& pixelBoxDetail = pixelBufferDetail->getCurrentLock();
                    pDetail = static_cast<uint8_t*>(pixelBoxDetail.data);
                }

                Ogre::Vector3 sym;
                const std::vector<Ogre::Vector3>& symTextureModes = egt->symTextureModes();

                // update the pixels with SCM and detail texture
                // NOTE: mSymTextureModes is assumed to have the image pixels in row-major order
                for (size_t i = 0; i < egt->numRows()*egt->numColumns(); ++i) // height*width, should be 256*256
                {
                    // FIXME: for some reason adding the race coefficients makes it look worse
                    //        even though it is clear that for shapes they are needed
                    // sum all the symmetric texture modes for a given pixel i
                    sym = Ogre::Vector3::ZERO; // WARN: sym reused
                    if (sTCoeff.empty())
                    {
                        // CheydinhalGuardCityPostNight03 does not have any symmetric texture coeff
                        for (size_t j = 0; j < 50/*mNumSymTextureModes*/; ++j)
                            sym += sRaceTCoeff[j] * symTextureModes[50/*mNumSymTextureModes*/ * i + j];
                    }
                    else
                    {
                        for (size_t j = 0; j < 50/*mNumSymTextureModes*/; ++j)
                            sym += (sRaceTCoeff[j] + sTCoeff[j]) * symTextureModes[50/*mNumSymTextureModes*/ * i + j];
                    }

                    float ar, ag, ab;
                    if (hasAgedTexture)
                    {
                        // Detail texture is applied after reconstruction of the colour map from the SCM.
                        // Using an average of the 3 colors makes the resulting texture less blotchy. Also see:
                        // "Each such factor is coded as a single unsigned byte in the range [0,255]..."
                        int r = *(pAge+0);
                        int g = *(pAge+1);
                        int b = *(pAge+2);
#if 0
                        ar = r/64.f;
                        ag = g/64.f;
                        ab = b/64.f;
#else
                        int t = *(pAge+0) + *(pAge+1) + *(pAge+2);
                        float ft = t/192.f; // 64 * 3 = 192
                        ar = ft; // use average
                        ag = ft; // use average
                        ab = ft; // use average
#endif
                        //ar = 1.f; // ignore age for now
                        //ag = 1.f; // ignore age for now
                        //ab = 1.f; // ignore age for now

                        pAge += 4;
                    }
                    else
                    {
                        ar = 1.f;
                        ag = 1.f;
                        ab = 1.f;
                    }

                    float dr, dg, db;
                    if (!faceDetailFile.empty())
                    {
                        dr = 2.f * *(pDetail+0)/255.f;
                        dg = 2.f * *(pDetail+1)/255.f;
                        db = 2.f * *(pDetail+2)/255.f;

                        pDetail += 4;
                    }
                    else
                    {
                        dr = 1.f;
                        dg = 1.f;
                        db = 1.f;
                    }

#if 0
                    *(pDest+0) = std::min(int((*(pDest+0)+sym.x) * fr), 255);
                    *(pDest+1) = std::min(int((*(pDest+1)+sym.y) * fg), 255);
                    *(pDest+2) = std::min(int((*(pDest+2)+sym.z) * fb), 255);
#else
                    *(pDest+0) = std::min(int(std::min(int((*(pDest+0)+sym.x)), 255) * ar * dr), 255);
                    *(pDest+1) = std::min(int(std::min(int((*(pDest+1)+sym.y)), 255) * ag * dg), 255);
                    *(pDest+2) = std::min(int(std::min(int((*(pDest+2)+sym.z)), 255) * ab * db), 255);
#endif
                    pDest += 4;
                }

                // Unlock the pixel buffers
                pixelBuffer->unlock();
                if (hasAgedTexture)
                    pixelBufferAge->unlock();
                if (!faceDetailFile.empty())
                    pixelBufferDetail->unlock();
#     endif
                pass->removeTextureUnitState(0);
                Ogre::TextureUnitState *newTUS = pass->createTextureUnitState(mNpc->mEditorId+"_"+textureName);
#endif

            } // while pass
        } // while technique
        }

        it->second->shareSkeletonInstanceWith(mSkelBase);
        mInsert->attachObject(it->second);
    }
    mHeadParts.push_back(scene);

#if 0
        meshName = "meshes\\clutter\\food\\apple01.nif";
        NifOgre::ObjectScenePtr sceneA = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        std::auto_ptr<NiBtOgre::BtOgreInst> instA(new NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), sceneA, meshName, group));
        instA->instantiate(mSkelBase->getMesh()->getSkeleton());

        //Ogre::Bone *ABone = mSkelBase->getSkeleton()->getBone("Bip01 L Hand");
        mSkelBase->attachObjectToBone("Bip01 L ForeTwist", sceneA->mForeignObj->mEntities[0]);
        mObjectParts[ESM::PRT_RKnee] = sceneA;

        meshName = "meshes\\clutter\\food\\pear01.nif";
        NifOgre::ObjectScenePtr sceneP = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        std::auto_ptr<NiBtOgre::BtOgreInst> instP(new NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), sceneP, meshName, group));
        instP->instantiate(mSkelBase->getMesh()->getSkeleton());

        //Ogre::Bone *ABone = mSkelBase->getSkeleton()->getBone("Bip01 L Hand");
        mSkelBase->attachObjectToBone("Bip01 R ForeTwist", sceneP->mForeignObj->mEntities[0]);
        mObjectParts[ESM::PRT_LKnee] = sceneP;
#endif

    //for(size_t i = 0;i < ESM::PRT_Count;i++)
        //removeIndividualPart((ESM::PartReferenceType)i);
    //updateParts();

    // FIXME: this section below should go to updateParts()
    std::vector<const ESM4::Clothing*> invCloth;
    std::vector<const ESM4::Armor*> invArmor;
    std::vector<const ESM4::Weapon*> invWeap;

    // check inventory
    //MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    for(size_t i = 0; i < 35; ++i) // FIXME: 16 slots for TES4
    {
        MWWorld::ContainerStoreIterator store = inv.getSlot(i);

        if(store == inv.end())
            continue;

        // TODO: this method of handling inventory doen't suit TES4 very well because it is possible
        //       for one part to occupy more than one slot; for now as a workaround just loop
        //       through the slots to gather all the equipped parts and process them afterwards

        if(store->getTypeName() == typeid(ESM4::Clothing).name())
        {
            const ESM4::Clothing *cloth = store->get<ESM4::Clothing>()->mBase;
            if (std::find(invCloth.begin(), invCloth.end(), cloth) == invCloth.end())
                invCloth.push_back(cloth);
        }
        else if(store->getTypeName() == typeid(ESM4::Armor).name())
        {
            const ESM4::Armor *armor = store->get<ESM4::Armor>()->mBase;
            if (std::find(invArmor.begin(), invArmor.end(), armor) == invArmor.end())
                invArmor.push_back(armor);
        }
        else if(store->getTypeName() == typeid(ESM4::Weapon).name())
        {
            const ESM4::Weapon *weap = store->get<ESM4::Weapon>()->mBase;
            if (std::find(invWeap.begin(), invWeap.end(), weap) == invWeap.end())
                invWeap.push_back(weap);
        }
    }

    for (std::size_t i = 0; i < invCloth.size(); ++i)
        equipClothes(invCloth[i], isFemale);

    for (std::size_t i = 0; i < invArmor.size(); ++i)
        equipArmor(invArmor[i], isFemale);

    for (std::size_t i = 0; i < invWeap.size(); ++i)
    {
        std::string meshName;

        meshName = "meshes\\"+invWeap[i]->mModel;

        int type = ESM4::Armor::TES4_Weapon;

        removeIndividualPart((ESM::PartReferenceType)type);

        // FIXME: group "General"
        // FIXME: prob wrap this with a try/catch block
        mObjectParts[type] =
                createObject(meshName, "General", mObjectRoot->mForeignObj->mModel);
    }

    //if (mAccumRoot) mAccumRoot->setPosition(Ogre::Vector3());

    mWeaponAnimationTime->updateStartTime();
}

// needs: mNpc, mRace
std::string ForeignNpcAnimation::getSkeletonModel(const MWWorld::ESMStore& store) const
{
    std::string skeletonModel;

    if (mNpc->mModel.empty() && mNpc->mBaseTemplate != 0) // TES5
    {
        uint32_t type = store.find(mNpc->mBaseTemplate);

        if (type == MKTAG('_', 'N', 'P', 'C'))
        {
            if ((mNpc->mActorBaseConfig.flags & 0x1) != 0) // female
                return  "meshes\\" + mRace->mModelFemale; // TODO: check if this can be empty
            else
                return "meshes\\" + mRace->mModelMale;
        }
        else if (type == MKTAG('N', 'L', 'V', 'L'))
        {
            const ESM4::LeveledActor* lvlActor
                = store.getForeign<ESM4::LeveledActor>().search(mNpc->mBaseTemplate);

            std::cout <<  "TES5 LVLN: " << mNpc->mEditorId << ","
                      << lvlActor->mEditorId << "," << lvlActor->mModel << std::endl;

            return "meshes\\" + lvlActor->mModel;
        }
        else
            throw std::runtime_error(mNpc->mEditorId + " TES5 NPC unknown BaseTemplate type");

    }
    else if (!mNpc->mModel.empty()) // TES4
    {
        // Characters\_Male\skeleton.nif
        // Characters\_Male\skeletonbeast.nif
        // Characters\_Male\skeletonsesheogorath.nif
        return "meshes\\" + mNpc->mModel;
    }
    else
        return ""; // shouldn't happen
}

NifOgre::ObjectScenePtr ForeignNpcAnimation::createSkinnedObject(NifOgre::ObjectScenePtr scene,
        const std::string& meshName, const std::string& group, NiModelPtr skeletonModel)
{
    // get or create a skinned model with this NPC's skeleton
    std::string skeletonName = skeletonModel->getName();

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    NiModelPtr object = modelManager.getByName(skeletonName + "_" + meshName, group);
    if (!object)
        object = modelManager.createSkinnedModel(meshName, group, skeletonModel.get(),""/*FIXME*/);

    // create an instance of the model
    scene->mForeignObj
        = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(object, mInsert->createChildSceneNode()));

    scene->mForeignObj->instantiateBodyPart(mInsert, mSkelBase);

    return scene;
}

// uses mNpc, mRace, mInsert and mSkelBase
// NOTE: Some Helmet, Hood and Hair are not skinned (mostly not)
//       Skinned models no not return a target bone (e.g. Clothes\RobeMage\M\Hood.NIF)
NifOgre::ObjectScenePtr ForeignNpcAnimation::createMorphedObject(const std::string& meshName,
        const std::string& group, NiModelPtr skeletonModel, const std::string& texture)
{
    // FIXME: probably needs a try/catch block here

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();

#if 1
    NiModelPtr object = modelManager.getByName(mNpc->mEditorId + "_" + meshName, group);
    if (!object)
        object = modelManager.createMorphedModel(meshName, group, mNpc, mRace, skeletonModel.get(),
                                                 texture, NiBtOgre::NiModelManager::BP_Mouth); // FIXME
#else
    // initially assume a morphed model
    NiModelPtr object = modelManager.getByName(npc->mEditorId + "_" + meshName, group);
    // if not found just create a non-skinned model to check
    if (!object)
    {
        object = modelManager.getOrLoadByName(meshName, group);
        if (object->buildData().mIsSkinned)
        {
            // skinned, so go ahead and create a morphed model
            object.reset();
            object = modelManager.createMorphedModel(meshName, group, mNpc, mRace, skeletonModel.get(), texture);
        }
    }
#endif

    // create an instance of the model
    NifOgre::ObjectScenePtr scene
        = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));

    std::string targetBone = object->getTargetBone();

    // create an instance of the model
    scene->mForeignObj
        = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(object, mInsert->createChildSceneNode()));

    if (object->buildData().mIsSkinned)
    {
        scene->mForeignObj->instantiateBodyPart(mInsert, mSkelBase);
    }
    else // attach to bone
    {
        scene->mForeignObj->instantiate();

        if (targetBone == "") // FIXME: hack
            targetBone = "Bip01 Head";

        Ogre::Bone *bone = mSkelBase->getSkeleton()->getBone(targetBone);

        // fix helmet issue
        Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        Ogre::Vector3 position = Ogre::Vector3::ZERO;


        if (targetBone == "Bip01 Head")
        {
            // some models already have the rotation in their base node :-(
            const Ogre::Quaternion baseRotation = scene->mForeignObj->mModel->getBaseRotation();
            if (baseRotation == Ogre::Quaternion::IDENTITY)
                orientation = bone->getOrientation() * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y);

            position = Ogre::Vector3(0.45f/*up*/, -0.55f/*forward*/, 0.f/*right*/);
        }

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
            mSkelBase->attachObjectToBone(targetBone, it->second, orientation, position);
    }

    return scene;
}

// uses mRace, mInsert, mSkelbase, mObjectRoot (or skeletonModel)
//
// NOTE: We make an assumption that if texture is not empty it must be for a particular race.
//       To make this work we need to add mRace->mEditorId to the model name, otherwise the same
//       object for different race, e.g. hands, can't be distinguished.
//
//       The object may or may not be skinned (head, clothes are skinned while eyes, ears, hair are
//       not skinned).  That means race name needs to be added in addition to the skeleton name.
NifOgre::ObjectScenePtr ForeignNpcAnimation::createObject(const std::string& meshName,
        const std::string& group, NiModelPtr skeletonModel, const std::string& raceTexture)
{
    // FIXME: probably needs a try/catch block here

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();

    std::string raceName = "";
    if (!raceTexture.empty())
        raceName = mRace->mEditorId + "$";

    // initially assume a skinned model
    std::string skeletonName = skeletonModel->getName();
    Misc::StringUtils::lowerCaseInPlace(skeletonName);
    NiModelPtr model = modelManager.getByName(raceName + skeletonName + "_" + meshName, group);

    // if not found just create a non-skinned model to check
    if (!model)
    {
        // create a vanilla model to test
        model = modelManager.getOrLoadByName(meshName, group);

        if (model->buildData().mIsSkinned)
        {
            // was skinned after all
            model.reset();
            model = modelManager.createSkinnedModel(meshName, group, skeletonModel.get(), raceName, raceTexture);
        }
        else if (!raceName.empty())
        {
            // not skinned but still need a race variant (e.g. hands)
            model.reset();
            model = modelManager.createManualModel(meshName, group, raceName, raceTexture);
        }
    }

    // create an instance of the model
    NifOgre::ObjectScenePtr scene
        = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));

    //if (type == 0x02) // TODO: check for PRT_Head as well?
    //{
    //    return false; // special handling for morphed model
    //}
#if 0
    else if(model->buildData().mIsSkinned) // test if the model is skinned
    {
        mObjectParts[type]
            = createSkinnedObject(scene, meshName, group, mObjectRoot->mForeignObj->mModel);

        return true;
    }
    else // some objects, e.g. a Shield, is not skinned
    {
        std::string targetBone = model->targetBone();

        if (targetBone == "")
            //return false;
            throw std::runtime_error("createObject: No target bone to attach part");

#endif
        scene->mForeignObj
            = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(model, mInsert->createChildSceneNode()));
        scene->mForeignObj->instantiateBodyPart(mInsert, mSkelBase);
#if 0
        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
            mSkelBase->attachObjectToBone(targetBone, it->second); // TODO: do these need cleanup later?
#endif

    return scene;
}

bool ForeignNpcAnimation::equipClothes(const ESM4::Clothing* cloth, bool isFemale)
{
    std::string meshName;

    if (isFemale && !cloth->mModelFemale.empty())
        meshName = "meshes\\" + cloth->mModelFemale;
    else
        meshName = "meshes\\" + cloth->mModelMale;

    // CLOT only in TES4
    int type = cloth->mClothingFlags & 0xffff; // remove general flags high bits

    // Some clothes show exposed skin - these textures need to be repladed with appropriate textures
    // from the RACE records.  Unfortunately it is not possible to know in advance whether a replace
    // ment will be required.
    //
    // An alternative solution would be to get the texture while building the material.  To do so
    // we'll need to supply a pointer to mRace.  This is not ideal since some clothes may be shared
    // across many different races.
    //
    // NOTE: Assumed that the default textures in the NIF files are in textures\characters\imperial
    //
    // FIXME: only support TES4 for now
    // FIXME: not sure if there can be more than one, e.g. upper body *and* hands
    const std::vector<ESM4::Race::BodyPart>& bodyParts
        = (isFemale ? mRace->mBodyPartsFemale : mRace->mBodyPartsMale);
    int index = -1;
    std::string raceTexture = "";
    if ((cloth->mClothingFlags & ESM4::Armor::TES4_UpperBody) != 0)
        index = ESM4::Race::UpperBody;
    else if ((cloth->mClothingFlags & ESM4::Armor::TES4_LowerBody) != 0)
        index = ESM4::Race::LowerBody;
    else if ((cloth->mClothingFlags & ESM4::Armor::TES4_Hands) != 0)
        index = ESM4::Race::Hands;
    else if ((cloth->mClothingFlags & ESM4::Armor::TES4_Feet) != 0)
        index = ESM4::Race::Feet;

    if (index != -1)
        raceTexture = "textures\\" + bodyParts[index].texture;

    removeParts((ESM::PartReferenceType)type);
    //removeIndividualPart((ESM::PartReferenceType)type);

    // FIXME: group "General"
    if ((cloth->mClothingFlags & ESM4::Armor::TES4_Hair) != 0) // Hair slot, e.g. hoods
    {
        mObjectParts[type] =
            createMorphedObject(meshName, "General", mObjectRoot->mForeignObj->mModel);
    }
    else if (index == ESM4::Race::UpperBody || index == ESM4::Race::LowerBody)
    {
        NifOgre::ObjectScenePtr scene =
                createObject(meshName, "General", mObjectRoot->mForeignObj->mModel);

        std::string npcTextureName;
        if (index = ESM4::Race::UpperBody)
            npcTextureName = mTextureUpperBody->getName();
        else if (index = ESM4::Race::LowerBody)
            npcTextureName = mTextureLowerBody->getName();

        replaceSkinTexture(scene, npcTextureName); // does nothing if none found

        mObjectParts[type] = scene;
    }
    else
        mObjectParts[type] =
            createObject(meshName, "General", mObjectRoot->mForeignObj->mModel, raceTexture);


    return true;
}

bool ForeignNpcAnimation::equipArmor(const ESM4::Armor* armor, bool isFemale)
{
    std::string meshName;

    if (isFemale && !armor->mModelFemale.empty())
        meshName = "meshes\\" + armor->mModelFemale;
    else
        meshName = "meshes\\" + armor->mModelMale;

    int type = armor->mArmorFlags;

    if ((armor->mGeneralFlags & ESM4::Armor::TYPE_TES4) != 0)
        type = armor->mArmorFlags & 0xffff; // remove general flags high bits

    const std::vector<ESM4::Race::BodyPart>& bodyParts
        = (isFemale ? mRace->mBodyPartsFemale : mRace->mBodyPartsMale);
    int index = -1;
    std::string raceTexture = "";
    if ((armor->mArmorFlags & ESM4::Armor::TES4_UpperBody) != 0)
        index = ESM4::Race::UpperBody;
    else if ((armor->mArmorFlags & ESM4::Armor::TES4_LowerBody) != 0)
        index = ESM4::Race::LowerBody;
    else if ((armor->mArmorFlags & ESM4::Armor::TES4_Hands) != 0)
        index = ESM4::Race::Hands;
    else if ((armor->mArmorFlags & ESM4::Armor::TES4_Feet) != 0)
        index = ESM4::Race::Feet;

    if (index != -1)
        raceTexture = "textures\\" + bodyParts[index].texture;

    removeParts((ESM::PartReferenceType)type);
    //removeIndividualPart((ESM::PartReferenceType)type);

    // FIXME: group "General"
    if ((armor->mArmorFlags & ESM4::Armor::TES4_Hair) != 0) // Hair slot
    {
        mObjectParts[type] =
            createMorphedObject(meshName, "General", mObjectRoot->mForeignObj->mModel);
    }
    else if (index == ESM4::Race::UpperBody || index == ESM4::Race::LowerBody)
    {
        NifOgre::ObjectScenePtr scene =
                createObject(meshName, "General", mObjectRoot->mForeignObj->mModel);

        std::string npcTextureName;
        if (index = ESM4::Race::UpperBody)
            npcTextureName = mTextureUpperBody->getName();
        else if (index = ESM4::Race::LowerBody)
            npcTextureName = mTextureLowerBody->getName();

        replaceSkinTexture(scene, npcTextureName); // does nothing if none found

        mObjectParts[type] = scene;
    }
    else // hands, feet
        mObjectParts[type] =
            createObject(meshName, "General", mObjectRoot->mForeignObj->mModel, raceTexture);


    return true;
}

void ForeignNpcAnimation::replaceMeshTexture(NifOgre::ObjectScenePtr scene, const std::string& npcTextureName)
{
    std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(it->second);
        Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
        while(techIter.hasMoreElements())
        {
            Ogre::Technique *tech = techIter.getNext();
            Ogre::Technique::PassIterator passes = tech->getPassIterator();
            while(passes.hasMoreElements())
            {
                Ogre::Pass *pass = passes.getNext();
                pass->removeTextureUnitState(0);
                Ogre::TextureUnitState *newTUS = pass->createTextureUnitState(npcTextureName);
            }
        }
    }
}

void ForeignNpcAnimation::replaceSkinTexture(NifOgre::ObjectScenePtr scene, const std::string& npcTextureName)
{
    std::map<std::string, std::vector<std::size_t> > visibleSkinMap;
    scene->mForeignObj->mModel->fillSkinIndicies(visibleSkinMap);

    if (visibleSkinMap.empty())
        return;

    // assumed that there is only one mesh with visible skin sub-meshes
    std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        // find the coresponding NiNode
        std::string meshName = it->second->getMesh()->getName();
        std::size_t pos = meshName.find_last_of('@');
        if (pos == std::string::npos)
            continue; // shouldn't happen, throw?

        std::string nodeName = meshName.substr(pos+1);
        std::map<std::string, std::vector<std::size_t> >::iterator lb = visibleSkinMap.lower_bound(nodeName);
        if (lb != visibleSkinMap.end() && !(visibleSkinMap.key_comp()(nodeName, lb->first)))
        {
            const std::vector<std::size_t>& skinIndicies = lb->second;
            for (std::size_t i = 0; i < skinIndicies.size(); ++i)
            {
                Ogre::MaterialPtr mat = createClonedMaterials(it->second->getSubEntity(skinIndicies[i]));

                Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
                while(techIter.hasMoreElements())
                {
                    Ogre::Technique *tech = techIter.getNext();
                    Ogre::Technique::PassIterator passes = tech->getPassIterator();
                    while(passes.hasMoreElements())
                    {
                        Ogre::Pass *pass = passes.getNext();
                        pass->removeTextureUnitState(0); // hopefully this is the correct one
                        Ogre::TextureUnitState *newTUS = pass->createTextureUnitState(npcTextureName);
                    }
                }
            }
        }
        else // None found
        {
            continue; // shouldn't happen, throw?
        }
    }
}

Ogre::MaterialPtr ForeignNpcAnimation::createClonedMaterials(Ogre::SubEntity *subEntity)
{
    if (mClonedMaterials.find(subEntity) != mClonedMaterials.end())
    {
        return mClonedMaterials[subEntity];
    }
    else
    {
        Ogre::MaterialPtr mat = subEntity->getMaterial();

        static int count = 0;
        Ogre::String newName = mat->getName() + Ogre::StringConverter::toString(count++);
        sh::Factory::getInstance().createMaterialInstance(newName, mat->getName());

        // Make sure techniques are created
        sh::Factory::getInstance()._ensureMaterial(newName, "Default");
        mat = Ogre::MaterialManager::getSingleton().getByName(newName);

        mClonedMaterials[subEntity] = mat;
        subEntity->setMaterial(mat);

        return mat;
    }
}

void ForeignNpcAnimation::deleteClonedMaterials()
{
    for (std::map<Ogre::SubEntity*, Ogre::MaterialPtr>::iterator it
            = mClonedMaterials.begin(); it != mClonedMaterials.end(); ++it)
    {
        sh::Factory::getInstance().destroyMaterialInstance(it->second->getName());
    }
}

void ForeignNpcAnimation::addAnimSource(const std::string &model)
{
    OgreAssert(mInsert, "Object is missing a root!");
    if (!mSkelBase)
        return; // FIXME: should throw here (or assert)

    // First find the kf file.  For TES3 the kf file has the same name as the nif file.
    // For TES4, different animations (e.g. idle, block) have different kf files.
    size_t pos = model.find("skeleton.nif");
    if (pos == std::string::npos)
    {
        pos = model.find("skeletonbeast.nif");
        if (pos == std::string::npos)
            return; // FIXME: should throw here
        // TODO: skeletonsesheogorath
    }

    // FIXME: for testing just load idle
    std::string animName = model.substr(0, pos) + "handtohandattackleft_jab.kf";

    addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "handtohandattackright_hook.kf";
    //addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "blockidle.kf";
    //addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "sneakidle.kf";
    //addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "walkforward.kf";
    //addForeignAnimSource(model, animName);
    animName = model.substr(0, pos) + "twohandidle.kf";
    addForeignAnimSource(model, animName);
    animName = model.substr(0, pos) + "castself.kf";
    addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "swimfastforward.kf";
    //animName = model.substr(0, pos) + "walkfastforward.kf";
    //animName = model.substr(0, pos) + "swimbackward.kf";
    //animName = model.substr(0, pos) + "dodgeleft.kf";
    //animName = model.substr(0, pos) + "idleanims\\cheer01.kf";
    //animName = model.substr(0, pos) + "idleanims\\talk_armscrossed_motion.kf";
    //animName = model.substr(0, pos) + "castselfalt.kf";
    //animName = model.substr(0, pos) + "idleanims\\umpa_disco.kf";
    animName = model.substr(0, pos) + "walkforward.kf";
    animName = model.substr(0, pos) + "idle.kf";
    //
    //animName = model.substr(0, pos) + "idleanims\\talk_relaxed.kf";
    //animName = model.substr(0, pos) + "idleanims\\bdaycheera.kf";
    //animName = model.substr(0, pos) + "idleanims\\laugha.kf";
    //animName = model.substr(0, pos) + "locomotion\\male\\mtforward.kf";
    addForeignAnimSource(model, animName);
}

void ForeignNpcAnimation::addForeignAnimSource(const std::string& model, const std::string &animName)
{
    // Check whether the kf file exists
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(animName))
        return;

    std::string group("General"); // FIXME

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    NiModelPtr skeleton = modelManager.getByName(model, group);
    if (!skeleton)
        skeleton = modelManager.createSkeletonModel(model, group);

    assert(!skeleton.isNull() && "skeleton.nif should have been built already");
    NiModelPtr anim = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(animName, group);

    // Animation::AnimSource : public Ogre::AnimationAlloc
    //   (has a) std::multimap<float, std::string> mTextKeys
    //   (also has a vector of 4 Ogre real controllers)  TODO: check if 4 is enough
    Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
    std::vector<Ogre::Controller<Ogre::Real> > controllers;
    anim->buildAnimation(mSkelBase, anim, animSource->mTextKeys, controllers, /*mObjectRoot->skeleton.get()*/skeleton.get()); // no bow

    if (animSource->mTextKeys.empty() || controllers.empty())
        return;

    mAnimSources.push_back(animSource);

    std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animSource->mControllers;
    for (size_t i = 0; i < controllers.size(); i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[i].getDestination().get());

        size_t grp = detectAnimGroup(dstval->getNode());

        if (!mAccumRoot && grp == 0)
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
            if (!mAccumRoot)
            {
                std::cerr << "Non-Accum root for " << mPtr.getCellRef().getRefId() << " is skeleton root??" << std::endl;
                mNonAccumRoot = NULL;
            }
        }

        if (grp == 0 && (dstval->getNode()->getName() == "Bip01 NonAccum"))
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent(); // should be "Bip01"
            if (!mAccumRoot)
            {
                std::cerr << "Non-Accum root for " << mPtr.getCellRef().getRefId() << " is skeleton root??" << std::endl;
                mNonAccumRoot = NULL;
            }
        }

        controllers[i].setSource(mAnimationTimePtr[grp]);
        grpctrls[grp].push_back(controllers[i]);
    }

    // FIXME: debugging
    NifOgre::NodeTargetValue<Ogre::Real> *dstval;
    dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[0].getDestination().get());
    Ogre::Node *node = dstval->getNode();
    while (node && node->getName() != "Bip01 NonAccum")
    {
        node = node->getParent();
    }
    if (node && node->getName() == "Bip01 NonAccum")
    {
        mNonAccumRoot = node;
        mAccumRoot = mNonAccumRoot->getParent();
    }
    // else throw?

    if (mNonAccumRoot->getName() != "Bip01 NonAccum" || mAccumRoot->getName() != "Bip01")
        std::cout << mAccumRoot->getName() << std::endl;
    // end debugging

    for (unsigned int i = 0; i < mObjectRoot->mControllers.size(); ++i)
    {
        if (!mObjectRoot->mControllers[i].getSource())
            mObjectRoot->mControllers[i].setSource(mAnimationTimePtr[0]);
    }
}

void ForeignNpcAnimation::updateParts()
{
    if (!mSkelBase)
        return;

    //mAlpha = 1.f;
    const MWWorld::Class &cls = mPtr.getClass();

    NpcType curType = Type_Normal;
#if 0 // beast types
    if (cls.getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Vampirism).getMagnitude() > 0)
        curType = Type_Vampire;
    if (cls.getNpcStats(mPtr).isWerewolf())
        curType = Type_Werewolf;

    if (curType != mNpcType)
    {
        mNpcType = curType;
        rebuild();
        return;
    }
#endif
    // Equipment Slots
    //
    // Morrowind      Oblivion
    //
    // Robe           -- takes other slots
    // Weapon R       Weapon
    // Weapon L       Shield
    // Helmet         Helmet/Hood
    // Cuirass        Cuirass/Shirt        - upper body
    // Gauntlets      Gauntlets            - hand
    // Greaves        Greaves/Skirts/Pants - lower body
    // Boots/Shoes    Boots/Shoes          - feet
    // Shirt
    // Skirts/Pants
    // Pauldron R     -- doesn't exist
    // Pauldron L     -- doesn't exist
    // Ring           Ring
    // Ring           Ring
    // Amulet         Amulet

    static const struct {
        int mSlot;
        int mBasePriority;
    } slotlist[] = {
        // FIXME: Priority is based on the number of reserved slots. There should be a better way.
        { MWWorld::InventoryStore::Slot_Robe,         12 },
        { MWWorld::InventoryStore::Slot_Skirt,         3 },
        { MWWorld::InventoryStore::Slot_Helmet,        0 },
        { MWWorld::InventoryStore::Slot_Cuirass,       0 },
        { MWWorld::InventoryStore::Slot_Greaves,       0 },
        { MWWorld::InventoryStore::Slot_LeftPauldron,  0 },
        { MWWorld::InventoryStore::Slot_RightPauldron, 0 },
        { MWWorld::InventoryStore::Slot_Boots,         0 },
        { MWWorld::InventoryStore::Slot_LeftGauntlet,  0 },
        { MWWorld::InventoryStore::Slot_RightGauntlet, 0 },
        { MWWorld::InventoryStore::Slot_Shirt,         0 },
        { MWWorld::InventoryStore::Slot_Pants,         0 },
        { MWWorld::InventoryStore::Slot_CarriedLeft,   0 },
        { MWWorld::InventoryStore::Slot_CarriedRight,  0 },
        { MWWorld::InventoryStore::Slot_ForeignHead,      0 },
        { MWWorld::InventoryStore::Slot_ForeignHair,      0 },
        { MWWorld::InventoryStore::Slot_ForeignUpperBody, 0 },
        { MWWorld::InventoryStore::Slot_ForeignLowerBody, 0 },
        { MWWorld::InventoryStore::Slot_ForeignHands,     0 },
        { MWWorld::InventoryStore::Slot_ForeignFeet,      0 },
        { MWWorld::InventoryStore::Slot_ForeignRightRing, 0 },
        { MWWorld::InventoryStore::Slot_ForeignLeftRing,  0 },
        { MWWorld::InventoryStore::Slot_ForeignAmulet,    0 },
        { MWWorld::InventoryStore::Slot_ForeignWeapon,    0 },
        { MWWorld::InventoryStore::Slot_ForeignBackWeapon,0 },
        { MWWorld::InventoryStore::Slot_ForeignSideWeapon,0 },
        { MWWorld::InventoryStore::Slot_ForeignQuiver,    0 },
        { MWWorld::InventoryStore::Slot_ForeignShield,    0 },
        { MWWorld::InventoryStore::Slot_ForeignTorch,     0 },
        { MWWorld::InventoryStore::Slot_ForeignTail,      0 }
    };
    static const size_t slotlistsize = sizeof(slotlist)/sizeof(slotlist[0]);

    bool wasArrowAttached = (mAmmunition.get() != NULL);

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    for(size_t i = 0; i < slotlistsize /*&& mViewMode != VM_HeadOnly*/; ++i)
    {
        MWWorld::ContainerStoreIterator store = inv.getSlot(slotlist[i].mSlot);

        removePartGroup(slotlist[i].mSlot);

        if(store == inv.end())
            continue;

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_ForeignHair)
            removeIndividualPart((ESM::PartReferenceType)ESM4::Armor::TES4_Hair);

        int prio = 1;
        //bool enchantedGlow = !store->getClass().getEnchantment(*store).empty();
        //Ogre::Vector3 glowColor = getEnchantmentColor(*store);
        if(store->getTypeName() == typeid(ESM4::Clothing).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 0;
            const ESM4::Clothing *clothes = store->get<ESM4::Clothing>()->mBase;
            //addPartGroup(slotlist[i].mSlot, prio, clothes->mParts.mParts, enchantedGlow, &glowColor);
        }
        else if(store->getTypeName() == typeid(ESM4::Armor).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 1;
            const ESM4::Armor *armor = store->get<ESM4::Armor>()->mBase;
            //addPartGroup(slotlist[i].mSlot, prio, armor->mParts.mParts, enchantedGlow, &glowColor);
        }

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Robe)
        {
            ESM::PartReferenceType parts[] = {
                ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee,
                ESM::PRT_RForearm, ESM::PRT_LForearm
            };
            size_t parts_size = sizeof(parts)/sizeof(parts[0]);
            for(size_t p = 0; p < parts_size; ++p)
                reserveIndividualPart(parts[p], slotlist[i].mSlot, prio);
        }
        else if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Skirt)
        {
            reserveIndividualPart(ESM::PRT_Groin, slotlist[i].mSlot, prio);
            reserveIndividualPart(ESM::PRT_RLeg, slotlist[i].mSlot, prio);
            reserveIndividualPart(ESM::PRT_LLeg, slotlist[i].mSlot, prio);
        }
    }

//  if(mViewMode != VM_FirstPerson)
//  {
//      if(mPartPriorities[ESM::PRT_Head] < 1 && !mHeadModel.empty())
//          addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
//      if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1 && !mHairModel.empty())
//          addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);
//  }
//  if(mViewMode == VM_HeadOnly)
//      return;

    if(mPartPriorities[ESM::PRT_Shield] < 1)
    {
        MWWorld::ContainerStoreIterator store = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        MWWorld::Ptr part;
        if(store != inv.end() && (part=*store).getTypeName() == typeid(ESM4::Light).name())
        {
            const ESM::Light *light = part.get<ESM::Light>()->mBase;
            addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft,
                                       1, "meshes\\" + light->mModel);
            addExtraLight(mInsert->getCreator(), mObjectParts[ESM::PRT_Shield], light);
        }
    }

//  showWeapons(mShowWeapons);
//  showCarriedLeft(mShowCarriedLeft);
#if 0
    // Remember body parts so we only have to search through the store once for each race/gender/viewmode combination
    static std::map< std::pair<std::string,int>,std::vector<const ESM::BodyPart*> > sRaceMapping;

    bool isWerewolf = (mNpcType == Type_Werewolf);
    int flags = (isWerewolf ? -1 : 0);
    bool isMale = (mNpc->mBaseConfig.flags & 0x000001) == 0; // 0x1 means female
    if (isMale)
    {
        static const int Flag_Female      = 1<<0;
        flags |= Flag_Female;
    }

    if(mViewMode == VM_FirstPerson)
    {
        static const int Flag_FirstPerson = 1<<1;
        flags |= Flag_FirstPerson;
    }

    std::string race = (isWerewolf ? "werewolf" : Misc::StringUtils::lowerCase(mNpc->mRace));
    std::pair<std::string, int> thisCombination = std::make_pair(race, flags);
    if (sRaceMapping.find(thisCombination) == sRaceMapping.end()) // init if thisCombination not found
    {
        typedef std::multimap<ESM::BodyPart::MeshPart,ESM::PartReferenceType> BodyPartMapType;
        static BodyPartMapType sBodyPartMap;
        if(sBodyPartMap.empty())
        {
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Neck, ESM::PRT_Neck));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Chest, ESM::PRT_Cuirass));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Groin, ESM::PRT_Groin));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Hand, ESM::PRT_RHand));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Hand, ESM::PRT_LHand));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Wrist, ESM::PRT_RWrist));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Wrist, ESM::PRT_LWrist));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Forearm, ESM::PRT_RForearm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Forearm, ESM::PRT_LForearm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperarm, ESM::PRT_RUpperarm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperarm, ESM::PRT_LUpperarm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Foot, ESM::PRT_RFoot));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Foot, ESM::PRT_LFoot));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Ankle, ESM::PRT_RAnkle));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Ankle, ESM::PRT_LAnkle));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Knee, ESM::PRT_RKnee));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Knee, ESM::PRT_LKnee));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperleg, ESM::PRT_RLeg));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperleg, ESM::PRT_LLeg));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Tail, ESM::PRT_Tail));
        }

        std::vector<const ESM::BodyPart*> &parts = sRaceMapping[thisCombination];
        parts.resize(ESM::PRT_Count, NULL);

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();
        for(MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            if(isWerewolf)
                break;
            const ESM::BodyPart& bodypart = *it;
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;

            if (!Misc::StringUtils::ciEqual(bodypart.mRace, ""/*mNpc->mRace*/))
                continue;

            bool firstPerson = (bodypart.mId.size() >= 3)
                    && bodypart.mId[bodypart.mId.size()-3] == '1'
                    && bodypart.mId[bodypart.mId.size()-2] == 's'
                    && bodypart.mId[bodypart.mId.size()-1] == 't';
            if(firstPerson != (mViewMode == VM_FirstPerson))
            {
                if(mViewMode == VM_FirstPerson && (bodypart.mData.mPart == ESM::BodyPart::MP_Hand ||
                                                   bodypart.mData.mPart == ESM::BodyPart::MP_Wrist ||
                                                   bodypart.mData.mPart == ESM::BodyPart::MP_Forearm ||
                                                   bodypart.mData.mPart == ESM::BodyPart::MP_Upperarm))
                {
                    /* Allow 3rd person skins as a fallback for the arms if 1st person is missing. */
                    BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                    while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                    {
                        if(!parts[bIt->second])
                            parts[bIt->second] = &*it;
                        ++bIt;
                    }
                }
                continue;
            }

            bool isFemale = (mNpc->mBaseConfig.flags & 0x000001) != 0; // 0x1 means female
            if (isFemale != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
            {
                // Allow opposite gender's parts as fallback if parts for our gender are missing
                BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                {
                    if(!parts[bIt->second])
                        parts[bIt->second] = &*it;
                    ++bIt;
                }
                continue;
            }

            BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
            while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
            {
                parts[bIt->second] = &*it;
                ++bIt;
            }
        }
    }

    // use thisCombination get the mesh
    const std::vector<const ESM::BodyPart*> &parts = sRaceMapping[thisCombination];
    for(int part = ESM::PRT_Neck; part < ESM::PRT_Count; ++part)
    {
        if(mPartPriorities[part] < 1)
        {
            const ESM::BodyPart* bodypart = parts[part];
            if(bodypart)
                addOrReplaceIndividualPart((ESM::PartReferenceType)part, -1, 1,
                                           "meshes\\"+bodypart->mModel);
        }
    }
#endif
    if (wasArrowAttached)
        attachArrow();
}

// TES4 animation is rather different, so a different implementation is required
//
// FIXME: Initially groupname is ignored, since only 'idle' is supported.  Not sure how to
// select which anim groups to load (each one would be a separate kf file).
//
// FIXME: The parameter 'groups' is also ignored, since TES4 groups are different. There might
// be ways to combine both, however.  Something to think about.
//
// FIXME: Just to make the initial implementation easier, 'priority', 'autodisable',
// 'startpoint', 'loops' and 'loopfallback' are also ignored.
// (always have highest priority for testing?)
void ForeignNpcAnimation::play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback)
{
    //Animation::play(groupname, priority, groups, true/*autodisable*/, speedmult, start, stop, startpoint, 3/*loops*/, false/*loopfallback*/);
    Animation::play(groupname, priority, groups, autodisable, speedmult, start, stop, startpoint, loops, loopfallback);
}

void ForeignNpcAnimation::addFirstPersonOffset(const Ogre::Vector3 &offset)
{
    mFirstPersonOffset += offset;
}

class SetObjectGroup {
    int mGroup;

public:
    SetObjectGroup(int group) : mGroup(group) { }

    void operator()(Ogre::MovableObject *obj) const
    {
        obj->getUserObjectBindings().setUserAny(Ogre::Any(mGroup));
    }
};

NifOgre::ObjectScenePtr ForeignNpcAnimation::insertBoundedPart(const std::string &model, int group, const std::string &bonename, const std::string &bonefilter, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    NifOgre::ObjectScenePtr objects = NifOgre::Loader::createObjects(mSkelBase, bonename, bonefilter, mInsert, model);
    setRenderProperties(objects, (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags, RQG_Main, RQG_Alpha, 0,
                        enchantedGlow, glowColor);

    std::for_each(objects->mEntities.begin(), objects->mEntities.end(), SetObjectGroup(group));
    std::for_each(objects->mParticles.begin(), objects->mParticles.end(), SetObjectGroup(group));

    if(objects->mSkelBase)
    {
        Ogre::AnimationStateSet *aset = objects->mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }
        Ogre::SkeletonInstance *skelinst = objects->mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }

    return objects;
}

Ogre::Vector3 ForeignNpcAnimation::runAnimation(float timepassed)
{
    Ogre::Vector3 ret = Animation::runAnimation(timepassed);

    mHeadAnimationTime->update(timepassed);
//#if 0
    if (mSkelBase)
    {
        Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        if(mViewMode == VM_FirstPerson)
        {
            float pitch = mPtr.getRefData().getPosition().rot[0];
            Ogre::Node *node = baseinst->getBone("Bip01 Neck");
            node->pitch(Ogre::Radian(-pitch), Ogre::Node::TS_WORLD);

            // This has to be done before this function ends;
            // updateSkeletonInstance, below, touches the hands.
            node->translate(mFirstPersonOffset, Ogre::Node::TS_WORLD);
        }
        else
        {
            // In third person mode we may still need pitch for ranged weapon targeting
            pitchSkeleton(mPtr.getRefData().getPosition().rot[0], baseinst);

            if (0)//mHeadYaw != Ogre::Radian() || mHeadPitch != Ogre::Radian())
            {
                Ogre::Radian r = mHeadYaw;
                std::cout << "yaw " << r.valueDegrees() << std::endl;
            }

            Ogre::Node* node = baseinst->getBone("Bip01 Head");
            if (node)
                node->rotate(Ogre::Quaternion(mHeadYaw, Ogre::Vector3::UNIT_Z)
                           * Ogre::Quaternion(mHeadPitch, Ogre::Vector3::UNIT_X)
                           //* Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y) // fix helmet issue
                             ,Ogre::Node::TS_WORLD);
        }
    }
//#endif
    mFirstPersonOffset = 0.f; // reset the X, Y, Z offset for the next frame.

    // FIXME: it looks like that in TES3 skinned object parts retain their own skeleton
    // (at least OpenMW implementaton does) and we copy the bone movements of the NPC's skeleton.
    // For TES4 we already do this?
#if 0
    for(size_t i = 0; i < ESM::PRT_Count; ++i)
    {
        if (!mObjectParts[i])
            continue;

        std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[i]->mControllers.begin());
        for(;ctrl != mObjectParts[i]->mControllers.end();++ctrl)
            ctrl->update();

        if (!isSkinned(mObjectParts[i])) // FIXME: maybe cache the result of isSkinned()?
            continue;

        if (mSkelBase)
            updateSkeletonInstance(mSkelBase->getSkeleton(), mObjectParts[i]->mSkelBase->getSkeleton());

        mObjectParts[i]->mSkelBase->getAllAnimationStates()->_notifyDirty();
    }
#endif
    return ret;
}

void ForeignNpcAnimation::removeIndividualPart(ESM::PartReferenceType type)
{
    //mPartPriorities[type] = 0;
    //mPartslots[type] = -1;

    if (mObjectParts[type])
        mObjectParts[type].reset();
    if (0)//!mSoundIds[type].empty() && !mSoundsDisabled)
    {
        MWBase::Environment::get().getSoundManager()->stopSound3D(mPtr, mSoundIds[type]);
        mSoundIds[type].clear();
    }
}

void ForeignNpcAnimation::removeParts(ESM::PartReferenceType type)
{
    std::uint32_t part = 0x00000001;
    for (std::size_t i = 0; i < 32; ++i)
    {
        if ((type & part) != 0)
            removeIndividualPart((ESM::PartReferenceType)part);

        part <<= 1;
    }
}

void ForeignNpcAnimation::reserveIndividualPart(ESM::PartReferenceType type, int group, int priority)
{
    if(priority > mPartPriorities[type])
    {
        removeIndividualPart(type);
        mPartPriorities[type] = priority;
        mPartslots[type] = group;
    }
}

void ForeignNpcAnimation::removePartGroup(int group)
{
    for(int i = 0; i < ESM::PRT_Count; i++)
    {
        if(mPartslots[i] == group)
            removeIndividualPart((ESM::PartReferenceType)i);
    }
}

// It seems TES5 allows lots of body part slots:
//   http://wiki.tesnexus.com/index.php/Skyrim_bodyparts_number
//   https://github.com/amorilia/nifxml/blob/master/nif.xml#L631
//
// Not sure how TES4 works.
bool ForeignNpcAnimation::addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    if(priority <= mPartPriorities[type])
        return false;

    removeIndividualPart(type);
    mPartslots[type] = group;
    mPartPriorities[type] = priority;
    try
    {
        const std::string& bonename = "Bip01";//sPartList.at(type); // FIXME
        // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the attachment bone
        const std::string bonefilter = "";//(type == ESM::PRT_Hair) ? "hair" : bonename; //FIXME
        mObjectParts[type] = insertBoundedPart(mesh, group, bonename, bonefilter, enchantedGlow, glowColor);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error adding NPC part: " << e.what() << std::endl;
        return false;
    }

    if (!mSoundsDisabled)
    {
        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator csi = inv.getSlot(group < 0 ? MWWorld::InventoryStore::Slot_Helmet : group);
        if (csi != inv.end())
        {
            mSoundIds[type] = csi->getClass().getSound(*csi);
            if (!mSoundIds[type].empty())
            {
                MWBase::Environment::get().getSoundManager()->playSound3D(mPtr, mSoundIds[type], 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx,
                    MWBase::SoundManager::Play_Loop);
            }
        }
    }

    if(mObjectParts[type]->mSkelBase)
    {
        Ogre::SkeletonInstance *skel = mObjectParts[type]->mSkelBase->getSkeleton();

        if(mObjectParts[type]->mSkelBase->isParentTagPoint())
        {
            Ogre::Node *root = mObjectParts[type]->mSkelBase->getParentNode();
            if(skel->hasBone("BoneOffset"))
            {
                Ogre::Bone *offset = skel->getBone("BoneOffset");

                root->translate(offset->getPosition());

                // It appears that the BoneOffset rotation is completely bogus, at least for light models.
                //root->rotate(offset->getOrientation());
                root->pitch(Ogre::Degree(-90.0f));

                root->scale(offset->getScale());
                root->setInitialState();
            }
        }

        if (isSkinned(mObjectParts[type]))
            updateSkeletonInstance(mSkelBase->getSkeleton(), skel);
    }

    std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[type]->mControllers.begin());
    for(;ctrl != mObjectParts[type]->mControllers.end();++ctrl)
    {
        if(!ctrl->getSource())
        {
            ctrl->setSource(mNullAnimationTimePtr);
//#if 0
            if (type == ESM::PRT_Head)
            {
                ctrl->setSource(mHeadAnimationTime);
                const NifOgre::TextKeyMap& keys = mObjectParts[type]->mTextKeys;
                for (NifOgre::TextKeyMap::const_iterator it = keys.begin(); it != keys.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->second, "talk: start"))
                        mHeadAnimationTime->setTalkStart(it->first);
                    if (Misc::StringUtils::ciEqual(it->second, "talk: stop"))
                        mHeadAnimationTime->setTalkStop(it->first);
                    if (Misc::StringUtils::ciEqual(it->second, "blink: start"))
                        mHeadAnimationTime->setBlinkStart(it->first);
                    if (Misc::StringUtils::ciEqual(it->second, "blink: stop"))
                        mHeadAnimationTime->setBlinkStop(it->first);
                }
            }
            else if (type == ESM::PRT_Weapon)
                ctrl->setSource(mWeaponAnimationTime);
//#endif
        }
    }

    return true;
}

void ForeignNpcAnimation::addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();

    const char *ext = (mViewMode == VM_FirstPerson) ? ".1st" : "";
    std::vector<ESM::PartReference>::const_iterator part(parts.begin());
    for(;part != parts.end();++part)
    {
        const ESM::BodyPart *bodypart = 0;
        bool isFemale = (mNpc->mBaseConfig.flags & 0x000001) != 0; // 0x1 means female
        if(isFemale && !part->mFemale.empty())
        {
            bodypart = partStore.search(part->mFemale+ext);
            if(!bodypart && mViewMode == VM_FirstPerson)
            {
                bodypart = partStore.search(part->mFemale);
                if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                    bodypart = NULL;
            }
            else if (!bodypart)
                std::cerr << "Failed to find body part '" << part->mFemale << "'" << std::endl;
        }
        if(!bodypart && !part->mMale.empty())
        {
            bodypart = partStore.search(part->mMale+ext);
            if(!bodypart && mViewMode == VM_FirstPerson)
            {
                bodypart = partStore.search(part->mMale);
                if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                    bodypart = NULL;
            }
            else if (!bodypart)
                std::cerr << "Failed to find body part '" << part->mMale << "'" << std::endl;
        }

        if(bodypart)
            addOrReplaceIndividualPart((ESM::PartReferenceType)part->mPart, group, priority, "meshes\\"+bodypart->mModel, enchantedGlow, glowColor);
        else
            reserveIndividualPart((ESM::PartReferenceType)part->mPart, group, priority);
    }
}

void ForeignNpcAnimation::showWeapons(bool showWeapon)
{
    mShowWeapons = showWeapon;
    if(showWeapon)
    {
        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if(weapon != inv.end())
        {
            Ogre::Vector3 glowColor = getEnchantmentColor(*weapon);
            std::string mesh = weapon->getClass().getModel(*weapon);
            addOrReplaceIndividualPart(ESM::PRT_Weapon, MWWorld::InventoryStore::Slot_CarriedRight, 1,
                                       mesh, !weapon->getClass().getEnchantment(*weapon).empty(), &glowColor);

            // Crossbows start out with a bolt attached
            if (weapon->getTypeName() == typeid(ESM::Weapon).name() &&
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
            {
                MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
                if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt)
                    attachArrow();
                else
                    mAmmunition.reset();
            }
            else
                mAmmunition.reset();
        }
    }
    else
    {
        removeIndividualPart(ESM::PRT_Weapon);
    }
    mAlpha = 1.f;
}

void ForeignNpcAnimation::showCarriedLeft(bool show)
{
    mShowCarriedLeft = show;
    // FIXME: need to do logic for Slot_CarriedLeft
#if 0
    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if(show && iter != inv.end())
    {
        Ogre::Vector3 glowColor = getEnchantmentColor(*iter);
        std::string mesh = iter->getClass().getModel(*iter);
        if (addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1,
                                   mesh, !iter->getClass().getEnchantment(*iter).empty(), &glowColor))
        {
            if (iter->getTypeName() == typeid(ESM::Light).name())
                addExtraLight(mInsert->getCreator(), mObjectParts[ESM::PRT_Shield], iter->get<ESM::Light>()->mBase);
        }
    }
    else
        removeIndividualPart(ESM::PRT_Shield);
#endif
}

void ForeignNpcAnimation::configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot)
{
    Ogre::Vector3 glowColor = getEnchantmentColor(ptr);
    setRenderProperties(object, (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags, RQG_Main, RQG_Alpha, 0,
                        !ptr.getClass().getEnchantment(ptr).empty(), &glowColor);

    std::for_each(object->mEntities.begin(), object->mEntities.end(), SetObjectGroup(slot));
    std::for_each(object->mParticles.begin(), object->mParticles.end(), SetObjectGroup(slot));
}

void ForeignNpcAnimation::attachArrow()
{
    WeaponAnimation::attachArrow(mPtr);
}

void ForeignNpcAnimation::releaseArrow()
{
    WeaponAnimation::releaseArrow(mPtr);
}

void ForeignNpcAnimation::permanentEffectAdded(const ESM::MagicEffect *magicEffect, bool isNew, bool playSound)
{
    // During first auto equip, we don't play any sounds.
    // Basically we don't want sounds when the actor is first loaded,
    // the items should appear as if they'd always been equipped.
    if (playSound)
    {
        static const std::string schools[] = {
            "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
        };

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(!magicEffect->mHitSound.empty())
            sndMgr->playSound3D(mPtr, magicEffect->mHitSound, 1.0f, 1.0f);
        else
            sndMgr->playSound3D(mPtr, schools[magicEffect->mData.mSchool]+" hit", 1.0f, 1.0f);
    }

    if (!magicEffect->mHit.empty())
    {
        const ESM::Static* castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
        bool loop = (magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
        // Don't play particle VFX unless the effect is new or it should be looping.
        if (isNew || loop)
            addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "");
    }
}

void ForeignNpcAnimation::setAlpha(float alpha)
{
    if (alpha == mAlpha)
        return;
    mAlpha = alpha;

    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        if (!mObjectParts[i])
            continue;

        for (unsigned int j=0; j<mObjectParts[i]->mEntities.size(); ++j)
        {
            Ogre::Entity* ent = mObjectParts[i]->mEntities[j];
            if (ent != mObjectParts[i]->mSkelBase)
                applyAlpha(alpha, ent, mObjectParts[i]);
        }
    }
}

void ForeignNpcAnimation::enableHeadAnimation(bool enable)
{
    mHeadAnimationTime->setEnabled(enable);
}

void ForeignNpcAnimation::preRender(Ogre::Camera *camera)
{
    Animation::preRender(camera);
    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        if (!mObjectParts[i])
            continue;
        mObjectParts[i]->rotateBillboardNodes(camera);
    }
}

void ForeignNpcAnimation::applyAlpha(float alpha, Ogre::Entity *ent, NifOgre::ObjectScenePtr scene)
{
    sh::Factory::getInstance()._ensureMaterial(ent->getSubEntity(0)->getMaterial()->getName(), "Default");
    ent->getSubEntity(0)->setRenderQueueGroup(alpha != 1.f || ent->getSubEntity(0)->getMaterial()->isTransparent()
            ? RQG_Alpha : RQG_Main);


    Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(ent);
    if (mAlpha == 1.f)
    {
        // Don't bother remembering what the original values were. Just remove the techniques and let the factory restore them.
        mat->removeAllTechniques();
        sh::Factory::getInstance()._ensureMaterial(mat->getName(), "Default");
        return;
    }

    Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
            Ogre::ColourValue diffuse = pass->getDiffuse();
            diffuse.a = alpha;
            pass->setDiffuse(diffuse);
            pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_DIFFUSE);
        }
    }
}

void ForeignNpcAnimation::equipmentChanged()
{
    updateParts();
}

void ForeignNpcAnimation::setVampire(bool vampire)
{
    if (mNpcType == Type_Werewolf) // we can't have werewolf vampires, can we
        return;
    if ((mNpcType == Type_Vampire) != vampire)
    {
        if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            MWBase::Environment::get().getWorld()->reattachPlayerCamera();
        else
            rebuild();
    }
}

void ForeignNpcAnimation::setHeadPitch(Ogre::Radian pitch)
{
    mHeadPitch = pitch;
}

void ForeignNpcAnimation::setHeadYaw(Ogre::Radian yaw)
{
    mHeadYaw = yaw;
}

Ogre::Radian ForeignNpcAnimation::getHeadPitch() const
{
    return mHeadPitch;
}

Ogre::Radian ForeignNpcAnimation::getHeadYaw() const
{
    return mHeadYaw;
}

}

// characters/_male/skeleton.nif has:
//                                    {{{
// Bip01
// Bip01 Tail03
// Bip01 Tail06
// Bip01 L Finger41
// Bip01 R Finger42
// Bip01 R Finger01
// Bip01 L Finger0
// Bip01 L Finger22
// Bip01 R Toe0
// Bip01 L Toe0
// Bip01 L Finger12
// Bip01 Tail07
// Bip01 L Finger02
// Bip01 L Finger01
// Bip01 R Finger2
// magicNode
// Bip01 R Finger31
// Bip01 R Finger21
// Quiver
// Bip01 L Finger31
// Bip01 Tail04
// Bip01 R Finger3
// Camera01
// Bip01 R Finger11
// Bip01 Tail08
// Bip01 Tail02
// Bip01 Tail01
// Bip01 L Finger2
// Bip01 TailRoot
// Bip01 R Finger22
// Bip01 L Finger3
// Bip01 Tail05
// Bip01 L Finger32
// Bip01 R Finger0
// Bip01 L Finger42
// Bip01 R Finger4
// Bip01 R Finger32
// Bip01 R Finger1
// Bip01 R Finger41
// Bip01 R Finger12
// Bip01 L Finger4
// Bip01 R Finger02
// Bip01 L Finger21
// Bip01 L Finger11
// BackWeapon
// Bip01 L Finger1
// Bip01 L Shoulder Helper
// Bip01 R Shoulder Helper
// Bip01 R Forearm
// Torch
// Weapon
// Bip01 L Wrist Helper
// Bip01 L ForearmTwist
// Bip01 L Hand
// Bip01 Neck1
// Bip01 R Hand
// Bip01 R ForearmTwist
// Bip01 L Forearm
// Bip01 Head
// Bip01 R Wrist Helper
// Bip01 R UpperArmTwist
// Bip01 Neck
// Bip01 R Foot
// Bip01 R Clavicle
// Bip01 L Clavicle
// Bip01 R UpperArm
// Bip01 L UpperArm
// Bip01 L Foot
// Bip01 L UpperArmTwist
// Bip01 L Calf
// Bip01 Spine2
// Bip01 R Calf
// Bip01 Spine1
// Bip01 Pelvis
// Bip01 R Thigh
// SideWeapon
// Bip01 Spine
// Bip01 L Thigh
//
// characters/_male/skeleton.nif xbase_anim.kf
// -- sorted Bip01
//
// BackWeapon
// Bip01                        Bip01
// Bip01 Head                   Bip01 Head
// Bip01 L Calf                 Bip01 L Calf
// Bip01 L Clavicle             Bip01 L Clavicle
// Bip01 L Finger0              Bip01 L Finger0
// Bip01 L Finger01             Bip01 L Finger01
// Bip01 L Finger02
// Bip01 L Finger1              Bip01 L Finger1
// Bip01 L Finger11             Bip01 L Finger11
// Bip01 L Finger12
// Bip01 L Finger2              Bip01 L Finger2
// Bip01 L Finger21             Bip01 L Finger21
// Bip01 L Finger22
// Bip01 L Finger3
// Bip01 L Finger31
// Bip01 L Finger32
// Bip01 L Finger4
// Bip01 L Finger41
// Bip01 L Finger42
// Bip01 L Foot                 Bip01 L Foot
// Bip01 L Forearm              Bip01 L Forearm
// Bip01 L ForearmTwist
// Bip01 L Hand                 Bip01 L Hand
// Bip01 L Shoulder Helper
// Bip01 L Thigh                Bip01 L Thigh
// Bip01 L Toe0
// Bip01 L UpperArm             Bip01 L UpperArm
// Bip01 L UpperArmTwist
// Bip01 L Wrist Helper
// Bip01 Neck                   Bip01 Neck
// Bip01 Neck1
// Bip01 Pelvis                 Bip01 Pelvis
// Bip01 R Calf                 Bip01 R Calf
// Bip01 R Clavicle             Bip01 R Clavicle
// Bip01 R Finger0              Bip01 R Finger0
// Bip01 R Finger01             Bip01 R Finger01
// Bip01 R Finger02
// Bip01 R Finger1              Bip01 R Finger1
// Bip01 R Finger11             Bip01 R Finger11
// Bip01 R Finger12
// Bip01 R Finger2              Bip01 R Finger2
// Bip01 R Finger21             Bip01 R Finger21
// Bip01 R Finger22
// Bip01 R Finger3
// Bip01 R Finger31
// Bip01 R Finger32
// Bip01 R Finger4
// Bip01 R Finger41
// Bip01 R Finger42
// Bip01 R Foot                 Bip01 R Foot
// Bip01 R Forearm              Bip01 R Forearm
// Bip01 R ForearmTwist
// Bip01 R Hand                 Bip01 R Hand
// Bip01 R Shoulder Helper
// Bip01 R Thigh                Bip01 R Thigh
// Bip01 R Toe0
// Bip01 R UpperArm             Bip01 R UpperArm
// Bip01 R UpperArmTwist
// Bip01 R Wrist Helper
// Bip01 Spine                  Bip01 Spine
// Bip01 Spine1                 Bip01 Spine1
// Bip01 Spine2                 Bip01 Spine2
// Bip01 Tail01
// Bip01 Tail02
// Bip01 Tail03
// Bip01 Tail04
// Bip01 Tail05
// Bip01 Tail06
// Bip01 Tail07
// Bip01 Tail08
// Bip01 TailRoot
// Camera01
// Quiver
// SideWeapon
// Torch
// Weapon                       Weapon Bone
// magicNode

// -- sorted Bip01 NonAccum
//
// Bip01 Pelvis
// Bip01 Spine
// Bip01 L Thigh
// Bip01 R Thigh
// SideWeapon
// Bip01 TailRoot
// ... etc
//                                    }}}
/* vim: set tw=100 fen fdm=marker fdl=0: */