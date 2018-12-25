/*
  Copyright (C) 2018 cc9cii

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
#ifndef NIBTOGRE_NIMESHLOADER_H
#define NIBTOGRE_NIMESHLOADER_H

#include <vector>

#include <OgreResource.h>

namespace NiBtOgre
{
    struct NiGeometry;
    struct BtOgreInst;

    class NiMeshLoader : public Ogre::ManualResourceLoader
    {
        BtOgreInst *mInstance;

        std::vector<NiGeometry*> mSubMeshGeometry; // registered NiNode children for the mesh
    public:

        NiMeshLoader(BtOgreInst* inst) : mInstance(inst) {}

        const std::vector<NiGeometry*>& getSubMeshGeometry() { return mSubMeshGeometry; }

        // returns the index number of registered NiGeometry
        /*std::uint32_t*/void registerSubMeshGeometry(NiGeometry* geometry);

        // reimplement Ogre::ManualResourceLoader
        virtual void loadResource(Ogre::Resource *resource);
    };
}

#endif // NIBTOGRE_NIMESHLOADER_H