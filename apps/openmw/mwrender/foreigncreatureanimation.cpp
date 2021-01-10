#include "foreigncreatureanimation.hpp"

#include <iostream> // FIXME

#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>

#include <extern/esm4/crea.hpp>
#include <extern/nibtogre/btogreinst.hpp>

#include <components/esm/loadcrea.hpp>
#include <components/misc/rng.hpp>

#include "../mwbase/world.hpp"
//#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/inventorystoretes4.hpp"

#include "../mwclass/foreigncreature.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "renderconst.hpp"

namespace MWRender
{

void ForeignCreatureAnimation::play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback)
{
    if (mPtr.getClass().getCreatureStats(mPtr).isDead())
        std::cout << "dead" << std::endl;

    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " play " << groupname << std::endl;

    Animation::play(groupname, priority, groups, autodisable, speedmult, start, stop, startpoint, loops, loopfallback);
}

ForeignCreatureAnimation::ForeignCreatureAnimation(const MWWorld::Ptr &ptr, const std::string& skeletonModel)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    //std::cout << ref->mBase->mEditorId << " " << ref->mBase->mFullName << std::endl;

    if(skeletonModel.empty())
        return;

    std::string skeletonName = skeletonModel;
    Misc::StringUtils::lowerCaseInPlace(skeletonName);

    //if (skeletonName.find("marker_creature") != std::string::npos)
        //return; // FIXME: FO3 mEditorId="LvlTurretCeiling768Raider

    setForeignObjectRootBase(skeletonName);
    //setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

    if (mObjectRoot->mForeignObj)
    {
        mObjectRoot->mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
        mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
    }

    if (mObjectRoot->mSkelBase == nullptr) // FIXME: FONV
        return;

    if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip01"))
    {
        Ogre::Bone* b = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01");
        Ogre::Bone* bna = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01 NonAccum");
        if (b && bna)
        {
            //std::cout << ref->mBase->mEditorId << " Bip01 " << ref->mBase->mFullName << std::endl;
            // FIXME: needs a good cleanup
            if (1)//ref->mBase->mFullName == "Storm Atronach")
            {
                std::map<int32_t, Ogre::Entity*>::const_iterator it(mObjectRoot->mForeignObj->mEntities.begin());
                for (; it != mObjectRoot->mForeignObj->mEntities.end(); ++it)
                {
                    if (!it->second->isAttached())
                    {
                        it->second->shareSkeletonInstanceWith(mSkelBase);
                        mInsert->attachObject(it->second);
                    }
                }
            }
    // "Creatures\\StormAtronach\\Skeleton.nif"

            Ogre::Quaternion qb = b->getOrientation(); // skeleton.nif has -90 roll
            Ogre::Quaternion qbna = bna->getOrientation(); // has 0 roll
            //b->setOrientation(qbna);
            //bna->setOrientation(qb);
            //b->setOrientation(qb * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y));

            // FIXME: all the skinned meshes seems to be offset by this, probably something to do
            // with the binding position
            Ogre::Vector3 vb = b->getPosition();
            Ogre::Vector3 vbna = bna->getPosition();
            Ogre::Vector3 ins = mInsert->getPosition();

            // 17/04/20 commenting out below doesn't seem to do anything? OurLadyHope01
            //          mEditorId "CrSuperMutant1GunA" has vbna ZERO
            //b->setPosition(vbna+ Ogre::Vector3(0,0,0.f)); // TEMP testing for FO3
            //bna->setPosition(vb);
            //bna->setPosition(vb.x, vb.y, -vb.z);

            //Ogre::Bone* rootBone = mObjectRoot->mSkelBase->getSkeleton()->getBone("Scene Root");
            //Ogre::Vector3 vRoot = rootBone->convertLocalToWorldPosition(rootBone->_getDerivedPosition());
            //Ogre::Vector3 vBase = mSkelBase->getParentSceneNode()->_getDerivedPosition();
            //std::cout << vRoot.x << " " << vBase.x << std::endl;
            //std::cout << vRoot.y << " " << vBase.y << std::endl;
            //std::cout << vRoot.z << " " << vBase.z << std::endl;
        }
    }
    else if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip02"))
    {
        // FIXME: just testing which creatures have this; looks like mainly horses
        //std::cout << ref->mBase->mEditorId << " Bip02 " << ref->mBase->mFullName << std::endl;

        Ogre::Bone* b2 = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02");
        Ogre::Bone* bna2 = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02 NonAccum");
        if (b2 && bna2)
        {
            Ogre::Quaternion qb = b2->getOrientation(); // skeleton.nif has -90 roll
            Ogre::Quaternion qbna = bna2->getOrientation(); // has 0 roll
            //b2->setOrientation(qbna);
            //bna2->setOrientation(qb);
            //Ogre::Vector3 vb = b2->getPosition();
            //Ogre::Vector3 ins = mInsert->getPosition();
        }
    }

    //if((ref->mBase->mFlags&ESM::Creature::Bipedal))
        //addAnimSource("meshes\\xbase_anim.nif");
    addAnimSource(skeletonName);

    size_t pos = skeletonName.find_last_of('\\');
    std::string path = skeletonName.substr(0, pos+1); // +1 for '\\'
    std::string group("General");

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    for (std::size_t i = 0; i < ref->mBase->mNif.size(); ++i)
    {
        //std::cout << ref->mBase->mNif[i] << std::endl;
        std::string meshName = path+ref->mBase->mNif[i];

        // initially assume a skinned model
        NiModelPtr object = modelManager.getByName(skeletonName + "_" + meshName, group);
        // if not found just create a non-skinned model to check
        if (!object)
        {
            object = modelManager.getOrLoadByName(meshName, group);
            if (object->buildData().isSkinnedModel()) // does it have an NiSkinInstance?
            {
                // was skinned after all
                object.reset();
                object = modelManager.createSkinnedModel(meshName,
                    group, mObjectRoot->mForeignObj->mModel.get(),""/*no race for creatures?*/);
            }
        }

        // create an instance of the model
        NifOgre::ObjectScenePtr scene
            = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));

#if defined(__GNUC__) && __GNUC__ < 8
        scene->mForeignObj = std::unique_ptr<NiBtOgre::BtOgreInst>(new NiBtOgre::BtOgreInst(object,
                             mInsert->createChildSceneNode()));
#else
        scene->mForeignObj = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(object,
                             mInsert->createChildSceneNode()));
#endif
#if 1
        scene->mForeignObj->instantiateBodyPart(mInsert, mSkelBase);
#else
        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            if (scene->mForeignObj->mIsSkinned)
            {
                it->second->shareSkeletonInstanceWith(mSkelBase);
                mInsert->attachObject(it->second);
            }
            else
                mSkelBase->attachObjectToBone(scene->mForeignObj->mTargetBone, it->second);
        }
#endif
        hideDismember(scene);
        mObjectParts.push_back(scene);
    }

    // the rotation is caused by the animations and can't be fixed here
#if 0
    MWBase::World *world = MWBase::Environment::get().getWorld();
    world->rotateObject(mPtr, 0.f, 0.f, 90.f, true);
    // FIXME: 90 deg issue - does not work
    mInsert->rotate(//Ogre::Quaternion(mHeadYaw, Ogre::Vector3::UNIT_Z)
               //* Ogre::Quaternion(mHeadPitch, Ogre::Vector3::UNIT_X)
               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z)
                 ,Ogre::Node::TS_WORLD);
#endif

    mSkelBase->getSkeleton()->reset(true);
}

NiModelPtr ForeignCreatureAnimation::getSkeletonModel() const
{
    if (mObjectRoot && mObjectRoot->mForeignObj)
        return mObjectRoot->mForeignObj->mModel;

    throw std::runtime_error("skeleton model not found");
}

// FIXME: duplicated with ForeignNpcAnimation
void ForeignCreatureAnimation::hideDismember(NifOgre::ObjectScenePtr scene)
{
    if (!scene->mForeignObj->mModel->buildData().isSkinnedModel()) // does it have an NiSkinInstance?
        return;

    std::map<std::int32_t, std::vector<std::vector<std::uint16_t> > > dismemberBodyPartMap;
    scene->mForeignObj->mModel->fillDismemberParts(dismemberBodyPartMap);

    // we need the vector of Dismember BodyPart (where the vector index is the sub-mesh
    // index which hopefully matches with sub-entity index)
    if (!dismemberBodyPartMap.empty())
    {
        std::map<std::int32_t, Ogre::Entity*>::iterator it = scene->mForeignObj->mEntities.begin();
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            // it->first is NiNodeRef
            std::map<std::int32_t, std::vector<std::vector<std::uint16_t> > >::iterator it2
                = dismemberBodyPartMap.find(it->first);
            if (it2 != dismemberBodyPartMap.end())
            {
                // it2->first should equal it->first
                if (it->first != it2->first)
                    throw std::runtime_error(scene->mForeignObj->mModel->getName() + " node mismatch");

                const std::vector < std::vector<std::uint16_t> >& subMeshes = it2->second;
                for (std::size_t i = 0; i < subMeshes.size(); ++i) // i == SubMesh index
                {
                    bool dismember = false;
                    for (std::size_t j = 0; j < subMeshes[i].size(); ++j) // all the parts
                    {
                        std::uint16_t part = subMeshes[i][j];
                        if (part > 13/*BP_BRAIN*/ && part < 1000/*BP_TORSOSECTION_HEAD*/)
                            dismember = true;
                    }

                    if (dismember)
                        it->second->getSubEntity(i)->setVisible(false);
                }
            }
        }
    }
}

void ForeignCreatureAnimation::addAnimSource(const std::string &skeletonName)
{
    size_t pos = skeletonName.find_last_of('\\');
    std::string path = skeletonName.substr(0, pos+1); // +1 for '\\'

    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    // seems to be for robots only in FO3/FONV
    std::string animName;
    for (unsigned int i = 0; i < ref->mBase->mKf.size(); ++i)
    {
        //std::cout << ref->mBase->mEditorId << " " << ref->mBase->mKf[i] << std::endl; // FIXME
        animName = path + ref->mBase->mKf[i];
        addForeignAnimSource(skeletonName, animName);
    }

    if (ref->mBase->mKf.size() != 0)
    {
        if (skeletonName.find("mistergutsy") != std::string::npos)
            addForeignAnimSource(skeletonName, path + "mtidle.kf"); // specified ones don't work yet?
        else
        {
            // FIXME: hack for testing, add a randome one again to play it
            int roll = Misc::Rng::rollDice(ref->mBase->mKf.size());
            addForeignAnimSource(skeletonName, path+ref->mBase->mKf[roll]);
        }

        return; // only use the specified ones;
    }

    // FIXME: fallback anims for demo

    //addForeignAnimSource(skeletonName, path + "castself.kf");
    //addForeignAnimSource(skeletonName, path + "backward.kf");
    addForeignAnimSource(skeletonName, path + "idle.kf");
    addForeignAnimSource(skeletonName, path + "forward.kf");
    //addForeignAnimSource(skeletonName, path + "fastforward.kf");  //10/03/20: Storm Atronach not working
    //addForeignAnimSource(skeletonName, path + "runforward.kf");  //10/03/20: Storm Atronach not working

    if (skeletonName.find("spine") != std::string::npos)
    {
        addForeignAnimSource(skeletonName, path + "mtidle.kf"); // Super Mutant
        addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_mtheadrub.kf"); // Super Mutant
    }
    if (skeletonName.find("entaur") != std::string::npos)
        addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_scan.kf"); // Centaur
    if (skeletonName.find("eathclaw") != std::string::npos)
    {
        //addForeignAnimSource(skeletonName, path + "locomotion\\mtforward.kf"); // doesn't work?
        int roll = Misc::Rng::rollDice(2);
        if (roll == 0)
            addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_hithead.kf");
        else if (roll == 1)
            //addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_hittorso.kf");
            addForeignAnimSource(skeletonName, path + "locomotion\\hurt\\mtforward_hurt.kf");
        else if (roll == 2)
            addForeignAnimSource(skeletonName, path + "mtidle.kf"); // Deathclaw
    }
    if (skeletonName.find("houl") != std::string::npos)
    {
        int roll = Misc::Rng::rollDice(4); // [0, 3]
        if (roll == 0)
            addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_scan.kf"); // Ghoul
        else if (roll == 1)
            addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_crouch2stand.kf"); // Ghoul
        else if (roll == 2)
            addForeignAnimSource(skeletonName, path + "locomotion\\mtforward.kf"); // Ghoul
        else
        {
            addForeignAnimSource(skeletonName, path + "idleanims\\specialidle_crouchidle.kf"); // Ghoul
            //addForeignAnimSource(skeletonName, path + "locomotion\\specialidle_hithead.kf"); // doesn't work?
            //addForeignAnimSource(skeletonName, path + "locomotion\\specialidle_getupfaceup.kf"); // doesn't work?
        }
    }
}

void ForeignCreatureAnimation::addForeignAnimSource(const std::string& model, const std::string &animName)
{
    // Check whether the kf file exists
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(animName))
        return;
    std::string group("General"); // FIXME
#if 0
    NiModelPtr creatureModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(model, group);
    //creatureModel->buildSkeleton(); // FIXME: hack
#else

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    NiModelPtr skeleton = modelManager.getByName(model, "General");
    if (!skeleton)
        skeleton = modelManager.createSkeletonModel(model, "General");

#endif
    assert(!skeleton.isNull() && "skeleton.nif should have been built already");
    NiModelPtr animModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(animName, group);

    // Animation::AnimSource : public Ogre::AnimationAlloc
    //   (has a) std::multimap<float, std::string> mTextKeys
    //   (also has a vector of 4 Ogre real controllers)  TODO: check if 4 is enough
    Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
    std::vector<Ogre::Controller<Ogre::Real> > controllers;
    animModel->buildAnimation(mSkelBase, animModel, animSource->mTextKeys, controllers, skeleton.get());

    if (animSource->mTextKeys.empty() || controllers.empty())
        return;

    mAnimSources.push_back(animSource);

    std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animSource->mControllers;
    for (size_t i = 0; i < controllers.size(); i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[i].getDestination().get());

        size_t grp = detectAnimGroup(dstval->getNode());
#if 0
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
#else
        if (!mAccumRoot && grp == 0 && ((dstval->getNode()->getName() == "Bip01 NonAccum")
                                     || (dstval->getNode()->getName() == "Bip02 NonAccum"))) // horse
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
        }
#endif
        controllers[i].setSource(mAnimationTimePtr[grp]);
        grpctrls[grp].push_back(controllers[i]);
    }
    if (!mAccumRoot)
        throw std::runtime_error(model + ": could not find NonAccum root");

    // FIXME: debugging
    NifOgre::NodeTargetValue<Ogre::Real> *dstval;
    dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[0].getDestination().get());
    Ogre::Node *node = dstval->getNode();
    while (node && node->getName() != "Bip01 NonAccum")// && (node->getName() != "Bip02 NonAccum")))
    {
        node = node->getParent();
    }

    if (node && node->getName() == "Bip01 NonAccum")// || (node->getName() == "Bip02 NonAccum")))
    {
    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " root " << ref->mBase->mFullName << std::endl;
        mNonAccumRoot = node;
        mAccumRoot = mNonAccumRoot->getParent();
    }
    else if (0)//node && ((node->getName() == "Bip01") || (node->getName() == "Bip02")))
    {
    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " no root " << ref->mBase->mFullName << std::endl;
        mAccumRoot = node;
    }
    // else throw?
    else if (0)
    {
    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " bone root " << ref->mBase->mFullName << std::endl;
        if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip01"))
        {
            mAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01");
            mNonAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01 NonAccum");
        }
        else if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip02"))
        {
            mAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02");
            mNonAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02 NonAccum");
        }
    }

    //if ((mNonAccumRoot->getName() != "Bip01 NonAccum" || mAccumRoot->getName() != "Bip01")
        //|| (mNonAccumRoot->getName() != "Bip02 NonAccum" || mAccumRoot->getName() != "Bip02"))
        //std::cout << mAccumRoot->getName() << std::endl;
    // end debugging

    for (unsigned int i = 0; i < mObjectRoot->mControllers.size(); ++i)
    {
        if (!mObjectRoot->mControllers[i].getSource())
            mObjectRoot->mControllers[i].setSource(mAnimationTimePtr[0]);
    }

    // check inventory
    //std::vector<const ESM4::Clothing*> invCloth;
    //std::vector<const ESM4::Armor*> invArmor;
    std::vector<const ESM4::Weapon*> invWeap;
    MWWorld::InventoryStoreTES4& inv = static_cast<const MWClass::ForeignCreature&>(mPtr.getClass()).getInventoryStoreTES4(mPtr);
    for(size_t i = 0; i < inv.getNumSlots(); ++i)
    {
        MWWorld::ContainerStoreIterator store = inv.getSlot(i);

        if(store == inv.end())
            continue;

        // TODO: this method of handling inventory doen't suit TES4 very well because it is possible
        //       for one part to occupy more than one slot; for now as a workaround just loop
        //       through the slots to gather all the equipped parts and process them afterwards

        /*if(store->getTypeName() == typeid(ESM4::Clothing).name())
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
        else */if(store->getTypeName() == typeid(ESM4::Weapon).name())
        {
            const ESM4::Weapon *weap = store->get<ESM4::Weapon>()->mBase;
            if (std::find(invWeap.begin(), invWeap.end(), weap) == invWeap.end())
                invWeap.push_back(weap);
        }
    }

    /*for (std::size_t i = 0; i < invCloth.size(); ++i)
        equipClothes(invCloth[i], isFemale);

    for (std::size_t i = 0; i < invArmor.size(); ++i)
        equipArmor(invArmor[i], isFemale);*/

    for (std::size_t i = 0; i < invWeap.size(); ++i)
    {
        if (invWeap[i]->mModel == "")
            continue; // FIXME: FO3 Centaur

        std::string meshName;

        meshName = "meshes\\"+invWeap[i]->mModel;

        int type = ESM4::Armor::TES4_Weapon;

        // FIXME: group "General"
        // FIXME: prob wrap this with a try/catch block
        mObjectParts.push_back(
                createObject(meshName, "General", mObjectRoot->mForeignObj->mModel));
    }
}

NifOgre::ObjectScenePtr ForeignCreatureAnimation::createObject(const std::string& meshName,
        const std::string& group, NiModelPtr skeletonModel)
{
    // FIXME: probably needs a try/catch block here

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();

    // initially assume a skinned model
    std::string skeletonName = skeletonModel->getName();
    Misc::StringUtils::lowerCaseInPlace(skeletonName);
    NiModelPtr model = modelManager.getByName(skeletonName + "_" + meshName, group);

    // if not found just create a non-skinned model to check
    if (!model)
    {
        // create a vanilla model to test
        model = modelManager.getOrLoadByName(meshName, group);

        if (model->buildData().isSkinnedModel()) // does it have an NiSkinInstance?
        {
            // was skinned after all
            model.reset();
            model = modelManager.createSkinnedModel(meshName, group, skeletonModel.get(), "");
        }
    }

    // create an instance of the model
    NifOgre::ObjectScenePtr scene
        = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));

#if defined(__GNUC__) && __GNUC__ < 8
        scene->mForeignObj
            = std::unique_ptr<NiBtOgre::BtOgreInst>(new NiBtOgre::BtOgreInst(model, mInsert->createChildSceneNode()));
#else
        scene->mForeignObj
            = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(model, mInsert->createChildSceneNode()));
#endif
        scene->mForeignObj->instantiateBodyPart(mInsert, mSkelBase);

    return scene;
}

Ogre::Vector3 ForeignCreatureAnimation::runAnimation(float timepassed)
{
    Ogre::Vector3 ret = Animation::runAnimation(timepassed);

    //if (mSkelBase)
        //pitchSkeleton(mPtr.getRefData().getPosition().rot[0], mSkelBase->getSkeleton());

    if (0)//mSkelBase)
    {
        Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        Ogre::Node* node = baseinst->getBone("Bip01");
        if (node)
            node->rotate(Ogre::Quaternion(Ogre::Degree(0), Ogre::Vector3::UNIT_Z)
                       * Ogre::Quaternion(Ogre::Degree(0), Ogre::Vector3::UNIT_X)
                       * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y)
                         ,Ogre::Node::TS_WORLD);
    }
#if 0
    for(size_t i = 0; i < mObjectParts.size(); ++i)
    {
        std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[i]->mControllers.begin());
        for(;ctrl != mObjectParts[i]->mControllers.end();++ctrl)
            ctrl->update();

        if (mObjectParts[i]->mForeignObj->mEntities.size() == 0 ||
                !mObjectParts[i]->mForeignObj->mEntities.begin()->second->hasSkeleton()) // FIXME: maybe cache the result of isSkinned()?
            continue;

        if (0)//mSkelBase)
            updateSkeletonInstance(mSkelBase->getSkeleton(), mObjectParts[i]->mSkelBase->getSkeleton());

        mObjectParts[i]->mForeignObj->mSkeletonRoot->getAllAnimationStates()->_notifyDirty();
    }
#endif
    return ret;
}

// called from Actors::insertCreature if ForeignCreature::insertObjectRendering() indicates
// weapon/shield can be used (maybe not so useful in TES4?)
ForeignCreatureWeaponAnimation::ForeignCreatureWeaponAnimation(const MWWorld::Ptr &ptr, const std::string& model)
    : Animation(ptr, ptr.getRefData().getBaseNode())
    , mShowWeapons(false)
    , mShowCarriedLeft(false)
{
    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);

        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);

        updateParts();
    }

    mWeaponAnimationTime = Ogre::SharedPtr<WeaponAnimationTime>(new WeaponAnimationTime(this));
}

void ForeignCreatureWeaponAnimation::showWeapons(bool showWeapon)
{
    if (showWeapon != mShowWeapons)
    {
        mShowWeapons = showWeapon;
        updateParts();
    }
}

void ForeignCreatureWeaponAnimation::showCarriedLeft(bool show)
{
    if (show != mShowCarriedLeft)
    {
        mShowCarriedLeft = show;
        updateParts();
    }
}

void ForeignCreatureWeaponAnimation::updateParts()
{
    mWeapon.reset();
    mShield.reset();

    if (mShowWeapons)
        updatePart(mWeapon, MWWorld::InventoryStore::Slot_CarriedRight);
    if (mShowCarriedLeft)
        updatePart(mShield, MWWorld::InventoryStore::Slot_CarriedLeft);
}

void ForeignCreatureWeaponAnimation::updatePart(NifOgre::ObjectScenePtr& scene, int slot)
{
    if (!mSkelBase)
        return;

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator it = inv.getSlot(slot);

    if (it == inv.end())
    {
        scene.reset();
        return;
    }
    MWWorld::Ptr item = *it;

    std::string bonename;
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        bonename = "Weapon Bone";
    else
        bonename = "Shield Bone";

    scene = NifOgre::Loader::createObjects(mSkelBase, bonename, bonename, mInsert, item.getClass().getModel(item));
    Ogre::Vector3 glowColor = getEnchantmentColor(item);

    setRenderProperties(scene, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !item.getClass().getEnchantment(item).empty(), &glowColor);

    // Crossbows start out with a bolt attached
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight &&
            item.getTypeName() == typeid(ESM4::Weapon).name() /*&&
            item.get<ESM4::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow*/) // FIXME: temp comment out to compile
    {
        MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (ammo != inv.end() /*&& ammo->get<ESM4::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt*/) // FIXME: temp comment out
            attachArrow();
        else
            mAmmunition.reset();
    }
    else
        mAmmunition.reset();

    if(scene->mSkelBase)
    {
        Ogre::SkeletonInstance *skel = scene->mSkelBase->getSkeleton();
// FIXME: what does this block do?
#if 0
        if(scene->mSkelBase->isParentTagPoint())
        {
            Ogre::Node *root = scene->mSkelBase->getParentNode();
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
#endif
        updateSkeletonInstance(mSkelBase->getSkeleton(), skel);
    }

    std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(scene->mControllers.begin());
    for(;ctrl != scene->mControllers.end();++ctrl)
    {
        if(!ctrl->getSource())
        {
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                ctrl->setSource(mWeaponAnimationTime);
            else
                ctrl->setSource(Ogre::SharedPtr<NullAnimationTime>(new NullAnimationTime()));
        }
    }
}

void ForeignCreatureWeaponAnimation::configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot)
{
    Ogre::Vector3 glowColor = getEnchantmentColor(ptr);

    setRenderProperties(object, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !ptr.getClass().getEnchantment(ptr).empty(), &glowColor);
}

void ForeignCreatureWeaponAnimation::attachArrow()
{
    WeaponAnimation::attachArrow(mPtr);
}

void ForeignCreatureWeaponAnimation::releaseArrow()
{
    WeaponAnimation::releaseArrow(mPtr);
}

Ogre::Vector3 ForeignCreatureWeaponAnimation::runAnimation(float duration)
{
    Ogre::Vector3 ret = Animation::runAnimation(duration);

    if (mSkelBase)
        pitchSkeleton(mPtr.getRefData().getPosition().rot[0], mSkelBase->getSkeleton());

    if (mWeapon)
    {
        for (unsigned int i=0; i<mWeapon->mControllers.size(); ++i)
            mWeapon->mControllers[i].update();
    }
    if (mShield)
    {
        for (unsigned int i=0; i<mShield->mControllers.size(); ++i)
            mShield->mControllers[i].update();
    }

    return ret;
}

}
