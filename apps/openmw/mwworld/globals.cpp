#include "globals.hpp"

#include <stdexcept>

#include <components/misc/stringops.hpp>

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>

#include "esmstore.hpp"

namespace MWWorld
{
    Globals::Collection::const_iterator Globals::find (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (Misc::StringUtils::lowerCase (name));

        if (iter==mVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);

        return iter;
    }

    Globals::Collection::iterator Globals::find (const std::string& name)
    {
        Collection::iterator iter = mVariables.find (Misc::StringUtils::lowerCase (name));

        if (iter==mVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);

        return iter;
    }

    void Globals::fill (const MWWorld::ESMStore& store)
    {
        mVariables.clear();

        const MWWorld::Store<ESM::Global>& globals = store.get<ESM::Global>();

        for (MWWorld::Store<ESM::Global>::iterator iter = globals.begin(); iter!=globals.end();
            ++iter)
        {
            mVariables.insert (std::make_pair (Misc::StringUtils::lowerCase (iter->mId), *iter));
        }

        const MWWorld::ForeignStore<ESM4::GlobalVariable>& foreignGlobals
            = store.getForeign<ESM4::GlobalVariable>();
        for (ForeignStore<ESM4::GlobalVariable>::iterator iter2 = foreignGlobals.begin(); iter2 != foreignGlobals.end(); ++iter2)
        {
            ESM::Global record;
            record.mId = iter2->mEditorId;

            ESM::VarType type = ESM::VT_Unknown;

            if (iter2->mType == 's')
            {
                type = ESM::VT_Short;
                record.mValue.setType(type);

                if (iter2->mValue != iter2->mValue)
                    record.mValue.setInteger(0);
                else
                    record.mValue.setInteger(static_cast<short> (iter2->mValue));
            }
            else if (iter2->mType == 'l')
            {
                type = ESM::VT_Long;
                record.mValue.setType(type);
                record.mValue.setInteger(static_cast<int> (iter2->mValue));
            }
            else if (iter2->mType == 'f')
            {
                type = ESM::VT_Float;
                record.mValue.setType(type);
                record.mValue.setFloat(iter2->mValue);
            }

            std::pair<std::map<std::string, ESM::Global>::iterator, bool> res =
                mVariables.insert (std::make_pair (Misc::StringUtils::lowerCase (iter2->mEditorId), record));

            // FIXME: warn duplicates
        }
    }

    const ESM::Variant& Globals::operator[] (const std::string& name) const
    {
        return find (Misc::StringUtils::lowerCase (name))->second.mValue;
    }

    ESM::Variant& Globals::operator[] (const std::string& name)
    {
        return find (Misc::StringUtils::lowerCase (name))->second.mValue;
    }

    char Globals::getType (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (Misc::StringUtils::lowerCase (name));

        if (iter==mVariables.end())
            return ' ';

        switch (iter->second.mValue.getType())
        {
            case ESM::VT_Short: return 's';
            case ESM::VT_Long: return 'l';
            case ESM::VT_Float: return 'f';

            default: return ' ';
        }
    }

    int Globals::countSavedGameRecords() const
    {
        return mVariables.size();
    }

    void Globals::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (Collection::const_iterator iter (mVariables.begin()); iter!=mVariables.end(); ++iter)
        {
            writer.startRecord (ESM::REC_GLOB);
            iter->second.save (writer);
            writer.endRecord (ESM::REC_GLOB);
        }
    }

    bool Globals::readRecord (ESM::ESMReader& reader,  uint32_t type)
    {
        if (type==ESM::REC_GLOB)
        {
            ESM::Global global;
            bool isDeleted = false;

            // This readRecord() method is used when reading a saved game.
            // Deleted globals can't appear there, so isDeleted will be ignored here.
            global.load(reader, isDeleted);
            Misc::StringUtils::lowerCaseInPlace(global.mId);

            Collection::iterator iter = mVariables.find (global.mId);
            if (iter!=mVariables.end())
                iter->second = global;

            return true;
        }

        return false;
    }
}
