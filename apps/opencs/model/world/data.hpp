#ifndef CSM_WOLRD_DATA_H
#define CSM_WOLRD_DATA_H

#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <QObject>
#include <QModelIndex>

#include <extern/esm4/stat.hpp>
#include <extern/esm4/anio.hpp>
#include <extern/esm4/cont.hpp>
#include <extern/esm4/acti.hpp>
#include <extern/esm4/misc.hpp>
#include <extern/esm4/armo.hpp>
#include <extern/esm4/npc_.hpp>
#include <extern/esm4/flor.hpp>
#include <extern/esm4/gras.hpp>
#include <extern/esm4/tree.hpp>
#include <extern/esm4/ligh.hpp>
#include <extern/esm4/book.hpp>
#include <extern/esm4/furn.hpp>
#include <extern/esm4/soun.hpp>
#include <extern/esm4/weap.hpp>
#include <extern/esm4/door.hpp>
#include <extern/esm4/ammo.hpp>
#include <extern/esm4/clot.hpp>
#include <extern/esm4/alch.hpp>
#include <extern/esm4/appa.hpp>
#include <extern/esm4/ingr.hpp>
#include <extern/esm4/sgst.hpp>
#include <extern/esm4/slgm.hpp>
#include <extern/esm4/keym.hpp>
#include <extern/esm4/hair.hpp>
#include <extern/esm4/eyes.hpp>
#include <extern/esm4/crea.hpp>
#include <extern/esm4/lvlc.hpp>
#include <extern/esm4/ltex.hpp>

#include <components/esm/loadglob.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadsoun.hpp>
#include <components/esm/loadscpt.hpp>
#include <components/esm/loadregn.hpp>
#include <components/esm/loadbsgn.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/loaddial.hpp>
#include <components/esm/loadench.hpp>
#include <components/esm/loadbody.hpp>
#include <components/esm/loadsndg.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadsscr.hpp>
#include <components/esm/debugprofile.hpp>
#include <components/esm/filter.hpp>

#include <components/to_utf8/to_utf8.hpp>

#include "../doc/stage.hpp"

#include "../foreign/idrecord.hpp"
#include "../foreign/idcollection.hpp"
#include "../foreign/navigationcollection.hpp"
#include "../foreign/navmeshcollection.hpp"
#include "../foreign/landcollection.hpp"
#include "../foreign/cellcollection.hpp"
#include "../foreign/worldcollection.hpp"
#include "../foreign/regioncollection.hpp"
#include "../foreign/cellgroupcollection.hpp"
#include "../foreign/cellrefcollection.hpp"
#include "../foreign/cellref.hpp"
#include "../foreign/cellchar.hpp"

#include "idcollection.hpp"
#include "nestedidcollection.hpp"
#include "universalid.hpp"
#include "cell.hpp"
#include "land.hpp"
#include "landtexture.hpp"
#include "refidcollection.hpp"
#include "refcollection.hpp"
#include "infocollection.hpp"
#include "nestedinfocollection.hpp"
#include "pathgrid.hpp"
#include "metadata.hpp"
#ifndef Q_MOC_RUN
#include "subcellcollection.hpp"
#endif

class QAbstractItemModel;

namespace ESM
{
    class ESMReader;
    struct Dialogue;
}

namespace CSMWorld
{
    class ResourcesManager;
    class Resources;
    class NpcAutoCalc;

    class Data : public QObject
    {
            Q_OBJECT

            ToUTF8::Utf8Encoder mEncoder;
            IdCollection<ESM::Global> mGlobals;
            IdCollection<ESM::GameSetting> mGmsts;
            IdCollection<ESM::Skill> mSkills;
            IdCollection<ESM::Class> mClasses;
            NestedIdCollection<ESM::Faction> mFactions;
            NestedIdCollection<ESM::Race> mRaces;
            IdCollection<ESM::Sound> mSounds;
            IdCollection<ESM::Script> mScripts;
            NestedIdCollection<ESM::Region> mRegions;
            NestedIdCollection<ESM::BirthSign> mBirthsigns;
            NestedIdCollection<ESM::Spell> mSpells;
            IdCollection<ESM::Dialogue> mTopics;
            IdCollection<ESM::Dialogue> mJournals;
            NestedIdCollection<ESM::Enchantment> mEnchantments;
            IdCollection<ESM::BodyPart> mBodyParts;
            IdCollection<ESM::MagicEffect> mMagicEffects;
            SubCellCollection<Pathgrid> mPathgrids;
            IdCollection<ESM::DebugProfile> mDebugProfiles;
            IdCollection<ESM::SoundGenerator> mSoundGens;
            IdCollection<ESM::StartScript> mStartScripts;
            NestedInfoCollection mTopicInfos;
            InfoCollection mJournalInfos;
            NestedIdCollection<Cell> mCells;
            IdCollection<LandTexture> mLandTextures;
            IdCollection<Land> mLand;
            RefIdCollection mReferenceables;
            RefCollection mRefs;
            IdCollection<ESM::Filter> mFilters;
            Collection<MetaData> mMetaData;
            CSMForeign::WorldCollection mForeignWorlds;
            CSMForeign::RegionCollection mForeignRegions;
            CSMForeign::CellCollection mForeignCells;
            CSMForeign::CellGroupCollection mForeignCellGroups;
            CSMForeign::LandCollection mForeignLands;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LandTexture> > mForeignLandTextures;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Static> > mForeignStatics;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::AnimObject> > mForeignAnimObjs;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Container> > mForeignContainers;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::MiscItem> > mForeignMiscItems;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Activator> > mForeignActivators;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Armor> > mForeignArmors;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Npc> > mForeignNpcs;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Flora> > mForeignFloras;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Grass> > mForeignGrasses;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Tree> > mForeignTrees;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Light> > mForeignLights;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Book> > mForeignBooks;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Furniture> > mForeignFurnitures;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Sound> > mForeignSounds;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Weapon> > mForeignWeapons;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Door> > mForeignDoors;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ammunition> > mForeignAmmos;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Clothing> > mForeignClothings;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Potion> > mForeignPotions;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Apparatus> > mForeignApparatuses;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ingredient> > mForeignIngredients;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SigilStone> > mForeignSigilStones;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SoulGem> > mForeignSoulGems;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Key> > mForeignKeys;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Hair> > mForeignHairs;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Eyes> > mForeignEyesSet;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Creature> > mForeignCreatures;
            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LevelledCreature> > mForeignLvlCreatures;
            //CSMForeign::RefIdCollection mForeignReferenceables;
            CSMForeign::CellRefCollection<CSMForeign::CellRef> mForeignRefs;
            CSMForeign::CellRefCollection<CSMForeign::CellChar> mForeignChars;
            CSMForeign::NavigationCollection mNavigation;
            CSMForeign::NavMeshCollection mNavMesh;
            const ResourcesManager& mResourcesManager;
            std::vector<QAbstractItemModel *> mModels;
            std::map<UniversalId::Type, QAbstractItemModel *> mModelIndex;
            std::map<std::string, QAbstractItemModel *> mRegionMapIndex;
            ESM::ESMReader *mReader;
            const ESM::Dialogue *mDialogue; // last loaded dialogue
            bool mBase;
            bool mProject;
            std::map<std::string, std::map<unsigned int, unsigned int> > mRefLoadCache;
            int mReaderIndex;
            std::vector<std::string> mLoadedFiles;

            std::vector<boost::shared_ptr<ESM::ESMReader> > mReaders;

            NpcAutoCalc *mNpcAutoCalc;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

            void addModel (QAbstractItemModel *model, UniversalId::Type type,
                bool update = true);

            static void appendIds (std::vector<std::string>& ids, const CollectionBase& collection,
                bool listDeleted);
            ///< Append all IDs from collection to \a ids.

            static int count (RecordBase::State state, const CollectionBase& collection);

            const Data& self ();

            bool loadTes4Group (CSMDoc::Messages& messages);
            bool loadTes4Record (const ESM4::RecordHeader& hdr, CSMDoc::Messages& messages);

        public:

            Data (ToUTF8::FromType encoding, const ResourcesManager& resourcesManager);

            virtual ~Data();

            const IdCollection<ESM::Global>& getGlobals() const;

            IdCollection<ESM::Global>& getGlobals();

            const IdCollection<ESM::GameSetting>& getGmsts() const;

            IdCollection<ESM::GameSetting>& getGmsts();

            const IdCollection<ESM::Skill>& getSkills() const;

            IdCollection<ESM::Skill>& getSkills();

            const IdCollection<ESM::Class>& getClasses() const;

            IdCollection<ESM::Class>& getClasses();

            const IdCollection<ESM::Faction>& getFactions() const;

            IdCollection<ESM::Faction>& getFactions();

            const IdCollection<ESM::Race>& getRaces() const;

            IdCollection<ESM::Race>& getRaces();

            const IdCollection<ESM::Sound>& getSounds() const;

            IdCollection<ESM::Sound>& getSounds();

            const IdCollection<ESM::Script>& getScripts() const;

            IdCollection<ESM::Script>& getScripts();

            const IdCollection<ESM::Region>& getRegions() const;

            IdCollection<ESM::Region>& getRegions();

            const IdCollection<ESM::BirthSign>& getBirthsigns() const;

            IdCollection<ESM::BirthSign>& getBirthsigns();

            const IdCollection<ESM::Spell>& getSpells() const;

            IdCollection<ESM::Spell>& getSpells();

            const IdCollection<ESM::Dialogue>& getTopics() const;

            IdCollection<ESM::Dialogue>& getTopics();

            const IdCollection<ESM::Dialogue>& getJournals() const;

            IdCollection<ESM::Dialogue>& getJournals();

            const InfoCollection& getTopicInfos() const;

            InfoCollection& getTopicInfos();

            const InfoCollection& getJournalInfos() const;

            InfoCollection& getJournalInfos();

            const IdCollection<Cell>& getCells() const;

            IdCollection<Cell>& getCells();

            const RefIdCollection& getReferenceables() const;

            RefIdCollection& getReferenceables();

            const RefCollection& getReferences() const;

            RefCollection& getReferences();

            const IdCollection<ESM::Filter>& getFilters() const;

            IdCollection<ESM::Filter>& getFilters();

            const IdCollection<ESM::Enchantment>& getEnchantments() const;

            IdCollection<ESM::Enchantment>& getEnchantments();

            const IdCollection<ESM::BodyPart>& getBodyParts() const;

            IdCollection<ESM::BodyPart>& getBodyParts();

            const IdCollection<ESM::DebugProfile>& getDebugProfiles() const;

            IdCollection<ESM::DebugProfile>& getDebugProfiles();

            const IdCollection<CSMWorld::Land>& getLand() const;

            IdCollection<CSMWorld::Land>& getLand();

            const IdCollection<CSMWorld::LandTexture>& getLandTextures() const;

            IdCollection<CSMWorld::LandTexture>& getLandTextures();

            const IdCollection<ESM::SoundGenerator>& getSoundGens() const;

            IdCollection<ESM::SoundGenerator>& getSoundGens();

            const IdCollection<ESM::MagicEffect>& getMagicEffects() const;

            IdCollection<ESM::MagicEffect>& getMagicEffects();

            const SubCellCollection<Pathgrid>& getPathgrids() const;

            SubCellCollection<Pathgrid>& getPathgrids();

            const CSMForeign::WorldCollection& getForeignWorlds() const;

            CSMForeign::WorldCollection& getForeignWorlds();

            const CSMForeign::RegionCollection& getForeignRegions() const;

            CSMForeign::RegionCollection& getForeignRegions();

            const CSMForeign::CellCollection& getForeignCells() const;

            CSMForeign::CellCollection& getForeignCells();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LandTexture> >& getForeignLandTextures() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LandTexture> >& getForeignLandTextures();

            const CSMForeign::LandCollection& getForeignLands() const;

            CSMForeign::LandCollection& getForeignLands();

            //const CSMForeign::RefIdCollection& getForeignReferenceables() const;

            //CSMForeign::RefIdCollection& getForeignReferenceables();

            const CSMForeign::CellGroupCollection& getForeignCellGroups() const;

            CSMForeign::CellGroupCollection& getForeignCellGroups();

            const CSMForeign::CellRefCollection<CSMForeign::CellRef>& getForeignReferences() const;

            CSMForeign::CellRefCollection<CSMForeign::CellRef>& getForeignReferences();

            const CSMForeign::CellRefCollection<CSMForeign::CellChar>& getForeignChars() const;

            CSMForeign::CellRefCollection<CSMForeign::CellChar>& getForeignChars();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Static> >& getForeignStatics() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Static> >& getForeignStatics();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::AnimObject> >& getForeignAnimObjs() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::AnimObject> >& getForeignAnimObjs();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Container> >& getForeignContainers() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Container> >& getForeignContainers();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::MiscItem> >& getForeignMiscItems() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::MiscItem> >& getForeignMiscItems();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Activator> >& getForeignActivators() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Activator> >& getForeignActivators();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Armor> >& getForeignArmors() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Armor> >& getForeignArmors();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Npc> >& getForeignNpcs() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Npc> >& getForeignNpcs();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Flora> >& getForeignFloras() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Flora> >& getForeignFloras();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Grass> >& getForeignGrasses() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Grass> >& getForeignGrasses();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Tree> >& getForeignTrees() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Tree> >& getForeignTrees();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Light> >& getForeignLights() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Light> >& getForeignLights();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Book> >& getForeignBooks() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Book> >& getForeignBooks();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Furniture> >& getForeignFurnitures() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Furniture> >& getForeignFurnitures();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Sound> >& getForeignSounds() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Sound> >& getForeignSounds();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Weapon> >& getForeignWeapons() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Weapon> >& getForeignWeapons();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Door> >& getForeignDoors() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Door> >& getForeignDoors();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ammunition> >& getForeignAmmos() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ammunition> >& getForeignAmmos();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Clothing> >& getForeignClothings() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Clothing> >& getForeignClothings();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Potion> >& getForeignPotions() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Potion> >& getForeignPotions();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Apparatus> >& getForeignApparatuses() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Apparatus> >& getForeignApparatuses();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ingredient> >& getForeignIngredients() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Ingredient> >& getForeignIngredients();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SigilStone> >& getForeignSigilStones() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SigilStone> >& getForeignSigilStones();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SoulGem> >& getForeignSoulGems() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::SoulGem> >& getForeignSoulGems();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Key> >& getForeignKeys() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Key> >& getForeignKeys();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Hair> >& getForeignHairs() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Hair> >& getForeignHairs();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Eyes> >& getForeignEyesSet() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Eyes> >& getForeignEyesSet();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Creature> >& getForeignCreatures() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::Creature> >& getForeignCreatures();

            const CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LevelledCreature> >& getForeignLvlCreatures() const;

            CSMForeign::IdCollection<CSMForeign::IdRecord<ESM4::LevelledCreature> >& getForeignLvlCreatures();

            const CSMForeign::NavigationCollection& getNavigation() const;

            CSMForeign::NavigationCollection& getNavigation();

            const CSMForeign::NavMeshCollection& getNavMeshes() const;

            CSMForeign::NavMeshCollection& getNavMeshes();

            const IdCollection<ESM::StartScript>& getStartScripts() const;

            IdCollection<ESM::StartScript>& getStartScripts();

            /// Throws an exception, if \a id does not match a resources list.
            const Resources& getResources (const UniversalId& id) const;

            const MetaData& getMetaData() const;

            void setMetaData (const MetaData& metaData);

            QAbstractItemModel *getTableModel (const UniversalId& id);
            ///< If no table model is available for \a id, an exception is thrown.
            ///
            /// \note The returned table may either be the model for the ID itself or the model that
            /// contains the record specified by the ID.

            void merge();
            ///< Merge modified into base.

            int getTotalRecords (const std::vector<boost::filesystem::path>& files); // for better loading bar

            int startLoading (const boost::filesystem::path& path, bool base, bool project);
            ///< Begin merging content of a file into base or modified.
            ///
            /// \param project load project file instead of content file
            ///
            ///< \return estimated number of records

            bool continueLoading (CSMDoc::Messages& messages);
            ///< \return Finished?

            bool hasId (const std::string& id) const;

            std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs that are not internal to the editor.
            ///
            /// \param listDeleted include deleted record in the list

            int count (RecordBase::State state) const;
            ///< Return number of top-level records with the given \a state.

            const NpcAutoCalc& getNpcAutoCalc() const;

        signals:

            void idListChanged();

        private slots:

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsChanged (const QModelIndex& parent, int start, int end);
    };
}

#endif
