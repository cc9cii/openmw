/*
  Copyright (C) 2018, 2020 cc9cii

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
#ifndef NIBTOGRE_OGREMATERIAL_H
#define NIBTOGRE_OGREMATERIAL_H

#include <map>

#include <OgreVector3.h>

#include "niproperty.hpp" // TextureType

namespace NiBtOgre
{
    struct OgreMaterial
    {
        Ogre::Vector3 ambient;
        Ogre::Vector3 diffuse;
        Ogre::Vector3 specular;
        Ogre::Vector3 emissive;
        float glossiness;
        float alpha;
        int alphaFlags;
        int alphaTest;
        int vertMode;
        int lightMode;
        int depthFlags;
        int specFlags; // Default should be 1, but Bloodmoon's models are broken
        int wireFlags;
        int drawMode;
        bool vertexColor;
        BSLightingShaderProperty *bsLightingShaderProperty;
        //bool isBSLightingShader;
        //std::uint32_t shaderType;
        //std::uint32_t shaderFlags1;
        //std::uint32_t shaderFlags2;

        std::map<NiTexturingProperty::TextureType, std::string> texName;
        std::map<NiTexturingProperty::TextureType, NiTexturingProperty::TexDesc> *textureDescriptions;

        std::vector<std::string> bsTextureSet;


        OgreMaterial() : ambient(1.f), diffuse(1.f), specular(0.f), emissive(0.f), glossiness(0.f), alpha(1.f)
                       , alphaFlags(0), alphaTest(0), vertMode(2), lightMode(1), depthFlags(3)
                       , specFlags(0), wireFlags(0), drawMode(1), vertexColor(false), mNeedTangents(false)
                       , bsLightingShaderProperty(nullptr)//, shaderType(0), shaderFlags1(0), shaderFlags2(0)
        {
        }

        void setExternalTexture(const std::string& texture);

        std::string getOrCreateMaterial(const std::string& name);
        std::string getOrCreateLSMaterial(const std::string& name);

        bool needTangents() const { return mNeedTangents; }

    private:
        bool mNeedTangents;
        static std::map<size_t,std::string> sMaterialMap;
    };
}

#endif // NIBTOGRE_OGREMATERIAL_H
