#ifndef GAME_RENDER_FOREIGNACTIVATORANIMATION_H
#define GAME_RENDER_FOREIGNACTIVATORANIMATION_H

#include "animation.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class ForeignActivatorAnimation : public ObjectAnimation
    {
        // FIXME: for temp testing
#if 0
        NifOgre::ObjectScenePtr scene;
        Ogre::TagPoint *tag;
        float zPos;
#endif
    public:
        ForeignActivatorAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~ForeignActivatorAnimation();

        void addActivatorAnimSource();

        void play(const std::string &groupname, int priority, int groups, bool autodisable,
            float speedmult, const std::string &start, const std::string &stop,
            float startpoint, size_t loops, bool loopfallback);

        Ogre::Vector3 runAnimation(float duration);
    };
}

#endif
