#include "foreignnpc.hpp"

#include <extern/esm4/npc_.hpp>
#include <extern/esm4/race.hpp>
#include <extern/esm4/formid.hpp> // FIXME mainly for debugging

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystoretes4.hpp"
#include "../mwworld/inventorystorefo3.hpp"
#include "../mwworld/inventorystoretes5.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/levelledlist.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/foreignnpcanimation.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    struct ForeignNpcCustomData : public MWWorld::CustomData
    {
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore *mInventoryStore;
        int mGameType; // FIXME: make enum

        MWWorld::Ptr mPlaced;

        ForeignNpcCustomData(const ESM4::Npc& npc)
            : mInventoryStore(nullptr), mGameType(-1), mPlaced(MWWorld::Ptr())
        {
            bool isTES5 = false;
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Race *race = store.getForeign<ESM4::Race>().search(npc.mRace);
            if (race)
                isTES5 = race->mIsTES5;

            if (npc.mIsTES4)
            {
                mGameType = 0;
                mInventoryStore = new MWWorld::InventoryStoreTES4();
            }
            else if (isTES5)
            {
                mGameType = 2;
                mInventoryStore = new MWWorld::InventoryStoreTES5();
            }
            else// if (npc.mIsFONV)
            {
                mGameType = 1;
                mInventoryStore = new MWWorld::InventoryStoreFO3();
            }

        }

        ~ForeignNpcCustomData()
        {
// FIXME: linux crash here (but not for gcc 9.3.0 / Ubuntu 20.04)
#if !defined(__GNUC__) || __GNUC__ > 9
            if (mInventoryStore)
                delete mInventoryStore;
#endif
        }

        MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *ForeignNpcCustomData::clone() const
    {
        ForeignNpcCustomData *clone = new ForeignNpcCustomData (*this);

        return clone;
    }
}

namespace MWClass
{
    std::string ForeignNpc::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Npc>()->mBase->mEditorId;
    }

    void ForeignNpc::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        // this might be a levelled actor (e.g. FO3) - check if the model is empty
        const ESM4::Npc *npc = ptr.get<ESM4::Npc>()->mBase;
        if (npc && npc->mBaseTemplate != 0
                && (npc->mModel.empty() || npc->mModel == "marker_creature.nif"))
        {
            //std::cout << npc->mEditorId << std::endl; // FIXME

            // FIXME: save inventory details (and baseconfig?) to update later

            std::string id = MWMechanics::getFO3LevelledNpc(npc);

            if (!id.empty())
            {
                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                MWWorld::ManualRef ref(store, id);
                ref.getPtr().getCellRef().setPosition(ptr.getCellRef().getPosition());

                // FIXME: update inventory details, etc. (probably have to use customData)

                // FIXME: basically we spawn a new one each time, need to keep track and delete the old one
                //        actually, rather than deleting the old one we should be remembering
                //        that the ptr already exists
                ensureCustomData(ptr); // NOTE: this is ptr's custom data, not ref->getPtr()
                ForeignNpcCustomData *data
                    = dynamic_cast<ForeignNpcCustomData*>(ptr.getRefData().getCustomData());

                if (data->mPlaced.isEmpty())
                {
                    data->mPlaced = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(),
                                   ptr.getCell() , ptr.getCellRef().getPosition());
                    //std::cout << "placed " << ptr.get<ESM4::Npc>()->mBase->mEditorId << std::endl;
                }
                //else
                    //std::cout << "already placed " << ptr.get<ESM4::Npc>()->mBase->mEditorId << std::endl;
            }
        }
        else
            renderingInterface.getActors().insertNPC(ptr);
    }

    void ForeignNpc::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
#if 0
        // FIXME: in TES5 the skeletal model is in Race
        if (model == "")
        {
            const ESM4::Npc *npc = ptr.get<ESM4::Npc>()->mBase;
            if (!npc)
                return;

            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Race *race = store.getForeign<ESM4::Race>().search(npc->mRace); // WARN: might be null
            if (race && race->mModelMale != "" || race->mModelFemale != "")
            {
                //physics.addActor(ptr, /*skelModel*/model);
                MWBase::Environment::get().getMechanicsManager()->add(ptr);
            }

            return;
        }
#endif

#if 0 // FIXME: doesn't work for FO3
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            std::string skelModel;
            if (model == "")
            {
                const ESM4::Npc *npc = ptr.get<ESM4::Npc>()->mBase;

                const ESM4::Race* race = store.getForeign<ESM4::Race>().search(npc->mRace);
                if ((npc->mActorBaseConfig.flags & 0x1) != 0) // female
                    skelModel = "meshes\\" + race->mFemaleModel;
                else
                    skelModel = "meshes\\" + race->mMaleModel;
            }

#endif
#if 0
        const ESM4::Npc *npc = ptr.get<ESM4::Npc>()->mBase;
        if (npc && npc->mBaseTemplate != 0
                && (npc->mModel.empty() || npc->mModel == "marker_creature.nif"))
        {
            //std::cout << npc->mEditorId << std::endl;
        }
        else
        {
            physics.addActor(ptr, /*skelModel*/model);
            MWBase::Environment::get().getMechanicsManager()->add(ptr);
        }
#else
        // we ignore `model` and retrieve the skeleton already built
        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (MWRender::ForeignNpcAnimation *foreignAnim = dynamic_cast<MWRender::ForeignNpcAnimation*>(anim))
        {
            NiModelPtr skelModel = foreignAnim->getSkeletonModel();
            Ogre::Entity* skelBase = foreignAnim->getSkelBase();

            physics.addForeignActor(ptr, skelModel->getName(), *skelBase);
            MWBase::Environment::get().getMechanicsManager()->add(ptr);
        }
#endif
    }

    std::string ForeignNpc::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM4::Npc>();
        assert(ref->mBase != NULL);

        std::string model = "meshes\\skeleton.nif";
        return model;
#endif
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            //std::cout << "ForeignNpc: " << model << std::endl;
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignNpc::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        return ref->mBase->mFullName;
    }

    MWMechanics::CreatureStats& ForeignNpc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    MWMechanics::NpcStats& ForeignNpc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    MWWorld::ContainerStore& ForeignNpc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return *(dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore);
    }

    MWWorld::InventoryStore& ForeignNpc::getInventoryStore (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return *(dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore);
    }

    MWWorld::InventoryStoreTES4& ForeignNpc::getInventoryStoreTES4 (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<MWWorld::InventoryStoreTES4&>(*(dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore));
    }

    MWWorld::InventoryStoreFO3& ForeignNpc::getInventoryStoreFO3 (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<MWWorld::InventoryStoreFO3&>(*(dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore));
    }

    MWWorld::InventoryStoreTES5& ForeignNpc::getInventoryStoreTES5 (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<MWWorld::InventoryStoreTES5&>(*(dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore));
    }

    bool ForeignNpc::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        return !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat() || getCreatureStats(ptr).isDead();
    }

    MWGui::ToolTipInfo ForeignNpc::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        info.caption = getName(ptr);
        if(fullHelp && getNpcStats(ptr).isWerewolf())
        {
            info.caption += " (";
            info.caption += ref->mBase->mEditorId;
            info.caption += ")";
        }

        if(fullHelp)
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Script *script = store.getForeign<ESM4::Script>().search(ref->mBase->mScriptId);
            if (script)
            {
                info.text = MWGui::ToolTips::getMiscString(script->mScript.scriptSource, "Script");
            }
        }

        return info;
    }

    int ForeignNpc::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        // FIXME
        //return ptr.getClass().getNpcStats(ptr).getSkill(skill).getModified();
        return 15;
    }

    int ForeignNpc::getBloodTexture(const MWWorld::Ptr &ptr) const
    {
        //MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>(); // currently unused

        // FIXME
#if 0
        if (ref->mBase->mFlags & ESM::NPC::Skeleton)
            return 1;
        if (ref->mBase->mFlags & ESM::NPC::Metal)
            return 2;
#endif
        return 0;
    }

    void ForeignNpc::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

            std::auto_ptr<ForeignNpcCustomData> data(new ForeignNpcCustomData(*ref->mBase));

            // FIXME: only for TES4 at the momemnt
            // creature stats
            int gold = 0;
            if ((ref->mBase->mBaseConfig.tes4.flags & ESM4::Npc::TES4_AutoCalcStats) == 0)
            {
                gold = ref->mBase->mBaseConfig.tes4.barterGold;

                data->mNpcStats.getSkill (ESM::Skill::Block).setBase (ref->mBase->mData.skills.block);
                data->mNpcStats.getSkill (ESM::Skill::Armorer).setBase (ref->mBase->mData.skills.armorer);
                data->mNpcStats.getSkill (ESM::Skill::MediumArmor).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::HeavyArmor).setBase (ref->mBase->mData.skills.heavyArmor);
                data->mNpcStats.getSkill (ESM::Skill::BluntWeapon).setBase (ref->mBase->mData.skills.blunt);
                data->mNpcStats.getSkill (ESM::Skill::LongBlade).setBase (ref->mBase->mData.skills.blade);
                data->mNpcStats.getSkill (ESM::Skill::Axe).setBase (ref->mBase->mData.skills.blunt);
                data->mNpcStats.getSkill (ESM::Skill::Spear).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::Athletics).setBase (ref->mBase->mData.skills.athletics);
                data->mNpcStats.getSkill (ESM::Skill::Enchant).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::Destruction).setBase (ref->mBase->mData.skills.destruction);
                data->mNpcStats.getSkill (ESM::Skill::Alteration).setBase (ref->mBase->mData.skills.alteration);
                data->mNpcStats.getSkill (ESM::Skill::Illusion).setBase (ref->mBase->mData.skills.illusion);
                data->mNpcStats.getSkill (ESM::Skill::Conjuration).setBase (ref->mBase->mData.skills.conjuration);
                data->mNpcStats.getSkill (ESM::Skill::Mysticism).setBase (ref->mBase->mData.skills.mysticism);
                data->mNpcStats.getSkill (ESM::Skill::Restoration).setBase (ref->mBase->mData.skills.restoration);
                data->mNpcStats.getSkill (ESM::Skill::Alchemy).setBase (ref->mBase->mData.skills.alchemy);
                data->mNpcStats.getSkill (ESM::Skill::Unarmored).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::Security).setBase (ref->mBase->mData.skills.security);
                data->mNpcStats.getSkill (ESM::Skill::Sneak).setBase (ref->mBase->mData.skills.sneak);
                data->mNpcStats.getSkill (ESM::Skill::Acrobatics).setBase (ref->mBase->mData.skills.acrobatics);
                data->mNpcStats.getSkill (ESM::Skill::LightArmor).setBase (ref->mBase->mData.skills.lightArmor);
                data->mNpcStats.getSkill (ESM::Skill::ShortBlade).setBase (ref->mBase->mData.skills.blade);
                data->mNpcStats.getSkill (ESM::Skill::Marksman).setBase (ref->mBase->mData.skills.marksman);
                data->mNpcStats.getSkill (ESM::Skill::Mercantile).setBase (ref->mBase->mData.skills.mercantile);
                data->mNpcStats.getSkill (ESM::Skill::Speechcraft).setBase (ref->mBase->mData.skills.speechcraft);
                data->mNpcStats.getSkill (ESM::Skill::HandToHand).setBase (ref->mBase->mData.skills.handToHand);


                data->mNpcStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mData.attribs.strength);
                data->mNpcStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mData.attribs.intelligence);
                data->mNpcStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mData.attribs.willpower);
                data->mNpcStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mData.attribs.agility);
                data->mNpcStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mData.attribs.speed);
                data->mNpcStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mData.attribs.endurance);
                data->mNpcStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mData.attribs.personality);
                data->mNpcStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mData.attribs.luck);

                // FIXME: for testing
                data->mNpcStats.setHealth (/*(float)ref->mBase->mData.health*/50.f); // uint32 to float
                data->mNpcStats.setMagicka (ref->mBase->mBaseConfig.tes4.baseSpell);
                data->mNpcStats.setFatigue (/*ref->mBase->mBaseConfig.tes4.fatigue*/ 20);

                data->mNpcStats.setLevel(ref->mBase->mBaseConfig.tes4.levelOrOffset);
                data->mNpcStats.setBaseDisposition(0); // see http://www.uesp.net/wiki/Oblivion:Disposition
                // Fame is probably a global (GLOB)
                data->mNpcStats.setReputation(0); // see http://www.uesp.net/wiki/Oblivion:Fame

                data->mNpcStats.setNeedRecalcDynamicStats(false);
            }
            else // FIXME autocalc
            {
                gold = ref->mBase->mBaseConfig.tes4.barterGold;

                // FIXME: for testing
                data->mNpcStats.setHealth (/*ref->mBase->mData.health*/ 50.f); // FIXME: uint32 to float
                data->mNpcStats.setMagicka (ref->mBase->mBaseConfig.tes4.baseSpell);
                data->mNpcStats.setFatigue (/*ref->mBase->mBaseConfig.tes4.fatigue*/ 20);

                for (int i=0; i<3; ++i)
                    data->mNpcStats.setDynamic (i, 10);

                data->mNpcStats.setLevel(ref->mBase->mBaseConfig.tes4.levelOrOffset);
                data->mNpcStats.setBaseDisposition(0);
                data->mNpcStats.setReputation(0);

                //autoCalculateAttributes(ref->mBase, data->mNpcStats);
                //autoCalculateSkills(ref->mBase, data->mNpcStats, ptr);

                data->mNpcStats.setNeedRecalcDynamicStats(true);
            }
#if 0
            // race powers
            const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
            for (std::vector<std::string>::const_iterator iter (race->mPowers.mList.begin());
                iter!=race->mPowers.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                    std::cerr << "Warning: ignoring nonexistent race power '" << *iter << "' on NPC '" << ref->mBase->mId << "'" << std::endl;
            }

#endif
#if 0
            if (!ref->mBase->mFaction.empty())
            {
                static const int iAutoRepFacMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepFacMod")->getInt();
                static const int iAutoRepLevMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepLevMod")->getInt();
                int rank = ref->mBase->getFactionRank();

                data->mNpcStats.setReputation(iAutoRepFacMod * (rank+1) + iAutoRepLevMod * (data->mNpcStats.getLevel()-1));
            }
#endif
            //const ESM4::Npc *npc = ptr.get<ESM4::Npc>()->mBase;
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Npc* npc = ref->mBase;
            std::vector<ESM::AIPackage> aiPackages;
            for (std::size_t i = 0; i < npc->mAIPackages.size(); ++i)
            {
                const ESM4::AIPackage* pack = store.getForeign<ESM4::AIPackage>().search(npc->mAIPackages[i]);
                if (pack)
                {
                    ESM::AIPackage aiPackage;
                    aiPackage.mCellName = "";

                    // eat, sleep, etc not yet supported
                    switch (pack->mData.type)
                    {
                        case 1:
                        {
                            aiPackage.mType = ESM::AI_Follow;
                            //aiPackages.push_back(aiPackage);
                            break;
                        }
                        case 2:
                        {
                            aiPackage.mType = ESM::AI_Escort;
                            //aiPackage.mTarget = ""
        //float   mX, mY, mZ;
        //short   mDuration;
        //NAME32  mId;
        //short   mUnk;
                            aiPackage.mCellName = ""; // FIXME
                            //aiPackages.push_back(aiPackage);
                            break;
                        }
                        case 5:
                        {
                            aiPackage.mType = ESM::AI_Wander;
                            //aiPackage.mWander.mDistance = // short
                            //aiPackage.mWander.mDuration = // short
                            //aiPackage.mWander.mTimeOfDay = // uchar
                            //aiPackage.mWander.mIdle[0] = // uchar
                            //aiPackage.mWander.mShouldRepeat = // uchar

                            //aiPackages.push_back(aiPackage);
                            break;
                        }
                        case 6:
                        {
                            aiPackage.mType = ESM::AI_Travel;
                            //aiPackage.mTravel =
        //float   mX, mY, mZ;
        //int     mUnk;
                            //aiPackages.push_back(aiPackage);
                            break;
                        }
                        case 9:
                        {
                            aiPackage.mType = ESM::AI_Activate;
                            //aiPackage.mActivate =
        //NAME32 mName;
        //unsigned char mUnk;
                            //aiPackages.push_back(aiPackage);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
#if 0
            data->mNpcStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                {
                    /// \todo add option to make this a fatal error message pop-up, but default to warning for vanilla compatibility
                    std::cerr << "Warning: ignoring nonexistent spell '" << *iter << "' on NPC '" << ref->mBase->mId << "'" << std::endl;
                }
            }
#endif

            // inventory
            // setting ownership is used to make the NPC auto-equip his initial equipment only, and not bartered items

            ESM::InventoryList inventory;
            int gameType = data->mGameType;

            if (gameType == 2) // TES5
            {
                // add default outfit before any other inventory items
                // FIXME: should find a way to treat the outfit as a set rather than individual items
                ESM4::FormId outfitId = ref->mBase->mDefaultOutfit;
                if (outfitId)
                {
                    const ESM4::Outfit* outfit = store.getForeign<ESM4::Outfit>().search(outfitId);
                    if (outfit)
                    {
                        for (std::size_t i = 0; i < outfit->mInventory.size(); ++i)
                        {
                            // get ARMO or LVLI
                            // FIXME: search ESMStore?
                            ESM::ContItem item;
                            item.mCount = 1;
                            item.mItem.assign(ESM4::formIdToString(outfit->mInventory[i]));

                            inventory.mList.push_back(item);
                        }
                    }
                }
            }

            // make ESM4 inventory look like ESM::InventoryList so that we can pass it to
            // mInventoryStore.fill()
            //ESM::InventoryList inventory;
            for (unsigned int i = 0; i < ref->mBase->mInventory.size(); ++i)
            {
                ESM::ContItem item;
                item.mCount = ref->mBase->mInventory.at(i).count;
                item.mItem.assign(ESM4::formIdToString(ref->mBase->mInventory.at(i).item)); // FIXME

                inventory.mList.push_back(item);
            }

            // this "fills" the inventory with iterators for quick access?
            data->mInventoryStore->fill(inventory, getId(ptr));

            data->mNpcStats.setGoldPool(gold);

            //int gameType = data->mGameType;

            // store the data
            ptr.getRefData().setCustomData (data.release());

            // assign some of the inventory to the equipment "slots"
            if (gameType == 0)
                static_cast<MWWorld::InventoryStoreTES4&>(getInventoryStore(ptr)).autoEquip(ptr);
            else if (gameType == 1)
                static_cast<MWWorld::InventoryStoreFO3&>(getInventoryStore(ptr)).autoEquip(ptr);
            else
                static_cast<MWWorld::InventoryStoreTES5&>(getInventoryStore(ptr)).autoEquip(ptr);
        }
    }

    std::string ForeignNpc::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    MWMechanics::Movement& ForeignNpc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 ForeignNpc::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mPosition);
        movement.mPosition[0] = 0.0f;
        movement.mPosition[1] = 0.0f;
        movement.mPosition[2] = 0.0f;
        return vec;
    }

    Ogre::Vector3 ForeignNpc::getRotationVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mRotation);
        movement.mRotation[0] = 0.0f;
        movement.mRotation[1] = 0.0f;
        movement.mRotation[2] = 0.0f;
        return vec;
    }

    float ForeignNpc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        return 1.f;
#if 0
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        static const float fEncumbranceStrMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fEncumbranceStrMult")->getFloat();
        return stats.getAttribute(0).getModified()*fEncumbranceStrMult;
#endif
    }

    float ForeignNpc::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        return 1.f;
#if 0
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

        MWMechanics::NpcStats &stats = getNpcStats(ptr);
        MWWorld::InventoryStore &invStore = getInventoryStore(ptr);

        float fUnarmoredBase1 = store.find("fUnarmoredBase1")->getFloat();
        float fUnarmoredBase2 = store.find("fUnarmoredBase2")->getFloat();
        int unarmoredSkill = stats.getSkill(ESM::Skill::Unarmored).getModified();

        int ratings[MWWorld::InventoryStore::Slots];
        for(int i = 0;i < MWWorld::InventoryStore::Slots;i++)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getTypeName() != typeid(ESM::Armor).name())
            {
                // unarmored
                ratings[i] = static_cast<int>((fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill));
            }
            else
            {
                ratings[i] = it->getClass().getEffectiveArmorRating(*it, ptr);
            }
        }

        float shield = stats.getMagicEffects().get(ESM::MagicEffect::Shield).getMagnitude();

        return ratings[MWWorld::InventoryStore::Slot_Cuirass] * 0.3f
                + (ratings[MWWorld::InventoryStore::Slot_CarriedLeft] + ratings[MWWorld::InventoryStore::Slot_Helmet]
                    + ratings[MWWorld::InventoryStore::Slot_Greaves] + ratings[MWWorld::InventoryStore::Slot_Boots]
                    + ratings[MWWorld::InventoryStore::Slot_LeftPauldron] + ratings[MWWorld::InventoryStore::Slot_RightPauldron]
                    ) * 0.1f
                + (ratings[MWWorld::InventoryStore::Slot_LeftGauntlet] + ratings[MWWorld::InventoryStore::Slot_RightGauntlet])
                    * 0.05f
                + shield;
#endif
    }

    float ForeignNpc::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::NpcStats &stats = getNpcStats(ptr);

        // According to UESP, inventory weight is ignored in werewolf form. Does that include
        // feather and burden effects?
        float weight = 0.0f;
        if(!stats.isWerewolf())
        {
            weight  = getContainerStore(ptr).getWeight();
            weight -= stats.getMagicEffects().get(ESM::MagicEffect::Feather).getMagnitude();
            weight += stats.getMagicEffects().get(ESM::MagicEffect::Burden).getMagnitude();
            if(weight < 0.0f)
                weight = 0.0f;
        }

        return weight;
    }

    int ForeignNpc::getBaseFightRating (const MWWorld::Ptr& ptr) const
    {
        //MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>(); // currently unused
        //return ref->mBase->mAiData.mFight;
        return 5; // FIXME
    }

    bool ForeignNpc::isBipedal(const MWWorld::Ptr &ptr) const
    {
        return true;
    }

    std::string ForeignNpc::getPrimaryFaction (const MWWorld::Ptr& ptr) const
    {
        //MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>(); // currently unused
        //return ref->mBase->mFaction;
        return "foreign faction"; // FIXME
    }

    int ForeignNpc::getPrimaryFactionRank (const MWWorld::Ptr& ptr) const
    {
        //MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>(); // currently unused
        //return ref->mBase->getFactionRank();
        return 10; // FIXME
    }

    void ForeignNpc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignNpc);

        registerClass (typeid (ESM4::Npc).name(), instance);
    }

    MWWorld::Ptr ForeignNpc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Npc>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_NPC_);

        //return std::move(newPtr);
        return newPtr;
    }
}
