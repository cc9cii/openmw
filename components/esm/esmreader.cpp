#include "esmreader.hpp"

#include <stdexcept>

namespace ESM
{

using namespace Misc;

ESM_Context ESMReader::getContext()
{
    // Update the file position before returning
    mCtx.filePos = mEsm->tellg();
    return mCtx;
}

ESMReader::ESMReader()
    : mRecordFlags(0)
    , mBuffer(50*1024)
    , mGlobalReaderList(nullptr)
    , mEncoder(nullptr)
    , mFileSize(0)
{
    clearCtx();
}

void ESMReader::restoreContext(const ESM_Context &rc)
{
    // Reopen the file if necessary
    if (mCtx.filename != rc.filename)
        openRaw(rc.filename);

    // Copy the data
    mCtx = rc;

    // Make sure we seek to the right place
    mEsm->seekg(mCtx.filePos);
}

void ESMReader::close()
{
    mEsm.reset();
    clearCtx();
    mHeader.blank();
}

void ESMReader::clearCtx()
{
   mCtx.filename.clear();
   mCtx.leftFile = 0;
   mCtx.leftRec = 0;
   mCtx.leftSub = 0;
   mCtx.subCached = false;
   mCtx.recName.clear();
   mCtx.subName.clear();
}

void ESMReader::openRaw(Files::IStreamPtr _esm, const std::string& name)
{
    close();
    mEsm = _esm;
    mCtx.filename = name;
    mEsm->seekg(0, mEsm->end);
    mFileSize = mEsm->tellg();
    mCtx.leftFile = mFileSize;
    mEsm->seekg(0, mEsm->beg);
}

void ESMReader::openRaw(const std::string& filename)
{
    openRaw(Files::openConstrainedFileStream(filename.c_str()), filename);
}

void ESMReader::open(Files::IStreamPtr _esm, const std::string &name)
{
    openRaw(_esm, name);

    NAME modVer = getRecName();
    if (modVer == "TES3")
    {
        getRecHeader();

        mHeader.load (*this);
    }
    else if (modVer == "TES4")
    {
        mHeader.mData.author.assign("");
        mHeader.mData.desc.assign("");
        char buf[512]; // arbitrary number
        unsigned short size;

        skip(16); // skip the rest of the header, note it may be 4 bytes longer

        NAME rec = getRecName();
        if (rec != "HEDR")
            rec = getRecName(); // adjust for extra 4 bytes
        bool readRec = true;

        while (mFileSize - mEsm->tellg() >= 4) // Shivering Isle or Bashed Patch can end here
        {
            if (!readRec) // may be already read
                rec = getRecName();
            else
                readRec = false;

            switch (rec.intval)
            {
                case 0x52444548: // HEDR
                {
                    skip(2); // data size
                    getT(mHeader.mData.version);
                    getT(mHeader.mData.records);
                    skip(4); // skip next available object id
                    break;
                }
                case 0x4d414e43: // CNAM
                {
                    getT(size);
                    getExact(buf, size);
                    std::string author;
                    size = std::min(size, (unsigned short)32); // clamp for TES3 format
                    author.assign(buf, size - 1); // don't copy null terminator
                    mHeader.mData.author.assign(author);
                    break;
                }
                case 0x4d414e53: // SNAM
                {
                    getT(size);
                    getExact(buf, size);
                    std::string desc;
                    size = std::min(size, (unsigned short)256); // clamp for TES3 format
                    desc.assign(buf, size - 1); // don't copy null terminator
                    mHeader.mData.desc.assign(desc);
                    break;
                }
                case 0x5453414d: // MAST
                {
                    Header::MasterData m;
                    getT(size);
                    getExact(buf, size);
                    m.name.assign(buf, size-1); // don't copy null terminator

                    rec = getRecName();
                    if (rec == "DATA")
                    {
                        getT(size);
                        getT(m.size); // 64 bits
                    }
                    else
                    {
                        // some esp's don't have DATA subrecord
                        m.size = 0;
                        readRec = true; // don't read again at the top of while loop
                    }
                    mHeader.mMaster.push_back (m);
                    break;
                }
                case 0x56544e49: // INTV
                case 0x43434e49: // INCC
                case 0x4d414e4f: // ONAM
                {
                    getT(size);
                    skip(size);
                    break;
                }
                case 0x50555247: // GRUP
                default:
                    return;      // all done
            }
        }
        return;
    }
    else
        fail("Not a valid Morrowind file");
}

void ESMReader::open(const std::string &file)
{
    open (Files::openConstrainedFileStream (file.c_str ()), file);
}

std::string ESMReader::getHNOString(const char* name)
{
    if (isNextSub(name))
        return getHString();
    return "";
}

std::string ESMReader::getHNString(const char* name)
{
    getSubNameIs(name);
    return getHString();
}

std::string ESMReader::getHString()
{
    getSubHeader();

    // Hack to make MultiMark.esp load. Zero-length strings do not
    // occur in any of the official mods, but MultiMark makes use of
    // them. For some reason, they break the rules, and contain a byte
    // (value 0) even if the header says there is no data. If
    // Morrowind accepts it, so should we.
    if (mCtx.leftSub == 0 && hasMoreSubs() && !mEsm->peek())
    {
        // Skip the following zero byte
        mCtx.leftRec--;
        char c;
        getT(c);
        return std::string();
    }

    return getString(mCtx.leftSub);
}

void ESMReader::getHExact(void*p, int size)
{
    getSubHeader();
    if (size != static_cast<int> (mCtx.leftSub))
        fail("Size mismatch, requested " + std::to_string(size) + " but got "
                + std::to_string(mCtx.leftSub));
    getExact(p, size);
}

// Read the given number of bytes from a named subrecord
void ESMReader::getHNExact(void*p, int size, const char* name)
{
    getSubNameIs(name);
    getHExact(p, size);
}

// Get the next subrecord name and check if it matches the parameter
void ESMReader::getSubNameIs(const char* name)
{
    getSubName();
    if (mCtx.subName != name)
        fail("Expected subrecord " + std::string(name) + " but got " + mCtx.subName.toString());
}

bool ESMReader::isNextSub(const char* name)
{
    if (!hasMoreSubs())
        return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    mCtx.subCached = (mCtx.subName != name);

    // If subCached is false, then subName == name.
    return !mCtx.subCached;
}

bool ESMReader::peekNextSub(const char *name)
{
    if (!hasMoreSubs())
        return false;

    getSubName();

    mCtx.subCached = true;
    return mCtx.subName == name;
}

// Read subrecord name. This gets called a LOT, so I've optimized it
// slightly.
void ESMReader::getSubName()
{
    // If the name has already been read, do nothing
    if (mCtx.subCached)
    {
        mCtx.subCached = false;
        return;
    }

    // reading the subrecord data anyway.
    const int subNameSize = static_cast<int>(mCtx.subName.data_size());
    getExact(mCtx.subName.rw_data(), subNameSize);
    mCtx.leftRec -= static_cast<uint32_t>(subNameSize);
}

void ESMReader::skipHSub()
{
    getSubHeader();
    skip(mCtx.leftSub);
}

void ESMReader::skipHSubSize(int size)
{
    skipHSub();
    if (static_cast<int> (mCtx.leftSub) != size)
        fail("skipHSubSize() mismatch");
}

void ESMReader::skipHSubUntil(const char *name)
{
    while (hasMoreSubs() && !isNextSub(name))
    {
        mCtx.subCached = false;
        skipHSub();
    }
    if (hasMoreSubs())
        mCtx.subCached = true;
}

void ESMReader::getSubHeader()
{
    if (mCtx.leftRec < sizeof(mCtx.leftSub))
        fail("End of record while reading sub-record header");

    // Get subrecord size
    getT(mCtx.leftSub);
    mCtx.leftRec -= sizeof(mCtx.leftSub);

    // Adjust number of record bytes left
    if (mCtx.leftRec < mCtx.leftSub)
        fail("Record size is larger than rest of file");
    mCtx.leftRec -= mCtx.leftSub;
}

NAME ESMReader::getRecName()
{
    if (!hasMoreRecs())
        fail("No more records, getRecName() failed");
    getName(mCtx.recName);
    mCtx.leftFile -= mCtx.recName.data_size();

    // Make sure we don't carry over any old cached subrecord
    // names. This can happen in some cases when we skip parts of a
    // record.
    mCtx.subCached = false;

    return mCtx.recName;
}

void ESMReader::skipRecord()
{
    skip(mCtx.leftRec);
    mCtx.leftRec = 0;
    mCtx.subCached = false;
}

void ESMReader::getRecHeader(uint32_t &flags)
{
    // General error checking
    if (mCtx.leftFile < 3 * sizeof(uint32_t))
        fail("End of file while reading record header");
    if (mCtx.leftRec)
        fail("Previous record contains unread bytes");

    getUint(mCtx.leftRec);
    getUint(flags);// This header entry is always zero
    getUint(flags);
    mCtx.leftFile -= 3 * sizeof(uint32_t);

    // Check that sizes add up
    if (mCtx.leftFile < mCtx.leftRec)
        fail("Record size is larger than rest of file");

    // Adjust number of bytes mCtx.left in file
    mCtx.leftFile -= mCtx.leftRec;
}

/*************************************************************************
 *
 *  Lowest level data reading and misc methods
 *
 *************************************************************************/

std::string ESMReader::getString(int size)
{
    size_t s = size;
    if (mBuffer.size() <= s)
        // Add some extra padding to reduce the chance of having to resize
        // again later.
        mBuffer.resize(3*s);

    // And make sure the string is zero terminated
    mBuffer[s] = 0;

    // read ESM data
    char *ptr = mBuffer.data();
    getExact(ptr, size);

    size = static_cast<int>(strnlen(ptr, size));

    // Convert to UTF8 and return
    if (mEncoder)
        return mEncoder->getUtf8(ptr, size);

    return std::string (ptr, size);
}

[[noreturn]] void ESMReader::fail(const std::string &msg)
{
    std::stringstream ss;

    ss << "ESM Error: " << msg;
    ss << "\n  File: " << mCtx.filename;
    ss << "\n  Record: " << mCtx.recName.toString();
    ss << "\n  Subrecord: " << mCtx.subName.toString();
    if (mEsm.get())
        ss << "\n  Offset: 0x" << std::hex << mEsm->tellg();
    throw std::runtime_error(ss.str());
}

}
