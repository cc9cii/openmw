#ifndef OPENMW_MWWORLD_FOREIGNLAND_H
#define OPENMW_MWWORLD_FOREIGNLAND_H

#include <string>

#include <components/esm4terrain/land.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWWorld
{
    struct ForeignLand : public ESM4Terrain::Land // NOTE: not derived from ESM4::Land
    {
        static unsigned int sRecordId;

        ForeignLand();
        ~ForeignLand();

        std::string mId;          // cache converted string
        //std::string mCellId; // for region map (#x y for most exterior cells)
        //std::string mCellName; // Cell name

        void load (ESM::ESMReader& esm, bool isDeleted = false);
        void save (ESM::ESMWriter& esm, bool isDeleted = false) const {} // FIXME: TODO

        void blank(); // FIXME: is this needed?
    };
}

#endif // OPENMW_MWWORLD_FOREIGNLAND_H
