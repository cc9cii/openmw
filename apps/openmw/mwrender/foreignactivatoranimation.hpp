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
    public:
        ForeignActivatorAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~ForeignActivatorAnimation();

        virtual void play(const std::string &groupname, int priority, int groups, bool autodisable,
            float speedmult, const std::string &start, const std::string &stop,
            float startpoint, size_t loops, bool loopfallback);

        virtual Ogre::Vector3 runAnimation(float duration);

        void addActivatorAnimSource();
    };
}

#endif
