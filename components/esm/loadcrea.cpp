#include "loadcrea.hpp"

namespace ESM {

void Creature::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    original = esm.getHNOString("CNAM");
    name = esm.getHNOString("FNAM");
    script = esm.getHNOString("SCRI");

    esm.getHNT(data, "NPDT", 96);

    esm.getHNT(flags, "FLAG");
    scale = 1.0;
    esm.getHNOT(scale, "XSCL");

    inventory.load(esm);
    spells.load(esm);
    aiData.load(esm);

    if (esm.isNextSub("AIDT"))
    {
        esm.getHExact(&AI, sizeof(AI));
        hasAI = true;
    }
    else
        hasAI = false;

    // More subrecords:
    // AI_W - wander (14 bytes, i don't understand it)
    //    short distance
    //    byte duration
    //    byte timeOfDay
    //    byte idle[10]
    //
    // Rest is optional:
    // AI_T - travel?
    // AI_F - follow?
    // AI_E - escort?
    // AI_A - activate?
    esm.skipRecord();

}

void Creature::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNOCString("CNAM", original);
    esm.writeHNOCString("FNAM", name);
    esm.writeHNOCString("SCRI", script);
    esm.writeHNT("NPDT", data, 96);
    esm.writeHNT("FLAG", flags);
    if (scale != 1.0)
        esm.writeHNT("XSCL", scale);

    inventory.save(esm);
    spells.save(esm);
    aiData.save(esm);
}

}
