#include "soundmanagerimp.hpp"

#include <iostream>
#include <algorithm>
#include <map>
#include <cmath> // fmod

#include <OgreResourceGroupManager.h>

#include <components/misc/rng.hpp>
#include <components/misc/stringops.hpp>

#include <extern/esm4/formid.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"
#ifdef MKTAG /* ESM4 dragged in by esmstore.hpp and cellstore.hpp */
#undef MKTAG
#endif

#include "sound_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"

#include "openal_output.hpp"
#define SOUND_OUT "OpenAL"
#include "ffmpeg_decoder.hpp"
#ifndef SOUND_IN
#define SOUND_IN "FFmpeg"
#endif


namespace MWSound
{
    SoundManager::SoundManager(bool useSound)
        : mResourceMgr(Ogre::ResourceGroupManager::getSingleton())
        , mOutput(new DEFAULT_OUTPUT(*this))
        , mMasterVolume(1.0f)
        , mSFXVolume(1.0f)
        , mMusicVolume(1.0f)
        , mVoiceVolume(1.0f)
        , mFootstepsVolume(1.0f)
        , mListenerUnderwater(false)
        , mListenerPos(0,0,0)
        , mListenerDir(1,0,0)
        , mListenerUp(0,0,1)
        , mPausedSoundTypes(0)
        , mUseForeign(false)
    {
        if(!useSound)
            return;

        mMasterVolume = Settings::Manager::getFloat("master volume", "Sound");
        mMasterVolume = std::min(std::max(mMasterVolume, 0.0f), 1.0f);
        mSFXVolume = Settings::Manager::getFloat("sfx volume", "Sound");
        mSFXVolume = std::min(std::max(mSFXVolume, 0.0f), 1.0f);
        mMusicVolume = Settings::Manager::getFloat("music volume", "Sound");
        mMusicVolume = std::min(std::max(mMusicVolume, 0.0f), 1.0f);
        mVoiceVolume = Settings::Manager::getFloat("voice volume", "Sound");
        mVoiceVolume = std::min(std::max(mVoiceVolume, 0.0f), 1.0f);
        mFootstepsVolume = Settings::Manager::getFloat("footsteps volume", "Sound");
        mFootstepsVolume = std::min(std::max(mFootstepsVolume, 0.0f), 1.0f);

        std::cout << "Sound output: " << SOUND_OUT << std::endl;
        std::cout << "Sound decoder: " << SOUND_IN << std::endl;

        try
        {
            std::vector<std::string> names = mOutput->enumerate();
            std::cout <<"Enumerated output devices:"<< std::endl;
            for(size_t i = 0;i < names.size();i++)
                std::cout <<"  "<<names[i]<< std::endl;

            std::string devname = Settings::Manager::getString("device", "Sound");
            try
            {
                mOutput->init(devname);
            }
            catch(std::exception &e)
            {
                if(devname.empty())
                    throw;
                std::cerr <<"Failed to open device \""<<devname<<"\": " << e.what() << std::endl;
                mOutput->init();
                Settings::Manager::setString("device", "Sound", "");
            }
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound init failed: "<<e.what()<< std::endl;
        }

        // HACK: to allow playlist directories with same names
        // TODO: the strings used for matching the game type could be put in the config
        // (the default playPlaylist is TES3)
        Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
        for (std::size_t i = 0; i < groups.size(); ++i)
        {
            Ogre::StringVectorPtr list
                = Ogre::ResourceGroupManager::getSingleton().listResourceLocations(groups[i]);
            for (std::size_t j = 0; j < list->size(); ++j)
            {
                std::string loc = Misc::StringUtils::lowerCase(list->at(j));
                if (loc.find("morrowind") != std::string::npos)
                    mMusicResourceGroups.push_back(groups[i]);
                else if (loc.find("oblivion") != std::string::npos ||
                         loc.find("fallout") != std::string::npos ||
                         loc.find("skyrim") != std::string::npos)
                {
                    mForeignMusicResourceGroups.push_back(groups[i]);
                    //std::cout << "group " << groups[i] << ", " << list->at(j) << std::endl; // FIXME
                }
            }
        }
    }

    SoundManager::~SoundManager()
    {
        mUnderwaterSound.reset();
        mActiveSounds.clear();
        mMusic.reset();
        mOutput.reset();
    }

    // Return a new decoder instance, used as needed by the output implementations
    DecoderPtr SoundManager::getDecoder()
    {
        return DecoderPtr(new DEFAULT_DECODER);
    }

    // Convert a soundId to file name, and modify the volume
    // according to the sounds local volume setting, minRange and
    // maxRange.
    std::string SoundManager::lookup(const std::string &soundId,
                       float &volume, float &min, float &max)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const ESM::Sound *snd = world->getStore().get<ESM::Sound>().find(soundId);

        volume *= static_cast<float>(pow(10.0, (snd->mData.mVolume / 255.0*3348.0 - 3348.0) / 2000.0));

        if(snd->mData.mMinRange == 0 && snd->mData.mMaxRange == 0)
        {
            static const float fAudioDefaultMinDistance = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMinDistance")->getFloat();
            static const float fAudioDefaultMaxDistance = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMaxDistance")->getFloat();
            min = fAudioDefaultMinDistance;
            max = fAudioDefaultMaxDistance;
        }
        else
        {
            min = snd->mData.mMinRange;
            max = snd->mData.mMaxRange;
        }

        static const float fAudioMinDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->getFloat();
        static const float fAudioMaxDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->getFloat();
        min *= fAudioMinDistanceMult;
        max *= fAudioMaxDistanceMult;
        min = std::max(min, 1.0f);
        max = std::max(min, max);

        return "Sound/"+snd->mSound;
    }

    std::string SoundManager::lookupForeign(const std::string &soundId, float &volume, float &min, float &max)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::ForeignStore<ESM4::Sound>& soundStore = world->getStore().getForeign<ESM4::Sound>();
        // FIXME: TES5 FormId 0003C816
        const ESM4::Sound *snd = soundStore.search(ESM4::stringToFormId(soundId));

        volume *= static_cast<float>(pow(10.0, (-snd->mData.staticAttenuation / 100) / 20.f));

        // 1454, 0.199526 if integer division by 100 (discards remainder so a little louder)
        // 1454, 0.187499 if floating point division by 100.f
        //std::cout << snd->mData.staticAttenuation << ", " << volume << std::endl; // FIXME
        std::cout << "sound volume " << volume << std::endl; // FIXME

        if(snd->mData.minAttenuation == 0 && snd->mData.maxAttenuation == 0)
        {
            static const float fAudioDefaultMinDistance
                = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMinDistance")->getFloat();
            static const float fAudioDefaultMaxDistance
                = world->getStore().get<ESM::GameSetting>().find("fAudioDefaultMaxDistance")->getFloat();
            min = fAudioDefaultMinDistance;
            max = fAudioDefaultMaxDistance;
        }
        else
        {
            min = float(snd->mData.minAttenuation * 5);
            max = float(snd->mData.maxAttenuation * 100);
        }

        static const float fAudioMinDistanceMult
            = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->getFloat();
        static const float fAudioMaxDistanceMult
            = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->getFloat();
        min *= fAudioMinDistanceMult;
        max *= fAudioMaxDistanceMult;
        min = std::max(min, 1.0f);
        max = std::max(min, max);

        std::string soundFile = snd->mSoundFile;
        // FO3 has weird file naming e.g. sound\fx\drs\metalsheet_01\close\drs_metalsheet_01_close.wav
        std::size_t pos = soundFile.find('.');
        if (pos != std::string::npos)
        {
            // FIXME: hacky workaround to handle some files that are specified in
            // FalloutNV.esm as ".wav" but only ".ogg" found in "Fallout - Sound.bsa"
            if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("sound\\"+soundFile) &&
                soundFile.find(".wav") != std::string::npos)
            {
                //std::cout << "FONV: acoustic space interior loop sound "
                    //<< soundFile.substr(0, pos)<<".ogg" << std::endl; // FIXME

                return "Sound\\"+soundFile.substr(0, pos)+".ogg";
            }
            else
                return "Sound\\"+soundFile;
        }
        else
        {
            Ogre::StringVector filelist;
            Ogre::StringVector& groups = mUseForeign ? mForeignMusicResourceGroups : mMusicResourceGroups;
            for (Ogre::StringVector::iterator it = groups.begin(); it != groups.end(); ++it)
            {
                Ogre::StringVectorPtr filesInThisGroup
                    = mResourceMgr.findResourceNames(*it, "Sound\\"+soundFile+"*");
                filelist.insert(filelist.end(), filesInThisGroup->begin(), filesInThisGroup->end());
            }

            if(!filelist.size())
                return "";

            int i = Misc::Rng::rollDice(int(filelist.size()));
            return filelist[i];
        }
    }

    // Gets the combined volume settings for the given sound type
    float SoundManager::volumeFromType(PlayType type) const
    {
        float volume = mMasterVolume;
        switch(type)
        {
            case Play_TypeSfx:
                volume *= mSFXVolume;
                break;
            case Play_TypeVoice:
                volume *= mVoiceVolume;
                break;
            case Play_TypeFoot:
                volume *= mFootstepsVolume;
                break;
            case Play_TypeMusic:
                volume *= mMusicVolume;
                break;
            case Play_TypeMask:
                break;
            default:
                break;
        }
        return volume;
    }

    bool SoundManager::isPlaying(const MWWorld::Ptr &ptr, const std::string &id) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == id)
                return snditer->first->isPlaying();
            ++snditer;
        }
        return false;
    }


    void SoundManager::stopMusic()
    {
        if(mMusic)
            mMusic->stop();
        mMusic.reset();
    }

    void SoundManager::streamMusicFull(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        std::cout <<"Playing "<<filename<< std::endl;
        mLastPlayedMusic = filename;
        try
        {
            stopMusic();

            DecoderPtr decoder = getDecoder();
            decoder->open(filename);

            mMusic = mOutput->streamSound(decoder, volumeFromType(Play_TypeMusic),
                                          1.0f, Play_NoEnv|Play_TypeMusic);
        }
        catch(std::exception &e)
        {
            std::cout << "Music Error: " << e.what() << "\n";
        }
    }

    void SoundManager::streamMusic(const std::string& filename)
    {
        streamMusicFull("Music/"+filename);
    }

    // FIXME: looping option is not supported, need to implement a workaround
    // activity 0 = explore, 1 = suspence, 2 = battle start, 3 = battle end
    //
    // boundary probably means the ALOC (per cell?) - so % from cell center to edge?
    void SoundManager::streamMediaSet(ESM4::FormId msetId/*, int activity*/)
    {
        if(!mOutput->isInitialized())
            return;

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::ForeignStore<ESM4::MediaSet>& store = world->getStore().getForeign<ESM4::MediaSet>();
        const ESM4::MediaSet *mset = store.search(msetId);
        if (!mset)
            return;

        float hour =  std::fmod(world->getTimeStamp().getHour(), 24);
        float volume = 1.f;
        std::string musicFile = "";
        switch (mset->mSetType)
        {
            case 0: // battle
            {
                // wait time = mset->mTime1
                // loop fade out = mset->mTime2
                // recovery time = mset->mTime3
                //
                // loop battle = mset->mSet2
                //
                // loop db = mset->mLevel8
                //
                // intro sound = mset->mSoundIntro
                // outro sound = mset->mSoundOutro
                std::cout << "MSET battle not yet supported" << std::endl;
                break;
            }
            case 1: // location
            {
                // min time on = mset->mTime1
                // cross fade overlap = mset->mTime2
                // layer cross fade time = mset->mTime3
                //
                // 0x01 day outer,   0x02 day middle,   0x04 day inner
                // 0x08 night outer, 0x10 night middle, 0x20 night inner
                // enabled = mset->mEnabled
                //
                // mset->mBoundaryDayOuter;
                // mset->mBoundaryDayMiddle;
                // mset->mBoundaryDayInner;
                // mset->mBoundaryNightOuter;
                // mset->mBoundaryNightMiddle;
                // mset->mBoundaryNightInner;
                //
                // day outer = mset->mSet2
                // day middle = mset->mSet3
                // day inner = mset->mSet4
                // night outer = mset->mSet5
                // night middle = mset->mSet6
                // night inner = mset->mSet7
                //
                // day outer db = mset->mLevel8
                // day middle db = mset->mLevel9
                // day inner db = mset->mLevel0
                // night outer db = mset->mLevelA
                // night middle db = mset->mLevelB
                // night inner db = mset->mLevelC
                if (hour >= 6 || hour <= 23 ) // default daytime 6:00 - 23:54
                {
                    volume = static_cast<float>(pow(10.0, mset->mLevel8 / 20.f));
                    std::cout << "mset volume " << volume << std::endl; // FIXME
                    if ((mset->mEnabled & 0x01) != 0)
                    {
                        musicFile = "music\\"+mset->mSet2;
                    }
                    //else will throw due to musicFile == ""

                    // FIXME: support boundary distance
                }
                else
                {
                    volume = static_cast<float>(pow(10.0, mset->mLevelA / 20.f));
                    std::cout << "mset volume " << volume << std::endl; // FIXME
                    if ((mset->mEnabled & 0x08) != 0)
                    {
                        musicFile = "music\\"+mset->mSet5;
                    }
                    //else will throw due to musicFile == ""

                    // FIXME: support boundary distance
                }

                break;
            }
            case 2: // dungeon
            {
                // min time on = mset->mTime1
                // cross fade overlap = mset->mTime2
                // layer cross fade time = mset->mTime3
                //
                // battle = mset->mSet2
                // explore = mset->mSet3
                // suspense = mset->mSet4
                //
                // battle db = mset->mLevel8
                // explore db = mset->mLevel9
                // suspense db = mset->mLevel0
                //
                // intro sound = mset->mSoundIntro
                // outro sound = mset->mSoundOutro
                volume = static_cast<float>(pow(10.0, mset->mLevel9 / 20.f));
                std::cout << "mset volume " << volume << std::endl; // FIXME
                musicFile = "music\\"+mset->mSet3;
                std::cout << "MSET dungeon only using explore w/o intro/outro" << std::endl;

                break;
            }
            case 3: // incidental
            {
                // day time min = mset->mTime1
                // night time min = mset->mTime2
                // day time max = mset->mTime3
                // night time max = mset->mTime4
                //
                // daytime = mset->mSoundIntro
                // nighttime = mset->mSoundOutro
                std::cout << "MSET incidental not yet supported" << std::endl;
                break;
            }
            case -1: // none
                return;
            default:
                break;
        }

        if (musicFile == mLastPlayedMusic)
        {
            std::cout << "MSET continuing old music" << std::endl;
            return;
        }

        std::cout << "Playing Media Set " << musicFile << std::endl;
        mLastPlayedMusic = musicFile;
        try
        {
            stopMusic();

            DecoderPtr decoder = getDecoder();
            decoder->open(musicFile);

            // FIXME: not sure why looping is not allowed by OpenAL_Output::streamSound()
            mMusic = mOutput->streamSound(decoder, volume*volumeFromType(Play_TypeMusic),
                                          1.0f, /*Play_LoopNoEnv*/Play_NoEnv|Play_TypeMusic);
        }
        catch(std::exception &e)
        {
            std::cout << "Media Set Music Error: " << e.what() << "\n";
        }
    }

    void SoundManager::startRandomTitle()
    {
        Ogre::StringVector filelist;
        if (mMusicFiles.find(mCurrentPlaylist) == mMusicFiles.end())
        {
            Ogre::StringVector& groups = mUseForeign ? mForeignMusicResourceGroups : mMusicResourceGroups;
            for (Ogre::StringVector::iterator it = groups.begin(); it != groups.end(); ++it)
            {
                Ogre::StringVectorPtr resourcesInThisGroup
                    = mResourceMgr.findResourceNames(*it, "Music/"+mCurrentPlaylist+"/*");
                filelist.insert(filelist.end(), resourcesInThisGroup->begin(), resourcesInThisGroup->end());
            }
            mMusicFiles[mCurrentPlaylist] = filelist;
        }
        else
            filelist = mMusicFiles[mCurrentPlaylist];

        if(!filelist.size())
            return;

        int i = Misc::Rng::rollDice(int(filelist.size()));

        // Don't play the same music track twice in a row
        if (filelist[i] == mLastPlayedMusic)
        {
            i = (i+1) % filelist.size();
        }

        streamMusicFull(filelist[i]);
    }

    bool SoundManager::isMusicPlaying()
    {
        return mMusic && mMusic->isPlaying();
    }

    void SoundManager::playPlaylist(const std::string &playlist, bool foreign)
    {
        if (foreign != mUseForeign) // flush old
            mMusicFiles.clear();

        mUseForeign = foreign;

        mCurrentPlaylist = playlist;
        startRandomTitle();
    }

    void SoundManager::say(const MWWorld::Ptr &ptr, const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        try
        {
            float basevol = volumeFromType(Play_TypeVoice);
            std::string filePath = "Sound\\"+filename;
            const ESM::Position &pos = ptr.getRefData().getPosition();
            const Ogre::Vector3 objpos(pos.pos);

            MWBase::World* world = MWBase::Environment::get().getWorld();
            static const float fAudioMinDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMinDistanceMult")->getFloat();
            static const float fAudioMaxDistanceMult = world->getStore().get<ESM::GameSetting>().find("fAudioMaxDistanceMult")->getFloat();
            static const float fAudioVoiceDefaultMinDistance = world->getStore().get<ESM::GameSetting>().find("fAudioVoiceDefaultMinDistance")->getFloat();
            static const float fAudioVoiceDefaultMaxDistance = world->getStore().get<ESM::GameSetting>().find("fAudioVoiceDefaultMaxDistance")->getFloat();

            float minDistance = fAudioVoiceDefaultMinDistance * fAudioMinDistanceMult;
            float maxDistance = fAudioVoiceDefaultMaxDistance * fAudioMaxDistanceMult;
            minDistance = std::max(minDistance, 1.f);
            maxDistance = std::max(minDistance, maxDistance);

            MWBase::SoundPtr sound = mOutput->playSound3D(filePath, objpos, 1.0f, basevol, 1.0f,
                                                          minDistance, maxDistance, Play_Normal|Play_TypeVoice, 0, true);
            mActiveSounds[sound] = std::make_pair(ptr, std::string("_say_sound"));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    float SoundManager::getSaySoundLoudness(const MWWorld::Ptr &ptr) const
    {
        SoundMap::const_iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == "_say_sound")
                break;
            ++snditer;
        }
        if (snditer == mActiveSounds.end())
            return 0.f;

        return snditer->first->getCurrentLoudness();
    }

    void SoundManager::say(const std::string& filename)
    {
        if(!mOutput->isInitialized())
            return;
        try
        {
            float basevol = volumeFromType(Play_TypeVoice);
            std::string filePath = "Sound/"+filename;

            MWBase::SoundPtr sound = mOutput->playSound(filePath, 1.0f, basevol, 1.0f, Play_Normal|Play_TypeVoice, 0);
            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), std::string("_say_sound"));
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
    }

    bool SoundManager::sayDone(const MWWorld::Ptr &ptr) const
    {
        return !isPlaying(ptr, "_say_sound");
    }

    void SoundManager::stopSay(const MWWorld::Ptr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == "_say_sound")
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }


    MWBase::SoundPtr SoundManager::playTrack(const DecoderPtr& decoder, PlayType type)
    {
        MWBase::SoundPtr track;
        if(!mOutput->isInitialized())
            return track;
        try
        {
            track = mOutput->streamSound(decoder, volumeFromType(type), 1.0f, Play_NoEnv|type);
        }
        catch(std::exception &e)
        {
            std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return track;
    }


    MWBase::SoundPtr SoundManager::playSound(const std::string& soundId, float volume, float pitch, PlayType type, PlayMode mode, float offset)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            float basevol = volumeFromType(type);
            float min, max;
            std::string file = ESM4::isFormId(soundId) ?  lookupForeign(soundId, volume, min, max) :
                                                          lookup(soundId, volume, min, max);

            sound = mOutput->playSound(file, volume, basevol, pitch, mode|type, offset);
            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
        }
        catch(std::exception&)
        {
            //std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    MWBase::SoundPtr SoundManager::playSound3D(const MWWorld::Ptr &ptr, const std::string& soundId,
                                               float volume, float pitch, PlayType type, PlayMode mode, float offset)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            // Look up the sound in the ESM data
            float basevol = volumeFromType(type);
            float min, max;
            std::string file = ESM4::isFormId(soundId) ?  lookupForeign(soundId, volume, min, max) :
                                                          lookup(soundId, volume, min, max);
            const ESM::Position &pos = ptr.getRefData().getPosition();
            const Ogre::Vector3 objpos(pos.pos);

            if ((mode & Play_RemoveAtDistance) && mListenerPos.squaredDistance(objpos) > 2000*2000)
            {
                return MWBase::SoundPtr();
            }

            sound = mOutput->playSound3D(file, objpos, volume, basevol, pitch, min, max, mode|type, offset);
            if((mode&Play_NoTrack))
                mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
            else
                mActiveSounds[sound] = std::make_pair(ptr, soundId);
        }
        catch(std::exception&)
        {
            //std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    MWBase::SoundPtr SoundManager::playManualSound3D(const Ogre::Vector3& initialPos, const std::string& soundId,
                                                     float volume, float pitch, PlayType type, PlayMode mode, float offset)
    {
        MWBase::SoundPtr sound;
        if(!mOutput->isInitialized())
            return sound;
        try
        {
            // Look up the sound in the ESM data
            float basevol = volumeFromType(type);
            float min, max;
            std::string file = lookup(soundId, volume, min, max);

            sound = mOutput->playSound3D(file, initialPos, volume, basevol, pitch, min, max, mode|type, offset);
            mActiveSounds[sound] = std::make_pair(MWWorld::Ptr(), soundId);
        }
        catch(std::exception &)
        {
            //std::cout <<"Sound Error: "<<e.what()<< std::endl;
        }
        return sound;
    }

    void SoundManager::stopSound (MWBase::SoundPtr sound)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->first == sound)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound3D(const MWWorld::Ptr &ptr, const std::string& soundId)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == soundId)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound3D(const MWWorld::Ptr &ptr)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound(const MWWorld::CellStore *cell)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first != MWWorld::Ptr() &&
               snditer->second.first != MWBase::Environment::get().getWorld()->getPlayerPtr() &&
               snditer->second.first.getCell() == cell)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::stopSound(const std::string& soundId)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == MWWorld::Ptr() &&
               snditer->second.second == soundId)
            {
                snditer->first->stop();
                mActiveSounds.erase(snditer++);
            }
            else
                ++snditer;
        }
    }

    void SoundManager::fadeOutSound3D(const MWWorld::Ptr &ptr,
            const std::string& soundId, float duration)
    {
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(snditer->second.first == ptr && snditer->second.second == soundId)
            {
                snditer->first->setFadeout(duration);
            }
            ++snditer;
        }
    }

    bool SoundManager::getSoundPlaying(const MWWorld::Ptr &ptr, const std::string& soundId) const
    {
        return isPlaying(ptr, soundId);
    }


    void SoundManager::pauseSounds(int types)
    {
        if(mOutput->isInitialized())
        {
            types &= Play_TypeMask;
            mOutput->pauseSounds(types);
            mPausedSoundTypes |= types;
        }
    }

    void SoundManager::resumeSounds(int types)
    {
        if(mOutput->isInitialized())
        {
            types &= types&Play_TypeMask&mPausedSoundTypes;
            mOutput->resumeSounds(types);
            mPausedSoundTypes &= ~types;
        }
    }


    void SoundManager::updateRegionSound(float duration)
    {
        static float sTimeToNextEnvSound = 0.0f;
        static int total = 0;
        static std::string regionName = "";
        static float sTimePassed = 0.0;
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Ptr player = world->getPlayerPtr();
        const ESM::Cell *cell = player.getCell()->getCell();

        sTimePassed += duration;
        if(!cell->isExterior() || sTimePassed < sTimeToNextEnvSound)
            return;

        float a = Misc::Rng::rollClosedProbability();
        // NOTE: We should use the "Minimum Time Between Environmental Sounds" and
        // "Maximum Time Between Environmental Sounds" fallback settings here.
        sTimeToNextEnvSound = 5.0f*a + 15.0f*(1.0f-a);
        sTimePassed = 0;

        if(regionName != cell->mRegion)
        {
            regionName = cell->mRegion;
            total = 0;
        }

        const ESM::Region *regn = world->getStore().get<ESM::Region>().search(regionName);
        if(regn == NULL)
            return;

        std::vector<ESM::Region::SoundRef>::const_iterator soundIter;
        if(total == 0)
        {
            soundIter = regn->mSoundList.begin();
            while(soundIter != regn->mSoundList.end())
            {
                total += (int)soundIter->mChance;
                ++soundIter;
            }
            if(total == 0)
                return;
        }

        int r = Misc::Rng::rollDice(total);
        int pos = 0;

        soundIter = regn->mSoundList.begin();
        while(soundIter != regn->mSoundList.end())
        {
            if(r - pos < soundIter->mChance)
            {
                playSound(soundIter->mSound.toString(), 1.0f, 1.0f);
                break;
            }
            pos += soundIter->mChance;

            ++soundIter;
        }
    }

    void SoundManager::updateSounds(float duration)
    {
        static float timePassed = 0.0;

        timePassed += duration;
        if(timePassed < (1.0f/30.0f))
            return;
        duration = timePassed;
        timePassed = 0.0f;

        // Make sure music is still playing
        if(!isMusicPlaying())
            startRandomTitle();

        Environment env = Env_Normal;
        if (mListenerUnderwater)
        {
            env = Env_Underwater;
            //play underwater sound
            if(!(mUnderwaterSound && mUnderwaterSound->isPlaying()))
                mUnderwaterSound = playSound("Underwater", 1.0f, 1.0f, Play_TypeSfx, Play_LoopNoEnv);
        }
        else if(mUnderwaterSound)
        {
            mUnderwaterSound->stop();
            mUnderwaterSound.reset();
        }

        mOutput->updateListener(
            mListenerPos,
            mListenerDir,
            mListenerUp,
            env
        );

        // Check if any sounds are finished playing, and trash them
        // Lower volume on fading out sounds
        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            if(!snditer->first->isPlaying())
                mActiveSounds.erase(snditer++);
            else
            {
                const MWWorld::Ptr &ptr = snditer->second.first;
                if(!ptr.isEmpty())
                {
                    const ESM::Position &pos = ptr.getRefData().getPosition();
                    const Ogre::Vector3 objpos(pos.pos);
                    snditer->first->setPosition(objpos);

                    if ((snditer->first->mFlags & Play_RemoveAtDistance)
                            && mListenerPos.squaredDistance(Ogre::Vector3(ptr.getRefData().getPosition().pos)) > 2000*2000)
                    {
                        mActiveSounds.erase(snditer++);
                        continue;
                    }
                }
                //update fade out
                if(snditer->first->mFadeOutTime>0)
                {
                    float soundDuration=duration;
                    if(soundDuration>snditer->first->mFadeOutTime)
                        soundDuration=snditer->first->mFadeOutTime;
                    snditer->first->setVolume(snditer->first->mVolume
                                    - soundDuration / snditer->first->mFadeOutTime * snditer->first->mVolume);
                    snditer->first->mFadeOutTime -= soundDuration;
                }
                snditer->first->update();
                ++snditer;
            }
        }
    }

    void SoundManager::update(float duration)
    {
        if(!mOutput->isInitialized())
            return;

        if (MWBase::Environment::get().getStateManager()->getState()!=
            MWBase::StateManager::State_NoGame)
        {
            updateSounds(duration);
            updateRegionSound(duration);
        }
    }


    void SoundManager::processChangedSettings(const Settings::CategorySettingVector& settings)
    {
        mMasterVolume = Settings::Manager::getFloat("master volume", "Sound");
        mMusicVolume = Settings::Manager::getFloat("music volume", "Sound");
        mSFXVolume = Settings::Manager::getFloat("sfx volume", "Sound");
        mFootstepsVolume = Settings::Manager::getFloat("footsteps volume", "Sound");
        mVoiceVolume = Settings::Manager::getFloat("voice volume", "Sound");

        SoundMap::iterator snditer = mActiveSounds.begin();
        while(snditer != mActiveSounds.end())
        {
            snditer->first->mBaseVolume = volumeFromType(snditer->first->getPlayType());
            snditer->first->update();
            ++snditer;
        }
        if(mMusic)
        {
            mMusic->mBaseVolume = volumeFromType(mMusic->getPlayType());
            mMusic->update();
        }
    }

    void SoundManager::setListenerPosDir(const Ogre::Vector3 &pos, const Ogre::Vector3 &dir, const Ogre::Vector3 &up)
    {
        mListenerPos = pos;
        mListenerDir = dir;
        mListenerUp  = up;

        MWWorld::Ptr player =
            MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWWorld::CellStore *cell = player.getCell();

        mListenerUnderwater = ((cell->getCell()->mData.mFlags&ESM::Cell::HasWater) && mListenerPos.z < cell->getWaterLevel());
    }

    void SoundManager::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        for (SoundMap::iterator snditer = mActiveSounds.begin(); snditer != mActiveSounds.end(); ++snditer)
        {
            if (snditer->second.first == old)
                snditer->second.first = updated;
        }
    }

    // Default readAll implementation, for decoders that can't do anything
    // better
    void Sound_Decoder::readAll(std::vector<char> &output)
    {
        size_t total = output.size();
        size_t got;

        output.resize(total+32768);
        while((got=read(&output[total], output.size()-total)) > 0)
        {
            total += got;
            output.resize(total*2);
        }
        output.resize(total);
    }


    const char *getSampleTypeName(SampleType type)
    {
        switch(type)
        {
            case SampleType_UInt8: return "U8";
            case SampleType_Int16: return "S16";
            case SampleType_Float32: return "Float32";
        }
        return "(unknown sample type)";
    }

    const char *getChannelConfigName(ChannelConfig config)
    {
        switch(config)
        {
            case ChannelConfig_Mono:    return "Mono";
            case ChannelConfig_Stereo:  return "Stereo";
            case ChannelConfig_Quad:    return "Quad";
            case ChannelConfig_5point1: return "5.1 Surround";
            case ChannelConfig_7point1: return "7.1 Surround";
        }
        return "(unknown channel config)";
    }

    size_t framesToBytes(size_t frames, ChannelConfig config, SampleType type)
    {
        switch(config)
        {
            case ChannelConfig_Mono:    frames *= 1; break;
            case ChannelConfig_Stereo:  frames *= 2; break;
            case ChannelConfig_Quad:    frames *= 4; break;
            case ChannelConfig_5point1: frames *= 6; break;
            case ChannelConfig_7point1: frames *= 8; break;
        }
        switch(type)
        {
            case SampleType_UInt8: frames *= 1; break;
            case SampleType_Int16: frames *= 2; break;
            case SampleType_Float32: frames *= 4; break;
        }
        return frames;
    }

    size_t bytesToFrames(size_t bytes, ChannelConfig config, SampleType type)
    {
        return bytes / framesToBytes(1, config, type);
    }

    void SoundManager::clear()
    {
        for (SoundMap::iterator iter (mActiveSounds.begin()); iter!=mActiveSounds.end(); ++iter)
            iter->first->stop();

        mActiveSounds.clear();
        stopMusic();
    }
}
