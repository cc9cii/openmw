///Functions used to read raw binary data from .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <stdint.h>
#include <stdexcept>
#include <typeinfo>
#include <string>

#include <OgreAny.h>
#include <OgreDataStream.h>
#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>
#include <OgreQuaternion.h>
#include <OgreStringConverter.h>

#include <btBulletDynamicsCommon.h>

#include "niffile.hpp"
#include "niftypes.hpp"

namespace Nif
{

class NIFStream {

    /// Input stream
    Ogre::DataStreamPtr inp;

    uint8_t read_byte();
    uint16_t read_le16();
    uint32_t read_le32();
    float read_le32f();

public:

    NIFFile * const file;

    NIFStream (NIFFile * file, Ogre::DataStreamPtr inp): inp (inp), file (file) {}

    void skip(size_t size) { inp->skip((long)size); } // WARNING: possible loss of data for very large files

    size_t tell() { return inp->tell(); } // FIXME: debugging only
    size_t size() { return inp->size(); } // FIXME: debugging only
    void close()  { inp->close(); }       // FIXME: debugging only

    char getChar() { return read_byte(); }
    short getShort() { return read_le16(); }
    unsigned short getUShort() { return read_le16(); }
    int getInt() { return read_le32(); }
    unsigned int getUInt() { return read_le32(); }
    float getFloat() { return read_le32f(); }
    bool getBool(unsigned int nifVer);

    Ogre::Vector2 getVector2();
    Ogre::Vector3 getVector3();
    Ogre::Vector4 getVector4();
    Ogre::Matrix3 getMatrix3();
    Ogre::Quaternion getQuaternion();
    Transformation getTrafo();

    btVector3 getBtVector3();
    btVector4 getBtVector4();
    btQuaternion getBtQuaternion();

    // NOTE: the caller must ensure that the buffer of sufficient size is allocated
    void getBuffer(size_t length, char* buf);

    ///Read in a string of the given length
    std::string getString(size_t length);
    ///Read in a string of the length specified in the file
    std::string getString();
    ///This is special since the version string doesn't start with a number, and ends with "\n"
    std::string getVersionString();
    ///This is a strange type used by newer nif formats
    std::string getShortString(unsigned int ver = 0x04000002);

    std::string getSkyrimString(unsigned int nifVer = 0x04000002, std::vector<std::string> *strings = 0);

    void getShorts(std::vector<short> &vec, size_t size);
    void getFloats(std::vector<float> &vec, size_t size);
    void getVector2s(std::vector<Ogre::Vector2> &vec, size_t size);
    void getVector3s(std::vector<Ogre::Vector3> &vec, size_t size);
    void getVector4s(std::vector<Ogre::Vector4> &vec, size_t size);
    void getQuaternions(std::vector<Ogre::Quaternion> &quat, size_t size);

    template <typename T>
    T getIfVer(unsigned int testVersion)
    {
        if (file->getVersion() == testVersion)
            return Ogre::Any::get<T>(); // FIXME: just a guess
        else
            return T();
    }

};

}

#endif
