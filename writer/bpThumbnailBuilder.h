/***************************************************************************
 *   Copyright (c) 2020-present Bitplane AG Zuerich                        *
 *                                                                         *
 *   Licensed under the Apache License, Version 2.0 (the "License");       *
 *   you may not use this file except in compliance with the License.      *
 *   You may obtain a copy of the License at                               *
 *                                                                         *
 *       http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                         *
 *   Unless required by applicable law or agreed to in writing, software   *
 *   distributed under the License is distributed on an "AS IS" BASIS,     *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imp   *
 *   See the License for the specific language governing permissions and   *
 *   limitations under the License.                                        *
 ***************************************************************************/
#ifndef __BP_THUMBNAIL_BUILDER__
#define __BP_THUMBNAIL_BUILDER__


#include "bpThumbnail.h"
#include "bpMemoryBlock.h"


template<typename TDataType>
class bpColorCacheWithTable
{
public:
  explicit bpColorCacheWithTable(const bpConverterTypes::cColorInfo& aColorInfo)
    : mTable(1ull << (sizeof(TDataType) * 8))
  {
    static_assert((1ull << (sizeof(TDataType) * 8)) < 66000, "bpColorCacheWithTable: I am gonna blow up your memory");
    bpSize vTableSize = mTable.size();
    for (bpSize vIndex = 0; vIndex < vTableSize; ++vIndex) {
      mTable[vIndex] = aColorInfo.GetColor(vIndex);
    }
  }

  const bpConverterTypes::cColor& GetColor(const TDataType& aValue) const
  {
    return mTable[aValue];
  }

private:
  std::vector<bpConverterTypes::cColor> mTable;
};

template<typename TDataType>
struct bpColorCache
{
  using tType = const bpConverterTypes::cColorInfo&;
};

template<>
struct bpColorCache<bpUInt8>
{
  using tType = bpColorCacheWithTable<bpUInt8>;
};

template<>
struct bpColorCache<bpUInt16>
{
  using tType = bpColorCacheWithTable<bpUInt16>;
};

bpSize bpComputeThumbnailIndexR(bpSize aThumbnailSizeXY, const std::vector<bpVec3>& aImageResolutionSizes);

std::pair<bpSize, bpSize> bpComputeThumbnailSizeXY(bpSize aThumbnailSizeXY, const bpVec3& aImageSize, const bpFloatVec3& aImageExtents);

bpFloat bpComputeThumbnailQuality(const bpThumbnail& aThumbnail);

template<typename TDataType>
class bpThumbnailBuilder
{
public:
  bpThumbnailBuilder(bpSize aThumbnailSizeXY, const std::vector<bpVec3>& aImageResolutionSizes, const std::vector<bpVec3>& aResolutionBlockSizes, bpSize aSizeC)
    : mThumbnailSizeXY(aThumbnailSizeXY),
      mIndexR(bpComputeThumbnailIndexR(aThumbnailSizeXY, aImageResolutionSizes))
  {
    mImageSize = aImageResolutionSizes[mIndexR];
    mBlockSize = aResolutionBlockSizes[mIndexR];
    mNBlocks[0] = DivEx(mImageSize[0], mBlockSize[0]);
    mNBlocks[1] = DivEx(mImageSize[1], mBlockSize[1]);
    mNBlocks[2] = DivEx(mImageSize[2], mBlockSize[2]);

    bpSize vNumberOfBlocks = mNBlocks[0] * mNBlocks[1] * mNBlocks[2];
    mBlocksPerChannel.resize(aSizeC, std::vector<bpConstMemoryBlock<TDataType>>(vNumberOfBlocks));
  }

  void StartCopyDataBlock(const bpConstMemoryBlock<TDataType>& aData,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
  {
    if (aIndexT > 0 || aIndexR != mIndexR) {
      return;
    }

    mBlocksPerChannel[aIndexC][aBlockIndexX + mNBlocks[0] * (aBlockIndexY + mNBlocks[1] * aBlockIndexZ)] = aData;
  }

  bpThumbnail CreateThumbnail(const std::vector<bpConverterTypes::cColorInfo>& aChannelColors, const bpConverterTypes::cImageExtent& aImageExtent) const
  {
    std::vector<bpConverterTypes::cColorInfo> vChannelColors(aChannelColors);
    if (vChannelColors.size() > mBlocksPerChannel.size()) {
      vChannelColors.resize(mBlocksPerChannel.size());
    }

    cResampler vResampler(mThumbnailSizeXY, mImageSize, aChannelColors, aImageExtent);

    for (bpSize vBlockIndexZ = 0; vBlockIndexZ < mNBlocks[2]; vBlockIndexZ++) {
      for (bpSize vBlockIndexY = 0; vBlockIndexY < mNBlocks[1]; vBlockIndexY++) {
        for (bpSize vBlockIndexX = 0; vBlockIndexX < mNBlocks[0]; vBlockIndexX++) {
          for (bpSize vIndexC = 0; vIndexC < vChannelColors.size(); vIndexC++) {
            ResampleCopyDataBlock(vResampler, vIndexC, vBlockIndexX, vBlockIndexY, vBlockIndexZ);
          }
        }
      }
    }

    bpThumbnail vMIP = vResampler.GetMIP();
    bpThumbnail vMiddle = vResampler.GetMiddle();
    bpFloat vQualityMIP = bpComputeThumbnailQuality(vMIP);
    bpFloat vQualityMiddle = bpComputeThumbnailQuality(vMiddle);
    return vQualityMIP > vQualityMiddle ? vMIP : vMiddle;
  }

private:
  class cResampler;

  static inline bpSize DivEx(bpSize aNum, bpSize aDiv)
  {
    return (aNum + aDiv - 1) / aDiv;
  }

  void ResampleCopyDataBlock(cResampler& aResampler, bpSize aIndexC, bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) const
  {
    const auto& vData = mBlocksPerChannel[aIndexC][aBlockIndexX + mNBlocks[0] * (aBlockIndexY + mNBlocks[1] * aBlockIndexZ)];
    if (vData.GetSize() == 0 || !vData.GetData()) {
      return; // or rather throw
    }

    bpVec2 vBeginXY{ aBlockIndexX * mBlockSize[0], aBlockIndexY * mBlockSize[1] };
    bpVec2 vEndXY{ std::min((aBlockIndexX + 1) * mBlockSize[0], mImageSize[0]), std::min((aBlockIndexY + 1) * mBlockSize[1], mImageSize[1]) };
    bpSize vBeginZ = aBlockIndexZ * mBlockSize[2];

    for (bpSize vIndexZ = 0; vIndexZ < mBlockSize[2]; vIndexZ++) {
      const TDataType* vDataBlockXY = vData.GetData() + mBlockSize[0] * mBlockSize[1] * vIndexZ;
      aResampler.CopyData(aIndexC, vBeginZ + vIndexZ, vBeginXY, vEndXY, vDataBlockXY, mBlockSize[0]);
    }
  }

  class cResampler
  {
  public:
    cResampler(bpSize aThumbnailSizeXY, const bpVec3& aImageSize, const std::vector<bpConverterTypes::cColorInfo>& aChannelColors, const bpConverterTypes::cImageExtent& aImageExtent)
      : mImageSize(aImageSize),
        mChannelColors(aChannelColors)
    {
      bpFloatVec3 vExtentsDelta{ aImageExtent.mExtentMaxX - aImageExtent.mExtentMinX, aImageExtent.mExtentMaxY - aImageExtent.mExtentMinY, aImageExtent.mExtentMaxZ - aImageExtent.mExtentMinZ };
      auto vSizeXY = bpComputeThumbnailSizeXY(aThumbnailSizeXY, mImageSize, vExtentsDelta);
      mSizeX = vSizeXY.first;
      mSizeY = vSizeXY.second;
      mMIP.resize(mChannelColors.size(), std::vector<TDataType>(mSizeX * mSizeY));
      mMiddle.resize(mChannelColors.size(), std::vector<TDataType>(mSizeX * mSizeY));
    }

    bpThumbnail GetMIP() const
    {
      return bpThumbnail(mSizeX, mSizeY, Colorize(mMIP, mChannelColors));
    }

    bpThumbnail GetMiddle() const
    {
      return bpThumbnail(mSizeX, mSizeY, Colorize(mMiddle, mChannelColors));
    }

    void CopyData(bpSize aIndexC, bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlockXY, bpSize aBlockDataSizeX)
    {
      bpVec2 vBeginXY{
        aBeginXY[0] * mSizeX / mImageSize[0],
        aBeginXY[1] * mSizeY / mImageSize[1]
      };
      bpVec2 vEndXY{
        std::min((aEndXY[0] * mSizeX + mImageSize[0] - 1) / mImageSize[0], mSizeX),
        std::min((aEndXY[1] * mSizeY + mImageSize[1] - 1) / mImageSize[1], mSizeY)
      };
      for (bpSize vIndexY = vBeginXY[1]; vIndexY < vEndXY[1]; ++vIndexY) {
        bpSize vY = vIndexY * mImageSize[1] / mSizeY;
        if (vY < aBeginXY[1] || vY >= aEndXY[1]) {
          continue;
        }
        for (bpSize vIndexX = vBeginXY[0]; vIndexX < vEndXY[0]; ++vIndexX) {
          bpSize vX = vIndexX * mImageSize[0] / mSizeX;
          if (vX < aBeginXY[0] || vX >= aEndXY[0]) {
            continue;
          }
          bpSize vSource = (vX - aBeginXY[0]) + (vY - aBeginXY[1]) * aBlockDataSizeX;
          bpSize vDest = vIndexX + vIndexY * mSizeX;
          TDataType vData = aDataBlockXY[vSource];
          if (vData > mMIP[aIndexC][vDest]) {
            mMIP[aIndexC][vDest] = vData;
          }
          if (aIndexZ == mImageSize[2] / 2) {
            mMiddle[aIndexC][vDest] = vData;
          }
        }
      }
    }

  private:
    const std::vector<bpConverterTypes::cColorInfo>& mChannelColors;

    std::vector<std::vector<TDataType>> mMIP;
    std::vector<std::vector<TDataType>> mMiddle;

    bpVec3 mImageSize;
    bpSize mSizeX;
    bpSize mSizeY;
  };

  static std::vector<bpUInt8> Colorize(const std::vector<std::vector<TDataType>>& aChannels, const std::vector<bpConverterTypes::cColorInfo>& aChannelColors)
  {
    std::vector<bpUInt8> vRGBA(aChannels[0].size() * 4);
    for (bpSize vIndexC = 0; vIndexC < aChannels.size(); ++vIndexC) {
      typename bpColorCache<TDataType>::tType vColorCache(aChannelColors[vIndexC]);
      const auto& vChannel = aChannels[vIndexC];
      for (bpSize vIndex = 0; vIndex < vChannel.size(); ++vIndex) {
        const bpConverterTypes::cColor& vColor = vColorCache.GetColor(vChannel[vIndex]);
        vRGBA[vIndex * 4 + 0] = std::max(vRGBA[vIndex * 4 + 0], static_cast<bpUInt8>(vColor.mRed * 255));
        vRGBA[vIndex * 4 + 1] = std::max(vRGBA[vIndex * 4 + 1], static_cast<bpUInt8>(vColor.mGreen * 255));
        vRGBA[vIndex * 4 + 2] = std::max(vRGBA[vIndex * 4 + 2], static_cast<bpUInt8>(vColor.mBlue * 255));
        vRGBA[vIndex * 4 + 3] = std::max(vRGBA[vIndex * 4 + 3], static_cast<bpUInt8>(vColor.mAlpha * 255));
      }
    }
    return vRGBA;
  }

  std::vector<std::vector<bpConstMemoryBlock<TDataType>>> mBlocksPerChannel;
  bpSize mThumbnailSizeXY;
  bpSize mIndexR;
  bpVec3 mImageSize;
  bpVec3 mBlockSize;
  bpVec3 mNBlocks;
};

#endif
