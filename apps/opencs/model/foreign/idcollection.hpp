#ifndef CSM_FOREIGN_IDCOLLECTION_H
#define CSM_FOREIGN_IDCOLLECTION_H

#include "collection.hpp"

#include <extern/esm4/common.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    template<typename RecordT>
    class IdCollection : public Collection<RecordT>
    {
    protected:
        void loadRecord (RecordT& record, ESM4::Reader& reader);

    public:
        virtual int load(ESM4::Reader& reader, bool base);

        int load (const RecordT& record, bool base, int index = -2);
    };

    template<typename RecordT>
    void IdCollection<RecordT>::loadRecord (RecordT& record, ESM4::Reader& reader)
    {
        record.load(reader);
    }

    template<typename RecordT>
    int IdCollection<RecordT>::load (ESM4::Reader& reader, bool base)
    {
        using CSMWorld::Record;

        RecordT record;

        loadRecord(record, reader);

        int index = this->searchFormId(record.mFormId);

        if ((record.mFlags & ESM4::Rec_Deleted) != 0)
        {
            if (index == -1)
            {
                // deleting a record that does not exist
                // ignore it for now
                /// \todo report the problem to the user
                return -1;
            }

            if (base)
            {
                this->removeRows(index, 1);
                return -1;
            }

            std::unique_ptr<Record<RecordT> > baseRecord(new Record<RecordT>);
            baseRecord->mBase = this->getRecord(index).get();
            baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
        }

        return load (record, base, index);
    }

    template<typename RecordT>
    int IdCollection<RecordT>::load (const RecordT& record, bool base, int index)
    {
        using CSMWorld::Record;

        if (index == -2) // index unknown
            index = this->searchFormId(record.mFormId);

        if (index == -1)
        {
            // new record
            std::unique_ptr<Record<RecordT> > record2(new Record<RecordT>);
            record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly
                                   : CSMWorld::RecordBase::State_ModifiedOnly;
            (base ? record2->mBase : record2->mModified) = record;

            index = this->getSize();
            this->appendRecord(std::move(record2));
        }
        else
        {
            // old record
            std::unique_ptr<Record<RecordT> > record2(new Record<RecordT>(this->getRecord(index)));

            if (base)
                record2->mBase = record; // FIXME: should never happen?
            else
                record2->setModified(record);

            this->setRecord(index, std::move(record2));
        }

        return index;
    }
}

#endif // CSM_FOREIGN_IDCOLLECTION_H
