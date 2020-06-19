/*
  Copyright (C) 2019, 2020 cc9cii

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
#include "fgstream.hpp"

#include <cassert>
#include <stdexcept>

//#include <iostream> // FIXME: debugging only

#include <boost/scoped_array.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>

//#ifdef NDEBUG // FIXME: debugging only
//#undef NDEBUG
//#endif

namespace FgLib
{
    FgStream::FgStream(const std::string& name)
        : mStream(Ogre::ResourceGroupManager::getSingleton().openResource(name))
    {
    }

    FgStream::~FgStream()
    {
    }

    std::string FgStream::readString()
    {
        std::string str;
        readSizedString(str);
        //return std::move(str); // FIXME: test which is better
        return str;
    }

    void FgStream::readSizedString(std::string& str)
    {
        std::uint32_t size = 0;
        mStream->read(&size, sizeof(size));
        assert((mStream->size() > size) && "NiBtOgre::FgStream - string size too large");

        // FIXME: how to without using a temp buffer?
        boost::scoped_array<char> buf(new char[size]); // size includes null terminator
        if (mStream->read(buf.get(), size) == (size_t)size)
        {
            if (buf[size-1] != 0) // check null terminator position
                throw std::runtime_error ("NiBtOgre::FgStream - read string size mismatch");

            str.assign(buf.get(), size-1); // don't copy null terminator
        }
        else
        {
            str.clear();
            throw std::runtime_error ("NiBtOgre::FgStream - read error");
        }
    }

    template<>
    void FgStream::read<Ogre::Vector3>(Ogre::Vector3& value)
    {
        mStream->read(&value.x, sizeof(float));
        mStream->read(&value.y, sizeof(float));
        mStream->read(&value.z, sizeof(float));
    }

    template<>
    void FgStream::read<Ogre::Vector4>(Ogre::Vector4& value)
    {
        mStream->read(&value.x, sizeof(float));
        mStream->read(&value.y, sizeof(float));
        mStream->read(&value.z, sizeof(float));
        mStream->read(&value.w, sizeof(float));
    }

    template<>
    void FgStream::read<Ogre::Matrix3>(Ogre::Matrix3& value)
    {
        float a[3][3];
        for(size_t i = 0;i < 3;i++)
        {
            for(unsigned int j = 0; j < 3; ++j)
                mStream->read(&a[i][j], sizeof(float));
        }
        value = std::move(Ogre::Matrix3(a));
    }
}
