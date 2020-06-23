#include "foreigncreature.hpp"

#include <extern/esm4/crea.hpp>
#include <extern/esm4/formid.hpp> // FIXME mainly for debugging

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystoretes4.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/levelledlist.hpp"

namespace
{
    struct ForeignCreatureCustomData : public MWWorld::CustomData
    {
        MWMechanics::CreatureStats mCreatureStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStoreTES4 mInventoryStore;

        MWWorld::Ptr mPlaced;

        ForeignCreatureCustomData() : mPlaced(MWWorld::Ptr()) {}
        ~ForeignCreatureCustomData()
        {
        }

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *ForeignCreatureCustomData::clone() const
    {
        return new ForeignCreatureCustomData (*this);
    }
}

namespace MWClass
{
    std::string ForeignCreature::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Creature>()->mBase->mEditorId;
    }

    void ForeignCreature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        // this might be a levelled actor (e.g. FO3) - check if the model is empty
        const ESM4::Creature *creature = ptr.get<ESM4::Creature>()->mBase;
        if (creature && creature->mBaseTemplate != 0
                && (creature->mModel.empty() || creature->mModel == "marker_creature.nif"))
        {
            //std::cout << npc->mEditorId << std::endl; // FIXME

            // FIXME: save inventory details (and baseconfig?) to update later

            std::string id = MWMechanics::getFO3LevelledCreature(creature);

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
                ForeignCreatureCustomData *data
                    = dynamic_cast<ForeignCreatureCustomData*>(ptr.getRefData().getCustomData());

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
        {
            //MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>(); // currently unused

            MWRender::Actors& actors = renderingInterface.getActors();
            actors.insertCreature(ptr, model, false/*(ref->mBase->mFlags & ESM::Creature::Weapon) != 0*/);
        }
    }

    void ForeignCreature::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(model.empty())
            return;

        const ESM4::Creature *creature = ptr.get<ESM4::Creature>()->mBase;
        if (creature && creature->mBaseTemplate != 0
                && (creature->mModel.empty() || creature->mModel == "marker_creature.nif"))
        {
        }
        else
        {
            physics.addActor(ptr, model);
            //if (getCreatureStats(ptr).isDead())
                //MWBase::Environment::get().getWorld()->enableActorCollision(ptr, false);
            MWBase::Environment::get().getMechanicsManager()->add(ptr);
        }
    }

    std::string ForeignCreature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
#if 0
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
#endif
    }

    std::string ForeignCreature::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignCreature::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<ForeignCreatureCustomData> data(new ForeignCreatureCustomData);

            MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

            // creature stats (no autocalc)
            int gold = ref->mBase->mBaseConfig.tes4.barterGold;

            data->mCreatureStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mData.attribs.strength);
            data->mCreatureStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mData.attribs.intelligence);
            data->mCreatureStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mData.attribs.willpower);
            data->mCreatureStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mData.attribs.agility);
            data->mCreatureStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mData.attribs.speed);
            data->mCreatureStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mData.attribs.endurance);
            data->mCreatureStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mData.attribs.personality);
            data->mCreatureStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mData.attribs.luck);

            data->mCreatureStats.setHealth (/*float(ref->mBase->mData.health)*/ 50.f); // FIXME: temp testing
            data->mCreatureStats.setMagicka (ref->mBase->mBaseConfig.tes4.baseSpell);
            data->mCreatureStats.setFatigue (/*ref->mBase->mBaseConfig.tes4.fatigue*/ 20); // FIXME: for testing

            data->mCreatureStats.setLevel(ref->mBase->mBaseConfig.tes4.levelOrOffset);

            data->mCreatureStats.setNeedRecalcDynamicStats(false);

            // inventory
            // setting ownership is used to make the NPC auto-equip his initial equipment only, and not bartered items
            ESM::InventoryList inventory;
            for (unsigned int i = 0; i < ref->mBase->mInventory.size(); ++i)
            {
                ESM::ContItem item;
                item.mCount = ref->mBase->mInventory.at(i).count;
                item.mItem.assign(ESM4::formIdToString(ref->mBase->mInventory.at(i).item)); // FIXME

                inventory.mList.push_back(item);
            }

            data->mInventoryStore.fill(inventory, getId(ptr));

            data->mCreatureStats.setGoldPool(gold);

            // store
            ptr.getRefData().setCustomData (data.release());

            static_cast<MWWorld::InventoryStoreTES4&>(getInventoryStore(ptr)).autoEquip(ptr);
        }
    }

    MWWorld::InventoryStore& ForeignCreature::getInventoryStore (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWWorld::InventoryStoreTES4& ForeignCreature::getInventoryStoreTES4 (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWMechanics::CreatureStats& ForeignCreature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }

    MWMechanics::Movement& ForeignCreature::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    void ForeignCreature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignCreature);

        registerClass (typeid (ESM4::Creature).name(), instance);
    }

    MWWorld::Ptr ForeignCreature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Creature>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_CREA);

        //return std::move(newPtr);
        return newPtr;
    }
}
