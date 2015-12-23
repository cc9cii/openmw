#include "navmeshcollection.hpp"

#include <stdexcept>
#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>

#include "../world/idcollection.hpp"
#include "../world/cell.hpp"
#include "../world/record.hpp"

// FIXME: refactor as foreign/idcollection
namespace CSMWorld
{
    template<>
    void Collection<CSMForeign::NavMesh, IdAccessor<CSMForeign::NavMesh> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in NavMeshCollection::removeRows()
    }

    template<>
    void Collection<CSMForeign::NavMesh, IdAccessor<CSMForeign::NavMesh> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CSMForeign::NavMesh> > record2(static_cast<Record<CSMForeign::NavMesh>*>(record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in NavMeshCollection::insertRecord()
    }
}

CSMForeign::NavMeshCollection::NavMeshCollection (const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells)
  : mCells (cells)
{
}

CSMForeign::NavMeshCollection::~NavMeshCollection ()
{
}

int CSMForeign::NavMeshCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::NavMesh record;
    //std::cout << "new NavMesh " << std::hex << &record << std::endl; // FIXME

    std::string id;
#if 0
    // HACK // FIXME
    if (reader.grp().type != ESM4::Grp_CellTemporaryChild)
        return -1; // FIXME
    else if (reader.grp(2).type == ESM4::Grp_InteriorCell)
    {
        // FIXME: another id?
        id = "";
    }
    else
    {
        // FIXME: navmesh can occur in interior cells
        std::ostringstream stream;
        //stream << "#" << reader.currCell().grid.x << " " << reader.currCell().grid.y;
        //stream << "#" << std::floor((float)reader.currCell().grid.x/2)
               //<< " " << std::floor((float)reader.currCell().grid.y/2);
        id = ""; //stream.str();
        //std::cout << "loading Cell " << id << std::endl; // FIXME
    }
#endif
    id = std::to_string(reader.hdr().record.id); // use formId instead

    int index = searchId(reader.hdr().record.id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::NavMesh>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::NavMeshCollection::loadRecord (CSMForeign::NavMesh& record, ESM4::Reader& reader)
{
    record.load(reader, mCells);
}

int CSMForeign::NavMeshCollection::load (const CSMForeign::NavMesh& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::NavMesh>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMesh> > record2(new CSMWorld::Record<CSMForeign::NavMesh>);
        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMesh> > record2(
                new CSMWorld::Record<CSMForeign::NavMesh>(CSMWorld::Collection<CSMForeign::NavMesh,
                    CSMWorld::IdAccessor<CSMForeign::NavMesh> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::NavMeshCollection::searchId (const std::string& id) const
{
    return searchId(static_cast<std::uint32_t>(std::stoi(id)));
}

int CSMForeign::NavMeshCollection::getIndex (std::uint32_t id) const
{
    int index = searchId(id);

    if (index == -1)
        throw std::runtime_error("invalid formId: " + std::to_string(id));

    return index;
}

void CSMForeign::NavMeshCollection::removeRows (int index, int count)
{
    CSMWorld::Collection<NavMesh, CSMWorld::IdAccessor<NavMesh> >::removeRows(index, count); // erase records only

    std::map<std::uint32_t, int>::iterator iter = mNavMeshIndex.begin();
    while (iter != mNavMeshIndex.end())
    {
        if (iter->second>=index)
        {
            if (iter->second >= index+count)
            {
                iter->second -= count;
                ++iter;
            }
            else
                mNavMeshIndex.erase(iter++);
        }
        else
            ++iter;
    }
}

int CSMForeign::NavMeshCollection::searchId (std::uint32_t id) const
{
    std::map<std::uint32_t, int>::const_iterator iter = mNavMeshIndex.find(id);

    if (iter == mNavMeshIndex.end())
        return -1;

    return iter->second;
}

void CSMForeign::NavMeshCollection::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
    CSMWorld::UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // id is ignored
    std::uint32_t formId = static_cast<CSMWorld::Record<NavMesh>*>(record.get())->get().mFormId;

    CSMWorld::Collection<NavMesh, CSMWorld::IdAccessor<NavMesh> >::insertRecord(std::move(record), index, type); // add records only

    if (index < size-1)
    {
        for (std::map<std::uint32_t, int>::iterator iter(mNavMeshIndex.begin()); iter != mNavMeshIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mNavMeshIndex.insert(std::make_pair(formId, index));
}

#if 0
void CSMForeign::NavMeshCollection::addNestedRow (int row, int col, int position)
{
}

void CSMForeign::NavMeshCollection::removeNestedRows (int row, int column, int subRow)
{
}

QVariant CSMForeign::NavMeshCollection::getNestedData (int row,
        int column, int subRow, int subColumn) const
{
}

void CSMForeign::NavMeshCollection::setNestedData (int row,
        int column, const QVariant& data, int subRow, int subColumn)
{
}

NestedTableWrapperBase* CSMForeign::NavMeshCollection::nestedTable (int row, int column) const
{
}

void CSMForeign::NavMeshCollection::setNestedTable (int row,
        int column, const NestedTableWrapperBase& nestedTable)
{
}

int CSMForeign::NavMeshCollection::getNestedRowsCount (int row, int column) const
{
}

int CSMForeign::NavMeshCollection::getNestedColumnsCount (int row, int column) const
{
}

NestableColumn *CSMForeign::NavMeshCollection::getNestableColumn (int column)
{
}
#endif
