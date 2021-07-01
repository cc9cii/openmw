/*
  Copyright (C) 2015-2019, 2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "reader.hpp"

#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <iostream>
#include <sstream> // for debugging
#include <iomanip> // for debugging

#if defined(_MSC_VER)
    #pragma warning (push)
    #pragma warning (disable : 4706)
    #include <boost/iostreams/filter/zlib.hpp>
    #pragma warning (pop)
#else
    #include <boost/iostreams/filter/zlib.hpp>
#endif
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <components/bsa/memorystream.hpp>

#include "formid.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

ESM4::Reader::Reader() : mObserver(nullptr), mRecordRemaining(0), mCellGridValid(false), mFileSize(0)
{
    mCtx.modIndex = 0;
    mCtx.currWorld = 0;
    mCtx.currCell = 0;
    mCtx.recHeaderSize = sizeof(ESM4::RecordHeader);

    mStream.reset();
    mSavedStream.reset();
}

ESM4::Reader::~Reader()
{
}

// Since the record data may have been compressed, it is not always possible to use seek() to
// go to a position of a sub record.
//
// The record header needs to be saved in the context or the header needs to be re-loaded after
// restoring the context. The latter option was chosen.
ESM4::ReaderContext ESM4::Reader::getContext()
{
    mCtx.filePos = mStream->tellg();
    mCtx.filePos -= mCtx.recHeaderSize; // update file position
    return mCtx;
}

// NOTE: Assumes that the caller has reopened the file if necessary
bool ESM4::Reader::restoreContext(const ESM4::ReaderContext& ctx)
{
    if (mSavedStream)
    {
        mStream = mSavedStream;
        mSavedStream.reset();
    }

    mCtx.groupStack.clear(); // probably not necessary?
    mCtx = ctx;
    mStream->seekg(ctx.filePos); // update file position

    //return getRecordHeader(); // can't use it because mStream may have been switched
    mStream->read((char*)&mRecordHeader, mCtx.recHeaderSize);

    if (mObserver)
        mObserver->update(mCtx.recHeaderSize);

    mRecordRemaining = mRecordHeader.record.dataSize; // for keeping track of sub records

    return (mStream->gcount() == mCtx.recHeaderSize);
}

bool ESM4::Reader::skipNextGroupCellChild()
{
    if (mStream->eof())
        return false;

    std::size_t pos = mStream->tellg(); // save
    ESM4::RecordHeader hdr;
    mStream->read((char*)&hdr, mCtx.recHeaderSize);
    //if (!mStream->read((char*)&hdr, mCtx.recHeaderSize))
        //throw std::runtime_error("ESM4::Reader::could not peek header");

    if (hdr.group.type != ESM4::Grp_CellChild)
    {
        mStream->seekg(pos); // go back to saved
        return false;
    }

    mCtx.groupStack.back().second -= hdr.group.groupSize;
    mStream->ignore(hdr.group.groupSize - (std::uint32_t)mCtx.recHeaderSize); // already read the header
    if (mObserver)
        mObserver->update(hdr.group.groupSize);

    return true;
}

// TODO: consider checking file path using boost::filesystem::exists()
std::size_t ESM4::Reader::openTes4File(const std::string& name)
{
    mCtx.filename = name;
    // NOTE: Ogre::SharedPtr<DataStream> provides implicit destruction
    mStream = Files::IStreamPtr(Files::openConstrainedFileStream(name.c_str()));

    mStream->seekg(0, std::ios::end); // FIXME: is there a better way, or eliminate the need to get the size?
    mFileSize = mStream->tellg();
    mStream->seekg(0, std::ios::beg);

    return mFileSize;
}

void ESM4::Reader::setRecHeaderSize(const std::size_t size)
{
    mCtx.recHeaderSize = size;
}

void ESM4::Reader::registerForUpdates(ESM4::ReaderObserver *observer)
{
    mObserver = observer;
}

// FIXME: only "English" strings supported for now
void ESM4::Reader::buildLStringIndex()
{
    if ((mHeader.mFlags & Rec_ESM) == 0 || (mHeader.mFlags & Rec_Localized) == 0)
        return;

    boost::filesystem::path p(mCtx.filename);
    std::string filename = p.stem().filename().string();

    buildLStringIndex("Strings/" + filename + "_English.STRINGS",   Type_Strings);
    buildLStringIndex("Strings/" + filename + "_English.ILSTRINGS", Type_ILStrings);
    buildLStringIndex("Strings/" + filename + "_English.DLSTRINGS", Type_DLStrings);
}

void ESM4::Reader::buildLStringIndex(const std::string& stringFile, LocalizedStringType stringType)
{
    std::uint32_t numEntries;
    std::uint32_t dataSize;
    std::uint32_t stringId;
    LStringOffset sp;
    sp.type = stringType;

    // TODO: possibly check if the resource exists?
    Files::IStreamPtr filestream = Files::IStreamPtr(Files::openConstrainedFileStream(stringFile.c_str()));

    filestream->seekg(0, std::ios::end);
    std::size_t fileSize = filestream->tellg();
    filestream->seekg(0, std::ios::beg);

    switch (stringType)
    {
        case Type_Strings:   mStrings =   filestream; break;
        case Type_ILStrings: mILStrings = filestream; break;
        case Type_DLStrings: mDLStrings = filestream; break;
        default:
            throw std::runtime_error("ESM4::Reader::unexpected string type");
    }

    filestream->read((char*)&numEntries, sizeof(numEntries));
    filestream->read((char*)&dataSize, sizeof(dataSize));
    std::size_t dataStart = fileSize - dataSize;
    for (unsigned int i = 0; i < numEntries; ++i)
    {
        filestream->read((char*)&stringId, sizeof(stringId));
        filestream->read((char*)&sp.offset, sizeof(sp.offset));
        sp.offset += (std::uint32_t)dataStart;
        mLStringIndex[stringId] = sp;
    }
    //assert(dataStart - filestream->tell() == 0 && "String file start of data section mismatch");
}

void ESM4::Reader::getLocalizedString(std::string& str)
{
    std::uint32_t stringId;
    get(stringId);
    if (stringId) // TES5 FoxRace
        getLocalizedString(stringId, str);
}

// FIXME: very messy and probably slow/inefficient
void ESM4::Reader::getLocalizedString(const FormId stringId, std::string& str)
{
    const std::map<FormId, LStringOffset>::const_iterator it = mLStringIndex.find(stringId);

    if (it != mLStringIndex.end())
    {
        Files::IStreamPtr filestream;

        switch (it->second.type)
        {
            case Type_Strings:
            {
                filestream = mStrings;
                filestream->seekg(it->second.offset);

                char ch;
                std::vector<char> data;
                do {
                    filestream->read(&ch, sizeof(ch));
                    data.push_back(ch);
                } while (ch != 0);

                str = std::string(data.data());
                return;
            }
            case Type_ILStrings: filestream = mILStrings; break;
            case Type_DLStrings: filestream = mDLStrings; break;
            default:
                throw std::runtime_error("ESM4::Reader::getLocalizedString unexpected string type");
        }

        // get ILStrings or DLStrings
        filestream->seekg(it->second.offset);
        getZString(str, filestream);
    }
    else // FIXME: stringId might be null? (FoxRace)
        throw std::runtime_error("ESM4::Reader::getLocalizedString localized string not found");
}

bool ESM4::Reader::getRecordHeader()
{
    // FIXME: this seems very hacky but we may have skipped subrecords from within an inflated data block
    if (/*mStream->eof() && */mSavedStream)
    {
        mStream = mSavedStream;
        mSavedStream.reset();
    }

    mStream->read((char*)&mRecordHeader, mCtx.recHeaderSize);

    // keep track of data left to read from the file
    // FIXME: having a default instance of mObserver might be faster than checking for null all the time?
    if (mObserver)
        mObserver->update(mCtx.recHeaderSize);

    mRecordRemaining = mRecordHeader.record.dataSize; // for keeping track of sub records

    return (mStream->gcount() == mCtx.recHeaderSize);

    // After reading the record header we can cache a WRLD or CELL formId for convenient access later.
    // (currently currWorld and currCell are set manually when loading the WRLD and CELL records)
}

bool ESM4::Reader::getSubRecordHeader()
{
    bool result = false;
    // NOTE: some SubRecords have 0 dataSize (e.g. SUB_RDSD in one of REC_REGN records in Oblivion.esm).
    // Also SUB_XXXX has zero dataSize and the following 4 bytes represent the actual dataSize
    // - hence it require manual updtes to mRecordRemaining. See ESM4::NavMesh and ESM4::World.
    if (mRecordRemaining >= sizeof(mSubRecordHeader))
    {
        result = getExact(mSubRecordHeader);
        mRecordRemaining -= (sizeof(mSubRecordHeader) + mSubRecordHeader.dataSize);
    }
    return result;
}

// NOTE: the parameter 'files' must have the file names in the loaded order
void ESM4::Reader::updateModIndices(const std::vector<std::string>& files)
{
    if (files.size() >= 0xff)
        throw std::runtime_error("ESM4::Reader::updateModIndices too many files"); // 0xff is reserved

    // NOTE: this map is rebuilt each time this method is called (i.e. each time a file is loaded)
    // Perhaps there is an opportunity to optimize this by saving the result somewhere.
    // But then, the number of files is at most around 250 so perhaps keeping it simple might be better.

    // build a lookup map
    std::unordered_map<std::string, size_t> fileIndex;

    for (size_t i = 0; i < files.size(); ++i) // ATTENTION: assumes current file is not included
        fileIndex[boost::to_lower_copy<std::string>(files[i])] = i;

    mHeader.mModIndices.resize(mHeader.mMaster.size());
    for (unsigned int i = 0; i < mHeader.mMaster.size(); ++i)
    {
        // locate the position of the dependency in already loaded files
        std::unordered_map<std::string, size_t>::const_iterator it
            = fileIndex.find(boost::to_lower_copy<std::string>(mHeader.mMaster[i].name));

        if (it != fileIndex.end())
            mHeader.mModIndices[i] = (std::uint32_t)((it->second << 24) & 0xff000000);
        else
            throw std::runtime_error("ESM4::Reader::updateModIndices required dependency file not loaded");
#if 0
        std::cout << "Master Mod: " << mHeader.mMaster[i].name << ", " // FIXME: debugging only
                  << ESM4::formIdToString(mHeader.mModIndices[i]) << std::endl;
#endif
    }

    if (!mHeader.mModIndices.empty() &&  mHeader.mModIndices[0] != 0)
        throw std::runtime_error("ESM4::Reader::updateModIndices base modIndex is not zero");
}

void ESM4::Reader::saveGroupStatus()
{
#if 0
    std::string padding = ""; // FIXME: debugging only
    padding.insert(0, mCtx.groupStack.size()*2, ' ');
    std::cout << padding << "Starting record group "
              << ESM4::printLabel(mRecordHeader.group.label, mRecordHeader.group.type) << std::endl;
#endif
    if (mRecordHeader.group.groupSize == (std::uint32_t)mCtx.recHeaderSize)
    {
#if 0
        std::cout << padding << "Ignoring record group " // FIXME: debugging only
            << ESM4::printLabel(mRecordHeader.group.label, mRecordHeader.group.type)
            << " (empty)" << std::endl;
#endif
        if (!mCtx.groupStack.empty()) // top group may be empty (e.g. HAIR in Skyrim)
        {
            // don't put on the stack, checkGroupStatus() may not get called before recursing into this method
            mCtx.groupStack.back().second -= mRecordHeader.group.groupSize;
            checkGroupStatus();
        }
        return; // DLCMehrunesRazor - Unofficial Patch.esp is at EOF after one of these empty groups...
    }

    // push group
    mCtx.groupStack.push_back(std::make_pair(mRecordHeader.group,
                mRecordHeader.group.groupSize - (std::uint32_t)mCtx.recHeaderSize));
}

const ESM4::CellGrid& ESM4::Reader::currCellGrid() const
{
    // Maybe should throw an exception instead?
    assert(mCellGridValid && "Attempt to use an invalid cell grid");

    return mCurrCellGrid;
}

void ESM4::Reader::checkGroupStatus()
{
    // pop finished groups
    while (!mCtx.groupStack.empty() && mCtx.groupStack.back().second == 0)
    {
#if 0
        ESM4::GroupTypeHeader grp = mCtx.groupStack.back().first; // FIXME: grp is for debugging only
#endif
        uint32_t groupSize = mCtx.groupStack.back().first.groupSize;
        mCtx.groupStack.pop_back();
#if 0
        std::string padding = ""; // FIXME: debugging only
        padding.insert(0, mCtx.groupStack.size()*2, ' ');
        std::cout << padding << "Finished record group " << ESM4::printLabel(grp.label, grp.type) << std::endl;
#endif
        // Check if the previous group was the final one
        if (mCtx.groupStack.empty())
            return;

        //assert (mCtx.groupStack.back().second >= groupSize && "Read more records than available");
#if 0
        if (mCtx.groupStack.back().second < groupSize) // FIXME: debugging only
            std::cerr << ESM4::printLabel(mCtx.groupStack.back().first.label,
                                          mCtx.groupStack.back().first.type)
                      << " read more records than available" << std::endl;
#endif
        mCtx.groupStack.back().second -= groupSize;
    }
}

// WARNING: this method should be used after first calling saveGroupStatus()
const ESM4::GroupTypeHeader& ESM4::Reader::grp(std::size_t pos) const
{
    assert(pos <= mCtx.groupStack.size()-1 && "ESM4::Reader::grp - exceeded stack depth");

    return (*(mCtx.groupStack.end()-pos-1)).first;
}

void ESM4::Reader::getRecordData()
{
    std::uint32_t uncompressedSize = 0;

    if ((mRecordHeader.record.flags & ESM4::Rec_Compressed) != 0)
    {
        mStream->read(reinterpret_cast<char*>(&uncompressedSize), sizeof(std::uint32_t));

        std::size_t recordSize = mRecordHeader.record.dataSize - sizeof(std::uint32_t);
        Bsa::MemoryInputStream compressedRecord(recordSize);
        mStream->read(compressedRecord.getRawData(), recordSize);
        std::istream *fileStream = (std::istream*)&compressedRecord;
        mSavedStream = mStream;

        std::shared_ptr<Bsa::MemoryInputStream> memoryStreamPtr
            = std::make_shared<Bsa::MemoryInputStream>(uncompressedSize);

        boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStreamBuf;
        inputStreamBuf.push(boost::iostreams::zlib_decompressor());
        inputStreamBuf.push(*fileStream);

        boost::iostreams::basic_array_sink<char> sr(memoryStreamPtr->getRawData(), uncompressedSize);
        boost::iostreams::copy(inputStreamBuf, sr);

        mStream = std::shared_ptr<std::istream>(memoryStreamPtr, (std::istream*)memoryStreamPtr.get());
    }

    getRecordDataPostActions();
    //std::cout << "data size 0x" << std::hex << mRecordHeader.record.dataSize << std::endl; // FIXME: debug only
}

void ESM4::Reader::getRecordDataPostActions()
{
    // keep track of data left to read from the current group
    assert (!mCtx.groupStack.empty() && "Read data for a record without a group");
    mCtx.groupStack.back().second -= (std::uint32_t)mCtx.recHeaderSize + mRecordHeader.record.dataSize;

    // keep track of data left to read from the file
    if (mObserver)
        mObserver->update(mRecordHeader.record.dataSize);
}

bool ESM4::Reader::getZString(std::string& str)
{
    return getZString(str, mStream);
}

// FIXME: how to without using a temp buffer?
bool ESM4::Reader::getZString(std::string& str, Files::IStreamPtr filestream)
{
    std::uint32_t size = 0;
    if (filestream == mStream)
        size = mSubRecordHeader.dataSize; // WARNING: assumed size from the header is correct
    else
        filestream->read((char*)&size, sizeof(size));

    std::unique_ptr<char[]> buf(new char[size]);
    filestream->read(buf.get(), size);
    if (filestream->gcount() == (size_t)size)
    {
        if (buf[size - 1] != 0)
        {
            str.assign(buf.get(), size);
            //std::cerr << "ESM4::Reader::getZString string is not terminated with a zero" << std::endl;
        }
        else
            str.assign(buf.get(), size - 1);// don't copy null terminator

        //assert((size_t)size-1 == str.size() && "ESM4::Reader::getZString string size mismatch");
        return true;
    }
    else
    {
        str.clear();
        return false; // FIXME: throw instead?
    }
}

// Assumes that saveGroupStatus() is not called before this (hence we don't update mCtx.groupStack)
void ESM4::Reader::skipGroup()
{
#if 0
    std::string padding = ""; // FIXME: debugging only
    padding.insert(0, mCtx.groupStack.size()*2, ' ');
    std::cout << padding << "Skipping record group "
              << ESM4::printLabel(mRecordHeader.group.label, mRecordHeader.group.type) << std::endl;
#endif
    // Note: subtract the size of header already read before skipping
    mStream->ignore(mRecordHeader.group.groupSize - (std::uint32_t)mCtx.recHeaderSize);

    // keep track of data left to read from the file
    if (mObserver)
        mObserver->update((std::size_t)mRecordHeader.group.groupSize - mCtx.recHeaderSize);

    if (!mCtx.groupStack.empty())
        mCtx.groupStack.back().second -= mRecordHeader.group.groupSize;
}

void ESM4::Reader::skipRecordData()
{
    mStream->ignore(mRecordHeader.record.dataSize);

    // keep track of data left to read from the current group
    assert (!mCtx.groupStack.empty() && "Skipping a record without a group");
    mCtx.groupStack.back().second -= (std::uint32_t)mCtx.recHeaderSize + mRecordHeader.record.dataSize;

    // keep track of data left to read from the file
    if (mObserver)
        mObserver->update(mRecordHeader.record.dataSize);
}

void ESM4::Reader::skipSubRecordData()
{
    mStream->ignore(mSubRecordHeader.dataSize);
}

void ESM4::Reader::skipSubRecordData(std::uint32_t size)
{
    mStream->ignore(size);
}

// ModIndex adjusted formId according to master file dependencies
// (see http://www.uesp.net/wiki/Tes4Mod:FormID_Fixup)
// NOTE: need to update modindex to mModIndices.size() before saving
//
// FIXME: probably should add a parameter to check for mHeader::mOverrides
//        (ACHR, LAND, NAVM, PGRE, PHZD, REFR), but not sure what exactly overrides mean
//        i.e. use the modindx of its master?
// FIXME: Apparently ModIndex '00' in an ESP means the object is defined in one of its masters.
//        This means we may need to search multiple times to get the correct id.
//        (see https://www.uesp.net/wiki/Tes4Mod:Formid#ModIndex_Zero)
void ESM4::Reader::adjustFormId(FormId& id)
{
    if (mHeader.mModIndices.empty())
        return;

    std::size_t index = (id >> 24) & 0xff;

    if (index < mHeader.mModIndices.size())
        id = mHeader.mModIndices[index] | (id & 0x00ffffff);
    else
        id = mCtx.modIndex | (id & 0x00ffffff);
}

bool ESM4::Reader::getFormId(FormId& id)
{
    if (!getExact(id))
        return false;

    adjustFormId(id);
    return true;
}

void ESM4::Reader::adjustGRUPFormId()
{
    adjustFormId(mRecordHeader.group.label.value);
}
