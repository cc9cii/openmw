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

#include "fgfile.hpp"

#include <boost/algorithm/string.hpp>

#include "fgtri.hpp"
#include "fgegm.hpp"
#include "fgegt.hpp"
#include "fgctl.hpp"

namespace FgLib
{
    template<>
    const FgTri *FgFile<FgTri>::getOrLoadByMeshName(const std::string& mesh) const
    {
        return getOrLoadByMeshName(boost::algorithm::to_lower_copy(mesh), "tri");
    }

    template<>
    const FgEgm *FgFile<FgEgm>::getOrLoadByMeshName(const std::string& mesh) const
    {
        return getOrLoadByMeshName(boost::algorithm::to_lower_copy(mesh), "egm");
    }

    template<>
    const FgEgt *FgFile<FgEgt>::getOrLoadByMeshName(const std::string& mesh) const
    {
        return getOrLoadByMeshName(boost::algorithm::to_lower_copy(mesh), "egt");
    }

    template<>
    const FgTri *FgFile<FgTri>::replaceFile(const std::string& name, std::unique_ptr<FgTri> fgFile) const
    {
        std::map<std::string, std::unique_ptr<FgTri> >::iterator lb = sFgFileMap.lower_bound(name);
        if (lb != sFgFileMap.end() && !(sFgFileMap.key_comp()(name, lb->first)))
        {
            // there should be an entry already
            std::swap(lb->second, fgFile);
            return lb->second.get();
        }
        else
            return nullptr; // should never be a new entry - throw?
    }

    template<>
    std::map<std::string, std::unique_ptr<FgTri> > FgFile<FgTri>::sFgFileMap = std::map<std::string, std::unique_ptr<FgTri> >();
    template<>
    std::map<std::string, std::unique_ptr<FgEgm> > FgFile<FgEgm>::sFgFileMap = std::map<std::string, std::unique_ptr<FgEgm> >();
    template<>
    std::map<std::string, std::unique_ptr<FgEgt> > FgFile<FgEgt>::sFgFileMap = std::map<std::string, std::unique_ptr<FgEgt> >();
    template<>
    std::map<std::string, std::unique_ptr<FgCtl> > FgFile<FgCtl>::sFgFileMap = std::map<std::string, std::unique_ptr<FgCtl> >();
}
