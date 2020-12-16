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

  Below code is based on Ogre::SkeletonManager and Ogre::MeshManager.

*/
#ifndef NIBTOGRE_NIMODELMANAGER_H
#define NIBTOGRE_NIMODELMANAGER_H

#include <map>

#include <OgreResourceManager.h>
#include <OgreSingleton.h>
#include <OgreResource.h>

#include "nimeshloader.hpp"
#include "niskeletonloader.hpp"

namespace Ogre
{
    class Skeleton;
}

namespace ESM4
{
    struct Npc;
    struct Race;
}

namespace NiBtOgre
{
    class NiModel;
}
typedef Ogre::SharedPtr<NiBtOgre::NiModel> NiModelPtr;

namespace NiBtOgre
{
    class NiModelManager
        : public Ogre::ResourceManager, public Ogre::Singleton<NiModelManager>, public Ogre::ManualResourceLoader
    {
    public:
        enum ModelBodyPart
        {
            BP_Head        = 0,
            BP_EarMale     = 1,
            BP_EarFemale   = 2,
            BP_Mouth       = 3,
            BP_TeethLower  = 4,
            BP_TeethUpper  = 5,
            BP_Tongue      = 6,
            BP_EyeLeft     = 7,
            BP_EyeRight    = 8, // NOTE: up to here the same as ESM4::Race::HeadPartIndex
            BP_UpperBody   = 9,
            BP_LowerBody   = 10,
            BP_Hair        = 11
        };

    private:
        enum ModelBuildType
        {
            MBT_Object,
            MBT_Skinned,
            MBT_Skeleton,
            MBT_Morphed,
            MBT_FO_Hair,
            MBT_Anim
        };

        struct ModelBuildInfo
        {
            ModelBuildType type;
            const ESM4::Npc *npc;   // FIXME: danger in holding pointers here?
            const ESM4::Race *race;
            const NiModel *skel;
            Ogre::String skelNif;
            Ogre::String skelGroup;
            ModelBodyPart bodyPart;
            Ogre::String raceName; // TODO: derive from race instead? (then we need race)
            Ogre::String baseNif;
            Ogre::String baseTexture;
            bool hat;
        };

        std::map<Ogre::Resource*, ModelBuildInfo> mModelBuildInfoMap;

        NiSkeletonLoader mSkeletonLoader;
        NiMeshLoader mMeshLoader;

        void loadManualModel(NiModel* pModel, const ModelBuildInfo& params);
        void loadManualSkinnedModel(NiModel* pModel, const ModelBuildInfo& params);
        void loadManualSkeletonModel(NiModel* pModel, const ModelBuildInfo& params);
        void loadManualMorphedModel(NiModel* pModel, const ModelBuildInfo& params);
        void loadManualHairModel(NiModel* pModel, const ModelBuildInfo& params);
        void loadManualAnimModel(NiModel* pModel, const ModelBuildInfo& params);

    public:
        NiModelManager();
        ~NiModelManager();

        NiModelPtr getByName(const Ogre::String& name,
                const Ogre::String& group = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        NiModelPtr create(const Ogre::String& name, const Ogre::String& group,
                bool isManual = false,
                Ogre::ManualResourceLoader* loader = 0,
                const Ogre::NameValuePairList* createParams = 0);

        NiModelPtr createManual(const Ogre::String& name, const Ogre::String& group,
                const Ogre::String& nif, Ogre::ManualResourceLoader* loader);

        static NiModelManager& getSingleton(void);
        static NiModelManager* getSingletonPtr(void);

        void loadResource(Ogre::Resource* res);

        NiModelPtr createManualModel(const Ogre::String& name, const Ogre::String& group,
                const Ogre::String& raceName, const Ogre::String& texture = "");

        NiModelPtr createSkinnedModel(const Ogre::String& name, const Ogre::String& group,
                NiModel *skeleton, const Ogre::String& raceName, const Ogre::String& texture = "");

        NiModelPtr createSkeletonModel(const Ogre::String& name, const Ogre::String& group);

        NiModelPtr createMorphedModel(const Ogre::String& nif, const Ogre::String& group,
                const ESM4::Npc *npc, const ESM4::Race *race, NiModel *skeleton, const Ogre::String& texture,
                ModelBodyPart bodyPart);

        NiModelPtr createMorphedHair(const Ogre::String& nif, const Ogre::String& group,
                const ESM4::Npc *npc, const ESM4::Race *race, NiModel *skeleton, const Ogre::String& texture,
                ModelBodyPart bodyPart, bool hat);

        NiModelPtr createAnimModel(const Ogre::String& name, const Ogre::String& group,
                NiModel *skeleton);

        NiModelPtr getOrLoadByName(const Ogre::String& name,
                const Ogre::String& group = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        NiSkeletonLoader& skeletonLoader() { return mSkeletonLoader; }
        NiMeshLoader& meshLoader() { return mMeshLoader; }

    protected:
        Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
                const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                const Ogre::NameValuePairList* createParams);
    };
}

#endif // NIBTOGRE_NIMODELMANAGER_H