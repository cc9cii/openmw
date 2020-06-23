#include "idrecord.hpp"

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
#include <extern/esm4/land.hpp>
#include <extern/esm4/common.hpp>

template<> unsigned int CSMForeign::IdRecord<ESM4::Activator>::sRecordId  = ESM4::REC_ACTI;
template<> unsigned int CSMForeign::IdRecord<ESM4::Ammunition>::sRecordId = ESM4::REC_AMMO;
template<> unsigned int CSMForeign::IdRecord<ESM4::AnimObject>::sRecordId = ESM4::REC_ANIO;
template<> unsigned int CSMForeign::IdRecord<ESM4::Apparatus>::sRecordId  = ESM4::REC_APPA;
template<> unsigned int CSMForeign::IdRecord<ESM4::Armor>::sRecordId      = ESM4::REC_ARMO;
template<> unsigned int CSMForeign::IdRecord<ESM4::Book>::sRecordId       = ESM4::REC_BOOK;
template<> unsigned int CSMForeign::IdRecord<ESM4::Clothing>::sRecordId   = ESM4::REC_CLOT;
template<> unsigned int CSMForeign::IdRecord<ESM4::Container>::sRecordId  = ESM4::REC_CONT;
template<> unsigned int CSMForeign::IdRecord<ESM4::Creature>::sRecordId   = ESM4::REC_CREA;
template<> unsigned int CSMForeign::IdRecord<ESM4::Door>::sRecordId       = ESM4::REC_DOOR;
template<> unsigned int CSMForeign::IdRecord<ESM4::Eyes>::sRecordId       = ESM4::REC_EYES;
template<> unsigned int CSMForeign::IdRecord<ESM4::Flora>::sRecordId      = ESM4::REC_FLOR;
template<> unsigned int CSMForeign::IdRecord<ESM4::Furniture>::sRecordId  = ESM4::REC_FURN;
template<> unsigned int CSMForeign::IdRecord<ESM4::Grass>::sRecordId      = ESM4::REC_GRAS;
template<> unsigned int CSMForeign::IdRecord<ESM4::Hair>::sRecordId       = ESM4::REC_HAIR;
template<> unsigned int CSMForeign::IdRecord<ESM4::Ingredient>::sRecordId = ESM4::REC_INGR;
template<> unsigned int CSMForeign::IdRecord<ESM4::Key>::sRecordId        = ESM4::REC_KEYM;
template<> unsigned int CSMForeign::IdRecord<ESM4::LandTexture>::sRecordId = ESM4::REC_LTEX;
template<> unsigned int CSMForeign::IdRecord<ESM4::LevelledCreature>::sRecordId = ESM4::REC_LVLC;
template<> unsigned int CSMForeign::IdRecord<ESM4::Light>::sRecordId      = ESM4::REC_LIGH;
template<> unsigned int CSMForeign::IdRecord<ESM4::MiscItem>::sRecordId   = ESM4::REC_MISC;
template<> unsigned int CSMForeign::IdRecord<ESM4::Npc>::sRecordId        = ESM4::REC_NPC_;
template<> unsigned int CSMForeign::IdRecord<ESM4::Potion>::sRecordId     = ESM4::REC_ALCH;
template<> unsigned int CSMForeign::IdRecord<ESM4::SigilStone>::sRecordId = ESM4::REC_SGST;
template<> unsigned int CSMForeign::IdRecord<ESM4::SoulGem>::sRecordId    = ESM4::REC_SLGM;
template<> unsigned int CSMForeign::IdRecord<ESM4::Sound>::sRecordId      = ESM4::REC_SOUN;
template<> unsigned int CSMForeign::IdRecord<ESM4::Static>::sRecordId     = ESM4::REC_STAT;
template<> unsigned int CSMForeign::IdRecord<ESM4::Tree>::sRecordId       = ESM4::REC_TREE;
template<> unsigned int CSMForeign::IdRecord<ESM4::Weapon>::sRecordId     = ESM4::REC_WEAP;
template<> unsigned int CSMForeign::IdRecord<ESM4::Land>::sRecordId       = ESM4::REC_LAND;
