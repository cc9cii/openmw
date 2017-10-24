/*
  Copyright (C) 2015-2017 cc9cii

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
#include "nimodel.hpp"

#include <stdexcept>
//#include <cassert>

#include <iostream> // FIXME: debugging only
#ifdef NDEBUG       // FIXME: debugging only
#undef NDEBUG
#endif

#include "niobject.hpp"

// "name" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// The file is opened by mNiStream::mStream.
NiBtOgre::NiModel::NiModel(const std::string &name) : mNiStream(name), mHeader(mNiStream)
{
    mObjects.resize(mHeader.numBlocks());
    if (mNiStream.nifVer() >= 0x0a000100) // from 10.0.1.0
    {

        for (uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            //mObjects[i] = NiObject::create(mHeader.blockType(i)/*, mNiStream*/);
            ////mObjects[i] = NiObject::create(mHeader.blockType(i), mNiStream, mObjects);
            mObjects[i] = NiObject::create(mHeader.blockType(i), mNiStream, *this);
            //mObjects[i]->create(mNiStream);
        }
    }
    else
    {
        for (uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            // For TES3, the object type string is read first to determine the type.
            //mObjects[i] = NiObject::create(mNiStream.getString()/*, mNiStream*/);
            ////mObjects[i] = NiObject::create(mNiStream.getString(), mNiStream, mObjects);
            mObjects[i] = NiObject::create(mNiStream.readString(), mNiStream, *this);
            //mObjects[i]->create(mNiStream);
        }
    }

    // TODO: should assert that the first object, i.e. mObjects[0], is either a NiNode (TES3/TES4)
    //       or BSFadeNode (TES5)

    // read the footer to check for root nodes
    uint32_t numRoots = 0;
    mNiStream.read(numRoots);

    mRoots.resize(numRoots);
    for (uint32_t i = 0; i < numRoots; ++i)
        mNiStream.read(mRoots.at(i));

    if (numRoots == 0)
        throw std::runtime_error(name + " has no roots");
    else if (numRoots > 1) // FIXME: debugging only, to find out which NIF has multiple roots
        std::cout << name << " has numRoots: " << numRoots << std::endl;
}

NiBtOgre::NiModel::~NiModel()
{
}

// modifies scenenode and scene
void NiBtOgre::NiModel::build(Ogre::SceneNode *sceneNode, NifOgre::ObjectScenePtr scene)
{
    // build the first root
    mObjects[mRoots[0]]->build(mObjects, mHeader, sceneNode, scene);

    // FIXME: what to do with other roots?
}
