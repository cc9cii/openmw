/*
  Copyright (C) 2020 cc9cii

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
#ifndef FGLIB_FGFILE_H
#define FGLIB_FGFILE_H

#include <map>
#include <string>
#include <memory>
#include <iostream>

#include <boost/algorithm/string/case_conv.hpp>

#include <OgreException.h>

namespace FgLib
{
    template<typename T>
    class FgFile
    {
        // NOTE: There is no real management of FaceGen resources since there
        //       aren't that many of them (i.e. they are never unloaded).
        // NOTE: No fancy hashing of the std::string keys, either.
        static std::map<std::string, std::unique_ptr<T> > sFgFileMap;

        const T *getOrLoadByMeshName(const std::string& name, const std::string& ext) const;

        const T *getOrLoadByNameImpl(const std::string& base) const;

        FgFile(const FgFile& other);
        FgFile& operator=(const FgFile& other);

    public:
        FgFile() {}
        ~FgFile() {}

        const T* getOrLoadByMeshName(const std::string& base) const;
        const T *getOrLoadByName(const std::string& name) const;

        const T* replaceFile(const std::string& name, std::unique_ptr<T> fgFile) const;
    };

    template<typename T>
    const T *FgFile<T>::getOrLoadByMeshName(const std::string& mesh, const std::string& ext) const
    {
        std::string name = mesh;
        size_t pos = mesh.find_last_of(".");
        if (pos != std::string::npos && name.substr(pos+1) == "nif")
            name = mesh.substr(0, pos+1)+ext;

        return getOrLoadByNameImpl(name);
    }

    template<typename T>
    const T *FgFile<T>::getOrLoadByName(const std::string& name) const
    {
        return getOrLoadByNameImpl(boost::algorithm::to_lower_copy(name));
    }

    template<typename T>
    const T *FgFile<T>::getOrLoadByNameImpl(const std::string& name) const
    {
        typename std::map<std::string, std::unique_ptr<T> >::iterator lb = sFgFileMap.lower_bound(name);
        if (lb != sFgFileMap.end() && !(sFgFileMap.key_comp()(name, lb->first)))
        {
            return lb->second.get();
        }
        else // none found, create one
        {
            try
            {
#if defined(__GNUC__) && __GNUC__ < 8
                std::unique_ptr<T> fgFile = std::unique_ptr<T>(new T(name));
#else
                std::unique_ptr<T> fgFile = std::make_unique<T>(name);
#endif
                lb = sFgFileMap.insert(lb,
                        typename std::map<std::string, std::unique_ptr<T> >::value_type(name, std::move(fgFile)));

                return lb->second.get();
            }
            catch (Ogre::Exception e)
            {
                // most likely resource not found by Ogre
                std::cerr << "FgFile: " << e.getFullDescription() << std::endl;

                return nullptr;
            }
        }
    }
}

#endif // FGLIB_FGFILE_H
