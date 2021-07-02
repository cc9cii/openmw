#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esm4reader.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mEsm(readers)
  , mStore(store)
  , mEncoder(encoder)
{
    // NOTE: mEsm size is set in World ctor
    // the size indicates how many different types of games can be loaded concurrently
    mLoadedFiles.resize(mEsm.size());
}

// FIXME: tesVerIndex stuff is rather clunky, needs to be refactored
void EsmLoader::load(const boost::filesystem::path& filepath, int& index)
{
    int tesVerIndex = 0; // FIXME: hard coded, 0 = MW, 1 = TES4, 2 = TES5 3 = Fallout

    ContentLoader::load(filepath.filename(), index);

    ESM::ESMReader *lEsm = new ESM::ESMReader();
    lEsm->setEncoder(mEncoder);
    lEsm->setGlobalReaderList(&mEsm[tesVerIndex]);  // seems to be used only by ESMStore::load()
    lEsm->open(filepath.string());

    int esmVer = lEsm->getVer();
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_17;
    bool isFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;

    if (isTes4 || isTes5 || isFONV)
    {
        if (isTes4)
            tesVerIndex = 1;
        else if (isTes5)
            tesVerIndex = 2;
        else
            tesVerIndex = 3;

        lEsm->close();
        delete lEsm;
        ESM::ESM4Reader *esm = new ESM::ESM4Reader(isTes4); // NOTE: TES4 headers are 4 bytes shorter
        esm->setEncoder(mEncoder);

        int tes4Index = mLoadedFiles[tesVerIndex].size();
        mLoadedFiles[tesVerIndex].push_back(filepath.filename().string());
        esm->setIndex(tes4Index);

        // HACK: trick index into ignoring non TES3 content files
        index -= 1;

        esm->reader().setModIndex(tes4Index);
        esm->openTes4File(filepath.string());
        esm->reader().updateModIndices(mLoadedFiles[tesVerIndex]);
        // FIXME: this does not work well (copies the base class pointer)
        //i.e. have to check TES4/TES5 versions each time before use within EsmStore::load,
        //static casting as required
        mEsm[tesVerIndex].push_back(esm);

        mStore.load(*mEsm[tesVerIndex][tes4Index], &mListener);
    }
    else
    {
        tesVerIndex = 0; // 0 = MW
        assert (index == mLoadedFiles[tesVerIndex].size());

        mLoadedFiles[tesVerIndex].push_back(filepath.filename().string());
        lEsm->setIndex(index);
        mEsm[tesVerIndex].push_back(lEsm);

        mStore.load(*mEsm[tesVerIndex][index], &mListener);
    }
}

} /* namespace MWWorld */
