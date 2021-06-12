#include "esmstore.hpp"

#include <set>
#include <iostream>

#include <boost/filesystem/operations.hpp>

#include <extern/esm4/common.hpp>
#include <extern/esm4/formid.hpp>
#include <extern/esm4/refr.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/esm4reader.hpp>

namespace MWWorld
{

static bool isCacheableRecord(int id)
{
    if (id == ESM::REC_ACTI || id == ESM::REC_ALCH || id == ESM::REC_APPA || id == ESM::REC_ARMO ||
        id == ESM::REC_BOOK || id == ESM::REC_CLOT || id == ESM::REC_CONT || id == ESM::REC_CREA ||
        id == ESM::REC_DOOR || id == ESM::REC_INGR || id == ESM::REC_LEVC || id == ESM::REC_LEVI ||
        id == ESM::REC_LIGH || id == ESM::REC_LOCK || id == ESM::REC_MISC || id == ESM::REC_NPC_ ||
        id == ESM::REC_PROB || id == ESM::REC_REPA || id == ESM::REC_STAT || id == ESM::REC_WEAP ||
        id == ESM::REC_BODY)
    {
        return true;
    }
    return false;
}

// FIXME: not sure what records qualify as 'cacheable' - guess is that they are referenceable records
//        (if so why aren't REC_GRAS, REC_TREE, REC_FLOR, REC_FURN, etc included?)
static bool isCacheableForeignRecord(int id)
{
    if (// id == ESM4::REC_SOUN || /* sound */
     // id == ESM4::REC_ACTI || /* activator */
        id == ESM4::REC_APPA || /* apparatus */
        id == ESM4::REC_ARMO || /* armor */
        id == ESM4::REC_BOOK || /* book */
        id == ESM4::REC_CLOT || /* clothing */
     // id == ESM4::REC_CONT || /* container */
     // id == ESM4::REC_DOOR || /* door */
        id == ESM4::REC_INGR || /* ingredient */
     // id == ESM4::REC_LIGH || /* light */
     // id == ESM4::REC_MISC || /* misc item */
     // id == ESM4::REC_STAT || /* static */
      //id == ESM4::REC_GRAS || /* grass (not in FONV dummy cell) */
     // id == ESM4::REC_TREE || /* tree */
      //id == ESM4::REC_FLOR || /* flora */
     // id == ESM4::REC_FURN || /* furniture */
//FO3   id == ESM4::REC_WEAP || /* weapon */
      //id == ESM4::REC_AMMO || /* ammo (not in FONV dummy cell) */
     // id == ESM4::REC_NPC_ || /* npc */
     // id == ESM4::REC_CREA || /* creature */
//TES4  id == ESM4::REC_LVLC || /* lvlcreature */
        id == ESM4::REC_SLGM || /* soulgem */
//FO3   id == ESM4::REC_KEYM || /* key */
//FO3   id == ESM4::REC_ALCH || /* potion */
      //id == ESM4::REC_SBSP || /* subspace (not in FONV dummy cell) */
        id == ESM4::REC_SGST || /* sigilstone */
        id == ESM4::REC_LVLI || /* levelled item */
        id == ESM4::REC_LVLN || /* levelled actor */
     // id == ESM4::REC_IDLM || /* idle marker */
     // id == ESM4::REC_MSTT || /* movable static */
      //id == ESM4::REC_TXST || /* texture set (not in FONV dummy cell) */
        id == ESM4::REC_SCRL || /* scroll */
        id == ESM4::REC_ARMA || /* armor addon */
     // id == ESM4::REC_TERM || /* terminal */
     // id == ESM4::REC_TACT || /* talking activator */
        id == ESM4::REC_NOTE || /* note */
      //id == ESM4::REC_ASPC || /* acoustic space (not in FONV dummy cell) */
     // id == ESM4::REC_PWAT || /* placeable water */
      //id == ESM4::REC_SCOL || /* static collection (not in FONV dummy cell) */
        id == ESM4::REC_IMOD    /* item mod */
        )
    {
        return true;
    }
    return false;
}

void ESMStore::load(ESM::ESMReader& esm, Loading::Listener* listener)
{
    listener->setProgressRange(1000);

    ESM::Dialogue *dialogue = 0;

    int esmVer = esm.getVer();
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_17;
    bool isFONV = esmVer == ESM4::VER_132 || esmVer == ESM4::VER_133 || esmVer == ESM4::VER_134;

    // FIXME: temporary workaround
    if (!(isTes4 || isTes5 || isFONV)) // MW only
    {
        // Land texture loading needs to use a separate internal store for each plugin.
        // We set the number of plugins here to avoid continual resizes during loading,
        // and so we can properly verify if valid plugin indices are being passed to the
        // LandTexture Store retrieval methods.
        mLandTextures.resize(esm.getGlobalReaderList()->size()); // FIXME: size should be for MW only
    }

    // FIXME: for TES4/TES5 whether a dependent file is loaded is already checked in
    // ESM4::Reader::updateModIndices() which is called in EsmLoader::load() before this
    if (!(isTes4 || isTes5 || isFONV)) // MW only
    {
        /// \todo Move this to somewhere else. ESMReader?
        // Cache parent esX files by tracking their indices in the global list of
        //  all files/readers used by the engine. This will greaty accelerate
        //  refnumber mangling, as required for handling moved references.
        const std::vector<ESM::Header::MasterData> &masters = esm.getGameFiles();
        std::vector<ESM::ESMReader*> *allPlugins = esm.getGlobalReaderList();
        for (size_t j = 0; j < masters.size(); j++) {
            ESM::Header::MasterData &mast = const_cast<ESM::Header::MasterData&>(masters[j]);
            std::string fname = mast.name;
            int index = ~0;
            for (int i = 0; i < esm.getIndex(); i++) {
                const std::string &candidate = allPlugins->at(i)->getContext().filename;
                std::string fnamecandidate = boost::filesystem::path(candidate).filename().string();
                if (Misc::StringUtils::ciEqual(fname, fnamecandidate)) {
                    index = i;
                    break;
                }
            }
            if (index == (int)~0) {
                // Tried to load a parent file that has not been loaded yet. This is bad,
                //  the launcher should have taken care of this.
                std::string fstring = "File " + esm.getName() + " asks for parent file " + masters[j].name
                    + ", but it has not been loaded yet. Please check your load order.";
                esm.fail(fstring);
            }
            mast.index = index;
        }
    }

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        if (isTes4 || isTes5 || isFONV)
        {
            ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
            reader.checkGroupStatus();

            loadTes4Group(esm);
            listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
            continue;
        }

        ESM::NAME n = esm.getRecName();
        esm.getRecHeader();

        // Look up the record type.
        std::map<int, StoreBase *>::iterator it = mStores.find(n.val);

        if (it == mStores.end()) {
            if (n.val == ESM::REC_INFO) {
                if (dialogue)
                {
                    dialogue->readInfo(esm, esm.getIndex() != 0);
                }
                else
                {
                    std::cerr << "error: info record without dialog" << std::endl;
                    esm.skipRecord();
                }
            } else if (n.val == ESM::REC_MGEF) {
                mMagicEffects.load (esm);
            } else if (n.val == ESM::REC_SKIL) {
                mSkills.load (esm);
            }
            else if (n.val==ESM::REC_FILT || n.val == ESM::REC_DBGP)
            {
                // ignore project file only records
                esm.skipRecord();
            }
            else {
                std::stringstream error;
                error << "Unknown record: " << n.toString();
                throw std::runtime_error(error.str());
            }
        } else {
            RecordId id = it->second->load(esm);
            if (id.mIsDeleted)
            {
                it->second->eraseStatic(id.mId);
                continue;
            }

            if (n.val==ESM::REC_DIAL) {
                dialogue = const_cast<ESM::Dialogue*>(mDialogs.find(id.mId));
            } else {
                dialogue = 0;
            }
        }
        listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
    }
}

// Can't use ESM4::Reader& as the parameter here because we need esm.hasMoreRecs() for
// checking an empty group followed by EOF
void ESMStore::loadTes4Group (ESM::ESMReader &esm)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    reader.getRecordHeader();
    const ESM4::RecordHeader& hdr = reader.hdr();

    if (hdr.record.typeId != ESM4::REC_GRUP)
        return loadTes4Record(esm);

    switch (hdr.group.type)
    {
        case ESM4::Grp_RecordType:
        {
            // FIXME: rewrite to workaround reliability issue
            if (hdr.group.label.value == ESM4::REC_NAVI || hdr.group.label.value == ESM4::REC_WRLD ||
                hdr.group.label.value == ESM4::REC_REGN || hdr.group.label.value == ESM4::REC_STAT ||
                hdr.group.label.value == ESM4::REC_ANIO || hdr.group.label.value == ESM4::REC_CONT ||
                hdr.group.label.value == ESM4::REC_MISC || hdr.group.label.value == ESM4::REC_ACTI ||
                hdr.group.label.value == ESM4::REC_ARMO || hdr.group.label.value == ESM4::REC_NPC_ ||
                hdr.group.label.value == ESM4::REC_FLOR || hdr.group.label.value == ESM4::REC_GRAS ||
                hdr.group.label.value == ESM4::REC_TREE || hdr.group.label.value == ESM4::REC_LIGH ||
                hdr.group.label.value == ESM4::REC_BOOK || hdr.group.label.value == ESM4::REC_FURN ||
                hdr.group.label.value == ESM4::REC_SOUN || hdr.group.label.value == ESM4::REC_WEAP ||
                hdr.group.label.value == ESM4::REC_DOOR || hdr.group.label.value == ESM4::REC_AMMO ||
                hdr.group.label.value == ESM4::REC_CLOT || hdr.group.label.value == ESM4::REC_ALCH ||
                hdr.group.label.value == ESM4::REC_APPA || hdr.group.label.value == ESM4::REC_INGR ||
                hdr.group.label.value == ESM4::REC_SGST || hdr.group.label.value == ESM4::REC_SLGM ||
                hdr.group.label.value == ESM4::REC_KEYM || hdr.group.label.value == ESM4::REC_HAIR ||
                hdr.group.label.value == ESM4::REC_EYES || hdr.group.label.value == ESM4::REC_CELL ||
                hdr.group.label.value == ESM4::REC_CREA || hdr.group.label.value == ESM4::REC_LVLC ||
                hdr.group.label.value == ESM4::REC_LVLI || hdr.group.label.value == ESM4::REC_MATO ||
                hdr.group.label.value == ESM4::REC_IDLE || hdr.group.label.value == ESM4::REC_LTEX ||
                hdr.group.label.value == ESM4::REC_RACE || hdr.group.label.value == ESM4::REC_SBSP ||
                hdr.group.label.value == ESM4::REC_LVLN || hdr.group.label.value == ESM4::REC_IDLM ||
                hdr.group.label.value == ESM4::REC_MSTT || hdr.group.label.value == ESM4::REC_TXST ||
                hdr.group.label.value == ESM4::REC_SCRL || hdr.group.label.value == ESM4::REC_ARMA ||
                hdr.group.label.value == ESM4::REC_HDPT || hdr.group.label.value == ESM4::REC_TERM ||
                hdr.group.label.value == ESM4::REC_TACT || hdr.group.label.value == ESM4::REC_NOTE ||
                hdr.group.label.value == ESM4::REC_SCPT || hdr.group.label.value == ESM4::REC_LGTM ||
                hdr.group.label.value == ESM4::REC_DIAL || hdr.group.label.value == ESM4::REC_INFO ||
                hdr.group.label.value == ESM4::REC_QUST || hdr.group.label.value == ESM4::REC_PACK ||
                hdr.group.label.value == ESM4::REC_ASPC || hdr.group.label.value == ESM4::REC_IMOD ||
                hdr.group.label.value == ESM4::REC_PWAT || hdr.group.label.value == ESM4::REC_SCOL ||
                hdr.group.label.value == ESM4::REC_MUSC || hdr.group.label.value == ESM4::REC_ALOC ||
                hdr.group.label.value == ESM4::REC_MSET || hdr.group.label.value == ESM4::REC_DOBJ ||
                hdr.group.label.value == ESM4::REC_SNDR || hdr.group.label.value == ESM4::REC_OTFT ||
                hdr.group.label.value == ESM4::REC_BPTD || hdr.group.label.value == ESM4::REC_GLOB
                )
            {
                reader.saveGroupStatus();
                loadTes4Group(esm);
            }
            else
            {
                // Skip groups that are of no interest (for now).
                //  GMST CLAS FACT SKIL MGEF ENCH SPEL BSGN WTHR CLMT
                //  CSTY LSCR LVSP WATR EFSH

                // FIXME: The label field of a group is not reliable, so we will need to check here as well
                std::cout << "skipping group... " << ESM4::printLabel(hdr.group.label, hdr.group.type) << std::endl;
                reader.skipGroup();
                return;
            }

            break;
        }
        case ESM4::Grp_CellChild:
        case ESM4::Grp_WorldChild:
        case ESM4::Grp_TopicChild:
        {
            reader.saveGroupStatus();
            if (!esm.hasMoreRecs())
                return; // may have been an empty group followed by EOF

            loadTes4Group(esm);

            break;
        }
        case ESM4::Grp_CellPersistentChild:
        {
            reader.adjustGRUPFormId();  // not needed or even shouldn't be done? (only labels anyway)
            reader.saveGroupStatus();
//#if 0
            // This test shows that Oblivion.esm does not have any persistent cell child
            // groups under exterior world sub-block group.  Haven't checked other files yet.
            if (reader.grp(0).type == ESM4::Grp_CellPersistentChild &&
                reader.grp(1).type == ESM4::Grp_CellChild &&
                !(reader.grp(2).type == ESM4::Grp_WorldChild || reader.grp(2).type == ESM4::Grp_InteriorSubCell))
                std::cout << "Unexpected persistent child group in exterior subcell" << std::endl;
//#endif
            // If hdr.group.type == ESM4::Grp_CellPersistentChild, we are about to
            // partially load the persistent records such as REFR, ACHR and ACRE, if any.
            // (well, actually only examining for doors for the moment)
            //
            // The records from other groups are skipped as per below.

            if (reader.grp(1).type != ESM4::Grp_CellChild)
            {
                // if this group (CellPersistentChild) was empty, reader.saveGroupStatus()
                // above would have popped the GroupStack and the previous group will no longer
                // be CellChild

                // FIXME: come up with a better test, e.g. check if empty somehow? check the
                // size of the stack before/after? etc
                std::cout << "Grp_CellPersistentChild parent type " << reader.grp(1).type << std::endl; // FIXME

                break; // return instead?
                //throw std::runtime_error ("Unexpected group hierarchy in CellPersistentChild");
            }

            // NOTE: dummy cell no longer loaded here
            if (reader.grp(2).type == ESM4::Grp_WorldChild)
            {
                loadTes4Group(esm); // FIXME: testing
                //reader.skipGroup(); // FIXME: testing
                break;
            }
            else if (reader.grp(2).type == ESM4::Grp_InteriorSubCell)
            {
                loadTes4Group(esm); // interior
                break;
            }
            else
                throw std::runtime_error ("Unexpected group hierarchy in CellPersistentChild");

            break;
        }
        case ESM4::Grp_CellVisibleDistChild:
        {
            // these are loaded later along with cell temporary child group
            reader.skipGroup();
            break;
        }
        case ESM4::Grp_CellTemporaryChild:
        {
            mForeignCells.updateRefrEstimate(esm); // for loading bar

            // NOTE: preload strategy and persistent records
            //
            // Current strategy defers loading of "temporary" references and other records
            // (land and pathgrid) until they are needed.
            //
            // The "persistent" records need to be loaded up front, however.  This is to allow,
            // for example, doors to work.  A door reference will have a FormId of the
            // destination door FormId.  But we have no way of knowing to which cell the
            // destination FormId belongs until that cell and that reference is loaded.
            //
            // For worldspaces the persistent records are usully (always?) stored in a dummy
            // cell under a "world child" group.  It may be possible to skip the whole "cell
            // child" group without scanning for persistent records.  See above short test.
            reader.skipGroup();
            break;
        }
        case ESM4::Grp_ExteriorCell:
        case ESM4::Grp_ExteriorSubCell:
        case ESM4::Grp_InteriorCell:
        case ESM4::Grp_InteriorSubCell:
        {
            reader.saveGroupStatus();
            loadTes4Group(esm);

            break;
        }
        default:
            std::cout << "unknown group..." << std::endl; // FIXME, should throw?
            reader.skipGroup();
            break;
    }

    return;
}

void ESMStore::loadTes4Record (ESM::ESMReader& esm)
{
    // Assumes that the reader has just read the record header only.
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
    const ESM4::RecordHeader& hdr = reader.hdr();

    // TODO: checking for the store isn't really providing a value
    std::map<int, StoreBase *>::iterator it = mForeignStores.find(hdr.record.typeId);
    if (it == mForeignStores.end())
    {
        switch (hdr.record.typeId)
        {
            case ESM4::REC_INFO:
            {
                if (mCurrentDialogue)
                {
                    reader.getRecordData();

                    // FIXME: how to detect if not the first master ESM?
                    // maybe use reader.mCtx.modIndex
                    mCurrentDialogue->loadInfo(reader, false/*esm.getIndex() != 0*/);
                }
                else
                {
                    std::cerr << "error: foreign info record without dialog" << std::endl;
                    reader.skipRecordData();
                }

                return;
            }
            case ESM4::REC_WRLD:
            case ESM4::REC_CELL:
            case ESM4::REC_REGN:
            case ESM4::REC_ROAD:
            case ESM4::REC_LAND: // can ignore, handled in CellStore
            case ESM4::REC_REFR:
            case ESM4::REC_ACHR:
            case ESM4::REC_ACRE:
            case ESM4::REC_NAVI: // TODO
            case ESM4::REC_IDLE: // TODO
            case ESM4::REC_CCRD: // TODO
            case ESM4::REC_CMNY: // TODO
            case ESM4::REC_OTFT:
            {
                break; // handled below
            }
            default:
            {
                std::cout << "Unsupported TES4 record type: " + ESM4::printName(hdr.record.typeId) << std::endl;
                reader.skipRecordData();

                return;
            }
        }
    }

    mCurrentDialogue = nullptr;

    ForeignId id;
    switch (hdr.record.typeId)
    {
        // GMST, CLAS, FACT
        case ESM4::REC_HDPT: reader.getRecordData(); id = mHeadParts.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_HDPT; break;
        case ESM4::REC_GLOB: reader.getRecordData(); id = mForeignGlobals.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_GLOB; break;
        case ESM4::REC_HAIR: reader.getRecordData(); id = mForeignHairs.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_HAIR; break;
        case ESM4::REC_EYES: reader.getRecordData(); id = mForeignEyesSet.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_EYES; break;
        case ESM4::REC_RACE: reader.getRecordData(); id = mForeignRaces.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_RACE; break;
        case ESM4::REC_BPTD: reader.getRecordData(); id = mForeignBodyParts.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_BPTD; break;
        // SKIL, MGEF
        case ESM4::REC_LTEX: reader.getRecordData(); id = mForeignLandTextures.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_LTEX; break;
        case ESM4::REC_SCPT: reader.getRecordData(); id = mForeignScripts.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_SCPT; break;
        // ENCH, SPEL, BSGN
        // ---- referenceables start
        case ESM4::REC_SOUN: reader.getRecordData(); id = mForeignSounds.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_SOUN; break;
        case ESM4::REC_ACTI: reader.getRecordData(); id = mForeignActivators.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_ACTI; break;
        case ESM4::REC_APPA: reader.getRecordData(); mForeignApparatuses.loadForeign(reader); break;
        case ESM4::REC_ARMO: reader.getRecordData(); id = mForeignArmors.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_ARMO; break;
        case ESM4::REC_BOOK: reader.getRecordData(); mForeignBooks.loadForeign(reader); break;
        case ESM4::REC_CLOT: reader.getRecordData(); mForeignClothes.loadForeign(reader); break;
        case ESM4::REC_CONT: reader.getRecordData(); id = mForeignContainers.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_CONT; break;
        case ESM4::REC_DOOR: reader.getRecordData(); id = mForeignDoors.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_DOOR; break;
        case ESM4::REC_INGR: reader.getRecordData(); mForeignIngredients.loadForeign(reader); break;
        case ESM4::REC_LIGH: reader.getRecordData(); id = mForeignLights.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_LIGH; break;
        case ESM4::REC_MISC: reader.getRecordData(); id = mForeignMiscItems.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_MISC; break;
        case ESM4::REC_STAT: reader.getRecordData(); id = mForeignStatics.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_STAT; break;
        case ESM4::REC_GRAS: reader.getRecordData(); id = mForeignGrasses.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_GRAS; break;
        case ESM4::REC_TREE: reader.getRecordData(); id = mForeignTrees.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_TREE; break;
        case ESM4::REC_FLOR: reader.getRecordData(); id = mForeignFloras.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_FLOR; break;
        case ESM4::REC_FURN: reader.getRecordData(); id = mForeignFurnitures.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_FURN; break;
        case ESM4::REC_WEAP: reader.getRecordData(); id = mForeignWeapons.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_WEAP; break;
        case ESM4::REC_AMMO: reader.getRecordData(); id = mAmmunitions.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_AMMO; break;
        case ESM4::REC_NPC_: reader.getRecordData(); id = mForeignNpcs.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_NPC_; break;
        case ESM4::REC_CREA: reader.getRecordData(); id = mForeignCreatures.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_CREA; break;
        case ESM4::REC_LVLC: reader.getRecordData(); id = mLevelledCreatures.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_LVLC; break;
        case ESM4::REC_SLGM: reader.getRecordData(); mSoulGems.loadForeign(reader); break;
        case ESM4::REC_KEYM: reader.getRecordData(); id = mForeignKeys.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_KEYM; break;
        case ESM4::REC_ALCH: reader.getRecordData(); id = mForeignPotions.loadForeign(reader);
                             mForeignIds[id.mId] = ESM4::REC_ALCH; break;
        case ESM4::REC_SBSP: reader.getRecordData(); id = mSubSpaces.loadForeign(reader); // 9
                             mForeignIds[id.mId] = ESM4::REC_SBSP; break;
        case ESM4::REC_SGST: reader.getRecordData(); mSigilStones.loadForeign(reader); break;
        case ESM4::REC_LVLI: reader.getRecordData(); mLevelledItems.loadForeign(reader); break; // 12
        case ESM4::REC_LVLN: reader.getRecordData(); mLevelledNpcs.loadForeign(reader); break; // 8
        case ESM4::REC_IDLM: reader.getRecordData(); id = mIdleMarkers.loadForeign(reader); // 10
                             mForeignIds[id.mId] = ESM4::REC_IDLM; break;
        case ESM4::REC_MSTT: reader.getRecordData(); id = mMovableStatics.loadForeign(reader); // 6
                             mForeignIds[id.mId] = ESM4::REC_MSTT; break;
        case ESM4::REC_TXST: reader.getRecordData(); id = mTextureSets.loadForeign(reader);   //1
                             mForeignIds[id.mId] = ESM4::REC_TXST; break;
        case ESM4::REC_SCRL: reader.getRecordData(); mForeignScrolls.loadForeign(reader); break;
        case ESM4::REC_ARMA: reader.getRecordData(); mArmorAddons.loadForeign(reader); break; // 13
        case ESM4::REC_TERM: reader.getRecordData(); id = mTerminals.loadForeign(reader); // 4
                             mForeignIds[id.mId] = ESM4::REC_TERM; break;
        case ESM4::REC_TACT: reader.getRecordData(); id = mTalkingActivators.loadForeign(reader); // 3
                             mForeignIds[id.mId] = ESM4::REC_TACT; break;
        case ESM4::REC_NOTE: reader.getRecordData(); mNotes.loadForeign(reader); break; // 11
        case ESM4::REC_ASPC: reader.getRecordData(); id = mAcousticSpaces.loadForeign(reader); // 2
                             mForeignIds[id.mId] = ESM4::REC_ASPC; break;
        case ESM4::REC_PWAT: reader.getRecordData(); id = mPlaceableWaters.loadForeign(reader); // 7
                             mForeignIds[id.mId] = ESM4::REC_PWAT; break;
        case ESM4::REC_SCOL: reader.getRecordData(); id = mStaticCollections.loadForeign(reader); // 5
                             mForeignIds[id.mId] = ESM4::REC_SCOL; break;
        case ESM4::REC_IMOD: reader.getRecordData(); mItemMods.loadForeign(reader); break;
        // ---- referenceables end
        // WTHR, CLMT
        case ESM4::REC_REGN: reader.getRecordData(); mForeignRegions.loadForeign(reader); break;
        case ESM4::REC_CELL:
        {
            // do not load the refs and just save the context (but the Cell record itself is loaded)
            // TODO: if we pass this pointer some loading might be possible (e.g. lighting templates)
            mForeignCells.preload(esm, mForeignWorlds);
            // FIXME: deal with deleted recods

            // FIXME testing only - uncomment below call to testPreload() to test
            // saving/restoring file contexts
            // also see ForeignCell::testPreload() and Store<MWWorld::ForeignCell>::testPreload()
            //mForeignCells.testPreload(esm);
            break;
        }
        case ESM4::REC_WRLD:
        {
            RecordId id = mForeignWorlds.load(esm); // FIXME: loadForeign
            if (id.mIsDeleted)
                mForeignWorlds.eraseStatic(id.mId);

            // will be followed by another CELL or a Cell Child GRUP
            break;
        }
        case ESM4::REC_DIAL:
        {
            reader.getRecordData();
            ForeignId id = mForeignDialogues.loadForeign(reader);

            mCurrentDialogue = const_cast<ForeignDialogue*>(mForeignDialogues.find(id.mId));

            break;
        }
        case ESM4::REC_QUST: reader.getRecordData(); mForeignQuests.loadForeign(reader);      break;
        // IDLE
        // CSTY, LSCR, LVSP
        case ESM4::REC_PACK: reader.getRecordData(); mForeignAIPackages.loadForeign(reader);  break;
        case ESM4::REC_ANIO: reader.getRecordData(); mForeignAnimObjs.loadForeign(reader);    break;
        // WATR, EFSH
        case ESM4::REC_REFR:
        {
            // this REFR must be in "Cell Persistent Child" group
            ESM4::Reference record;

            reader.getRecordData();
            record.load(reader);

            // FIXME: loading *all* references just to check for doors is highly inefficient
            if (record.mDoor.destDoor != 0)
            {
                std::pair<std::map<ESM4::FormId, ESM4::FormId>::iterator, bool> result
                    = mDoorDestCell.insert({ record.mFormId, reader.currCell() });

                // FIXME: detect duplicates?
            }
#if 0
            if ((record.mFlags & ESM4::Rec_VisDistant) != 0 && reader.getContext().groupStack.back().first.type != ESM4::Grp_CellVisibleDistChild)
            {
                std::string padding = ""; // FIXME: debugging only
                padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                std::cout << padding << "REFR " << record.mEditorId << " "
                          << ESM4::formIdToString(record.mFormId) << " visible dist" << std::endl;
            }

            if ((record.mFlags & ESM4::Rec_VisDistant) == 0 && reader.getContext().groupStack.back().first.type == ESM4::Grp_CellVisibleDistChild)
            {
                std::string padding = ""; // FIXME: debugging only
                padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                std::cout << padding << "REFR " << record.mEditorId << " "
                          << ESM4::formIdToString(record.mFormId) << " NOT visible dist" << std::endl;
            }
#endif
            // for loading bar (not working that well)
            mForeignCells.incrementRefrCount(esm);

            mForeignRefs.loadForeign(record);

            break;
        }
        case ESM4::REC_ACHR:
        {
            // this ACHR must be in "Cell Persistent Child" group
            ESM4::ActorCharacter record;

            reader.getRecordData();
            mForeignAchrs.loadForeign(reader);
            break;
        }
        case ESM4::REC_ACRE: // Oblivion only?
        {
            // this ACHE must be in "Cell Persistent Child" group
            ESM4::Reference record;
            reader.getRecordData();
            mForeignAcres.loadForeign(reader);

            break;
        }
#if 0
        // NOTE: LAND records are loaded later (for now) - see CellStore
        // not loaded here since LAND is in "Cell Temporary Child" group
        // TODO: verify LTEX formIds exist
        case ESM4::REC_LAND: reader.getRecordData(); mForeignLands.load(esm, mForeignCells); break;
        case ESM4::REC_NAVI: reader.getRecordData(); mNavigation.loadTes4(esm); break;
        case ESM4::REC_NAVM:
        {
            // FIXME: should update mNavMesh to indicate this record was deleted
            if ((hdr.record.flags & ESM4::Rec_Deleted) != 0)
            {
//FIXME: debug only
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "NAVM: deleted record id "
                          << std::hex << hdr.record.id << std::endl;
#endif
                reader.skipRecordData();
                break;
            }

            reader.getRecordData();
            mNavMesh.loadTes4(esm);
            break;
        }
        //
        // PGRD is handled in CellStore::loadTes4Record()
        // not loaded here since PGRD is in "Cell Temporary Child" group
        case ESM4::REC_PGRD: // Oblivion only?
#endif
        case ESM4::REC_ROAD: reader.getRecordData(); mRoads.loadForeign(reader); break; // Oblivion only
        case ESM4::REC_LGTM: reader.getRecordData(); mLightingTemplates.loadForeign(reader); break;
        case ESM4::REC_MUSC: reader.getRecordData(); mMusic.loadForeign(reader); break;
        case ESM4::REC_ALOC: reader.getRecordData(); mMediaLocCtlr.loadForeign(reader); break; // FONV
        case ESM4::REC_MSET: reader.getRecordData(); mMediaSet.loadForeign(reader); break; // FONV
        case ESM4::REC_DOBJ: reader.getRecordData(); mDefaultObj.loadForeign(reader); break; // FONV
        // FIXME: should only get loaded in CellStore::loadTes4Record()?
        case ESM4::REC_PGRE: reader.getRecordData(); mPlacedGrenades.loadForeign(reader); break;
        case ESM4::REC_SNDR: reader.getRecordData(); mSoundReferences.loadForeign(reader); break;
        case ESM4::REC_OTFT:
        {
            reader.getRecordData();
            mOutfits.loadForeign(reader);

            break;
        }
        //case ESM4::REC_REGN:
        case ESM4::REC_PHZD: // Skyrim only?
        case ESM4::REC_NAVM: case ESM4::REC_NAVI:
        case ESM4::REC_IDLE:
        case ESM4::REC_MATO:
        {
            //std::cout << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
            reader.skipRecordData();
            break;
        }
        default:
        {
            std::cout << "Unsupported TES4 record type: " + ESM4::printName(hdr.record.typeId) << std::endl;
            reader.skipRecordData();
        }
    }

    return;
}

void ESMStore::setUp()
{
    mIds.clear();

    std::map<int, StoreBase *>::iterator storeIt = mStores.begin();
    for (; storeIt != mStores.end(); ++storeIt)
    {
        storeIt->second->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            storeIt->second->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mIds[*record] = storeIt->first;
        }
    }

    mSkills.setUp();
    mMagicEffects.setUp();
    mAttributes.setUp();
    mDialogs.setUp();

// this does not always work for TES4 since sometimes we need to lookup some object on the fly in
// CellStore (e.g. to load visible dist group) - need to populate mForeignIds as we read each record
// such as STAT, TREE, NPC_, etc
    std::map<int, StoreBase *>::iterator foreignStoreIt = mForeignStores.begin();
    for (; foreignStoreIt != mForeignStores.end(); ++foreignStoreIt)
    {
        if (isCacheableForeignRecord(foreignStoreIt->first))
        {
            std::vector<ESM4::FormId> identifiers;
            foreignStoreIt->second->listForeignIdentifier(identifiers);

            std::vector<ESM4::FormId>::const_iterator record = identifiers.begin();
            for (; record != identifiers.end(); ++record)
                mForeignIds[*record] = foreignStoreIt->first; // FIXME: log duplicates?
        }
    }
}

    int ESMStore::countSavedGameRecords() const
    {
        return 1 // DYNA (dynamic name counter)
            +mPotions.getDynamicSize()
            +mArmors.getDynamicSize()
            +mBooks.getDynamicSize()
            +mClasses.getDynamicSize()
            +mClothes.getDynamicSize()
            +mEnchants.getDynamicSize()
            +mNpcs.getDynamicSize()
            +mSpells.getDynamicSize()
            +mWeapons.getDynamicSize()
            +mCreatureLists.getDynamicSize()
            +mItemLists.getDynamicSize();
    }

    void ESMStore::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_DYNA);
        writer.startSubRecord("COUN");
        writer.writeT(mDynamicCount);
        writer.endRecord("COUN");
        writer.endRecord(ESM::REC_DYNA);

        mPotions.write (writer, progress);
        mArmors.write (writer, progress);
        mBooks.write (writer, progress);
        mClasses.write (writer, progress);
        mClothes.write (writer, progress);
        mEnchants.write (writer, progress);
        mSpells.write (writer, progress);
        mWeapons.write (writer, progress);
        mNpcs.write (writer, progress);
        mItemLists.write (writer, progress);
        mCreatureLists.write (writer, progress);
    }

    bool ESMStore::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        switch (type)
        {
            case ESM::REC_ALCH:
            case ESM::REC_ARMO:
            case ESM::REC_BOOK:
            case ESM::REC_CLAS:
            case ESM::REC_CLOT:
            case ESM::REC_ENCH:
            case ESM::REC_SPEL:
            case ESM::REC_WEAP:
            case ESM::REC_NPC_:
            case ESM::REC_LEVI:
            case ESM::REC_LEVC:

                {
                    mStores[type]->read (reader);
                }

                if (type==ESM::REC_NPC_)
                {
                    // NPC record will always be last and we know that there can be only one
                    // dynamic NPC record (player) -> We are done here with dynamic record loading
                    setUp();

                    const ESM::NPC *player = mNpcs.find ("player");

                    if (!mRaces.find (player->mRace) ||
                        !mClasses.find (player->mClass))
                        throw std::runtime_error ("Invalid player record (race or class unavailable");
                }

                return true;

            case ESM::REC_DYNA:
                reader.getSubNameIs("COUN");
                reader.getHT(mDynamicCount);
                return true;

            default:

                return false;
        }
    }

} // end namespace
