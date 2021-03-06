OpenMW
======

[![Build Status](https://img.shields.io/travis/OpenMW/openmw.svg)](https://travis-ci.org/OpenMW/openmw) [![Coverity Scan Build Status](https://scan.coverity.com/projects/3740/badge.svg)](https://scan.coverity.com/projects/3740)

OpenMW is an attempt at recreating the engine for the popular role-playing game
Morrowind by Bethesda Softworks. You need to own and install the original game for OpenMW to work.

* Version: 0.36.0
* License: GPL (see docs/license/GPL3.txt for more information)
* Website: http://www.openmw.org
* IRC: #openmw on irc.freenode.net

Font Licenses:
* DejaVuLGCSansMono.ttf: custom (see docs/license/DejaVu Font License.txt for more information)

Wrong Way, Go Back
------------------

This is a fork of an old version of OpenMW.  This version is probably not what you are looking for.  It is still stuck on Ogre 1.10 and hence does not have any of the recent graphics enhancements.  I suggest you use the [official release](https://github.com/OpenMW/openmw) instead.

Getting Started
---------------

* [Official forums](https://forum.openmw.org/)
* [Installation instructions](https://wiki.openmw.org/index.php?title=Installation_Instructions)
* [Build from source](https://wiki.openmw.org/index.php?title=Development_Environment_Setup)
* [Testing the game](https://wiki.openmw.org/index.php?title=Testing)
* [How to contribute](https://wiki.openmw.org/index.php?title=Contribution_Wanted)
* [Report a bug](http://bugs.openmw.org/projects/openmw) - read the [guidelines](https://wiki.openmw.org/index.php?title=Bug_Reporting_Guidelines) before submitting your first bug!
* [Known issues](http://bugs.openmw.org/projects/openmw/issues?utf8=%E2%9C%93&set_filter=1&f%5B%5D=status_id&op%5Bstatus_id%5D=%3D&v%5Bstatus_id%5D%5B%5D=7&f%5B%5D=tracker_id&op%5Btracker_id%5D=%3D&v%5Btracker_id%5D%5B%5D=1&f%5B%5D=&c%5B%5D=project&c%5B%5D=tracker&c%5B%5D=status&c%5B%5D=priority&c%5B%5D=subject&c%5B%5D=assigned_to&c%5B%5D=updated_on&group_by=tracker)

The data path
-------------

The data path tells OpenMW where to find your Morrowind files. If you run the launcher, OpenMW should be able to pick up the location of these files on its own, if both Morrowind and OpenMW are installed properly (installing Morrowind under WINE is considered a proper install).

Command line options
--------------------

    Syntax: openmw <options>
    Allowed options:
      --help                                print help message
      --version                             print version information and quit
      --data arg (=data)                    set data directories (later directories
                                            have higher priority)
      --data-local arg                      set local data directory (highest
                                            priority)
      --fallback-archive arg (=fallback-archive)
                                            set fallback BSA archives (later
                                            archives have higher priority)
      --resources arg (=resources)          set resources directory
      --start arg                           set initial cell
      --content arg                         content file(s): esm/esp, or
                                            omwgame/omwaddon
      --no-sound [=arg(=1)] (=0)            disable all sounds
      --script-verbose [=arg(=1)] (=0)      verbose script output
      --script-all [=arg(=1)] (=0)          compile all scripts (excluding dialogue
                                            scripts) at startup
      --script-all-dialogue [=arg(=1)] (=0) compile all dialogue scripts at startup
      --script-console [=arg(=1)] (=0)      enable console-only script
                                            functionality
      --script-run arg                      select a file containing a list of
                                            console commands that is executed on
                                            startup
      --script-warn [=arg(=1)] (=1)         handling of warnings when compiling
                                            scripts
                                            0 - ignore warning
                                            1 - show warning but consider script as
                                            correctly compiled anyway
                                            2 - treat warnings as errors
      --script-blacklist arg                ignore the specified script (if the use
                                            of the blacklist is enabled)
      --script-blacklist-use [=arg(=1)] (=1)
                                            enable script blacklisting
      --load-savegame arg                   load a save game file on game startup
                                            (specify an absolute filename or a
                                            filename relative to the current
                                            working directory)
      --skip-menu [=arg(=1)] (=0)           skip main menu on game startup
      --new-game [=arg(=1)] (=0)            run new game sequence (ignored if
                                            skip-menu=0)
      --fs-strict [=arg(=1)] (=0)           strict file system handling (no case
                                            folding)
      --encoding arg (=win1252)             Character encoding used in OpenMW game
                                            messages:

                                            win1250 - Central and Eastern European
                                            such as Polish, Czech, Slovak,
                                            Hungarian, Slovene, Bosnian, Croatian,
                                            Serbian (Latin script), Romanian and
                                            Albanian languages

                                            win1251 - Cyrillic alphabet such as
                                            Russian, Bulgarian, Serbian Cyrillic
                                            and other languages

                                            win1252 - Western European (Latin)
                                            alphabet, used by default
      --fallback arg                        fallback values
      --no-grab                             Don't grab mouse cursor
      --export-fonts [=arg(=1)] (=0)        Export Morrowind .fnt fonts to PNG
                                            image and XML file in current directory
      --activate-dist arg (=-1)             activation distance override

Changes
-------

Some of the differences with the official release are listed below.  Initial changes are mostly to do with OpenCS:

* Various minor bug fixes.
* C++11 features are used (or at least those available on MSVC 2013 onwards).
* Loading time improvements.
* Loading progress bar changes.
* Pathgrid points supported.
* 3D editing retained, but does not have OSG enhancements or mouse modes.
* Modifying an object in the cell view should trigger the instances table to scroll to the corresponding record.
* Initial support for Land and Land Texture records.
* NPC stats autocalculation.
* Per-subview shortcuts for "added" and "modified" filters.
* Global filters + warning splash screen.
* User preferences setting to save the window state (position, geometry, etc) at the time of exit.
* User preferences setting to workaround some X window managers not keeping pre-maximised state.
* Use opencs.ini to store window states between sessions.

Enhancements for both OpenMW and OpenCS:

* Hash based lookup for TES3 BSA files.
* TES4/TES5 BSA support.
* Loading TES4/TES5 records (incomplete).
* Experimental support of using multiple versions of ESM files concurrently in OpenMW (coming soon)
* Experimental support of loading newer NIF records (coming soon).
* Experimental support of NavMesh (eventually).

openmw.cfg example
------------------

      ...
      fallback-archive=Morrowind.bsa
      fallback-archive=Tribunal.bsa
      fallback-archive=Bloodmoon.bsa
      fallback-archive=TR_Data.bsa
      fallback-tes4archive=Oblivion - Meshes.bsa
      #fallback-tes4archive=Skyrim - Textures.bsa
      #fallback-tes4archive=Dragonborn.bsa
      #fallback-tes4archive=Dawnguard.bsa
      ...
      data="C:/Program Files (x86)/Bethesda Softworks/Morrowind/Data Files"
      data="C:/Program Files (x86)/Bethesda Softworks/Oblivion/Data"
      ...

Build Dependencies
------------------

The development was done using MSVC 2013 then MSVC Community 2015, Version 14.0.25431.01 Update 3 then finally MS Visual Studio Community 2017, Version 15.9.4.  The code may or may not compile under linux/gcc (probably not).

* boost-1_70
* Ogre 1.10.11
* ffmpeg-4.1
* MyGUI-3.2.2
* SDL-2.0.9
* libpng-1.6.36
* zlib 1.2.11
* Qt5

