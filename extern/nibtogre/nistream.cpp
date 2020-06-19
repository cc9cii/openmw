/*
  Copyright (C) 2015-2018, 2020 cc9cii

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
#include "nistream.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debugging only

#include <boost/scoped_array.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>
#include <OgreQuaternion.h>

#include <btBulletDynamicsCommon.h>

#include "niheader.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiStream::NiStream(const std::string& name)
    : mStream(Ogre::ResourceGroupManager::getSingleton().openResource(name)), mHeader(nullptr),
      mVer(0), mUserVer(0), mUserVer2(0)
{
}

NiBtOgre::NiStream::~NiStream()
{
}

void NiBtOgre::NiStream::setHeader(NiBtOgre::NiHeader *header)
{
    mHeader = header;
}

std::string NiBtOgre::NiStream::readString()
{
    std::string str;
    readSizedString(str);
    //return std::move(str);
    return str;
}

void NiBtOgre::NiStream::readSizedString(std::string& str)
{
    std::uint32_t size = 0;
    mStream->read(&size, sizeof(size));
    assert((mStream->size() > size) && "NiBtOgre::NiStream - string size too large");

    // FIXME: how to without using a temp buffer?
    boost::scoped_array<char> buf(new char[size+1]);
    if (mStream->read(buf.get(), size) == (size_t)size)
    {
        //if (buf[size] != 0) // check null terminator position
            //throw std::runtime_error ("NiBtOgre::NiStream - read string size mismatch");

        str.assign(buf.get(), size); // don't copy null terminator
    }
    else
    {
        str.clear();
        throw std::runtime_error ("NiBtOgre::NiStream - read error");
    }
}

// FIXME: almost identical to readSizedString, exceptt size is uint8_t
void NiBtOgre::NiStream::readShortString(std::string& str)
{
    std::uint8_t size = 0;
    mStream->read(&size, sizeof(size));
    assert((mStream->size() > size) && "NiBtOgre::NiStream - string size too large");

    // FIXME: how to without using a temp buffer?
    boost::scoped_array<char> buf(new char[size+1]); // add space for null terminator
    if (mStream->read(buf.get(), size) == (size_t)size)
    {
        //if (buf[size] != 0) // check null terminator position
            //throw std::runtime_error ("NiBtOgre::NiStream - read string size mismatch");

        str.assign(buf.get(), size); // don't copy null terminator
    }
    else
    {
        str.clear();
        throw std::runtime_error ("NiBtOgre::NiStream - read error");
    }
}

void NiBtOgre::NiStream::readLongString(std::uint32_t& index)
{
    if (mVer <= 0x14000005) // to 20.0.0.5 (TES3/TES4)
    {
        std::string str;
        readSizedString(str);

        assert(mHeader);
        index = mHeader->appendLongString(std::move(str));
    }
    else                    // from 20.2.0.7 (TES5)
        read(index);
}

bool NiBtOgre::NiStream::getBool()
{
    if (mVer >= 0x04010001) // from 4.1.0.1
    {
        char c;
        mStream->read(&c, sizeof(char));
        return !!c;
    }
    else //     0x04000002     e.g  4.0.0.2
    {
        std::int32_t i;
        mStream->read(&i, sizeof(std::int32_t));
        return !!i;
    }
}

void NiBtOgre::NiStream::getBool(bool& value)
{
    if (mVer >= 0x04010001) // from 4.1.0.1
    {
        char c;
        mStream->read(&c, sizeof(char));
        value = !!c;
    }
    else //     0x04000002     e.g  4.0.0.2
    {
        std::int32_t i;
        mStream->read(&i, sizeof(std::int32_t));
        value = !!i;
    }
}

template<>
void NiBtOgre::NiStream::read<btVector3>(btVector3& value)
{
    mStream->read(&value.m_floats[0], sizeof(float));
    mStream->read(&value.m_floats[1], sizeof(float));
    mStream->read(&value.m_floats[2], sizeof(float));
    value.m_floats[3] = 0.f;
}

template<>
void NiBtOgre::NiStream::read<Ogre::Vector3>(Ogre::Vector3& value)
{
    mStream->read(&value.x, sizeof(float));
    mStream->read(&value.y, sizeof(float));
    mStream->read(&value.z, sizeof(float));
}

template<>
void NiBtOgre::NiStream::read<Ogre::Vector4>(Ogre::Vector4& value)
{
    mStream->read(&value.x, sizeof(float));
    mStream->read(&value.y, sizeof(float));
    mStream->read(&value.z, sizeof(float));
    mStream->read(&value.w, sizeof(float));
}

template<>
void NiBtOgre::NiStream::read<Ogre::Matrix3>(Ogre::Matrix3& value)
{
    float a[3][3];
    for(size_t i = 0;i < 3;i++)
    {
        for(unsigned int j = 0; j < 3; ++j)
            mStream->read(&a[i][j], sizeof(float));
    }
    value = std::move(Ogre::Matrix3(a));
}

template<>
void NiBtOgre::NiStream::read<Ogre::Quaternion>(Ogre::Quaternion& value)
{
    mStream->read(&value.w, sizeof(float));
    mStream->read(&value.x, sizeof(float));
    mStream->read(&value.y, sizeof(float));
    mStream->read(&value.z, sizeof(float));
}

void NiBtOgre::NiStream::readQuaternionXYZW(Ogre::Quaternion& value)
{
    mStream->read(&value.x, sizeof(float));
    mStream->read(&value.y, sizeof(float));
    mStream->read(&value.z, sizeof(float));
    mStream->read(&value.w, sizeof(float));
}

void NiBtOgre::NiStream::readBtVector3(btVector3& value)
{
    mStream->read(&value.m_floats[0], sizeof(float));
    mStream->read(&value.m_floats[1], sizeof(float));
    mStream->read(&value.m_floats[2], sizeof(float));
    mStream->read(&value.m_floats[3], sizeof(float));
}
