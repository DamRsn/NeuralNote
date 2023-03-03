#pragma once

#include "CommonHeaders.h"

//Helper classes to automatically fetch all binary data assets

//Represents a binary data object:
struct RawData
{
    explicit RawData(int index)
    {
        using namespace BinaryData;
        data = getNamedResource(namedResourceList[index], size);
    }

    const char* data;
    int size;
};

//Returns a vector of all existing binary data assets:
inline std::vector<RawData> getBinaryDataAssets()
{
    std::vector<RawData> assets;

    using namespace BinaryData;

    for (int index = 0; index < namedResourceListSize; ++index)
        assets.emplace_back(index);

    return assets;
}

//returns all binary data images as a vector:
inline std::vector<Image> getBinaryDataImages()
{
    std::vector<Image> images;

    for (auto& asset: getBinaryDataAssets())
    {
        auto image = ImageCache::getFromMemory(asset.data, asset.size);

        if (image.isValid())
            images.emplace_back(image);
    }

    return images;
}