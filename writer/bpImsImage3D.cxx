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

#include "bpImsImage3D.h"

#include <algorithm>


template<typename TDataType>
bpImsImage3D<TDataType>::bpImsImage3D(bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ,
  bpSize aMemoryBlockSizeX, bpSize aMemoryBlockSizeY, bpSize aMemoryBlockSizeZ, bpSharedPtr<bpMemoryManager<TDataType> > aManager)
  : mSizeX(aSizeX),
    mSizeY(aSizeY),
    mSizeZ(aSizeZ),
    mMemoryBlockSizeX(aMemoryBlockSizeX),
    mMemoryBlockSizeY(aMemoryBlockSizeY),
    mMemoryBlockSizeZ(aMemoryBlockSizeZ),
    mLog2BlockSizeX(GetLog2BlockSize(mMemoryBlockSizeX)),
    mLog2BlockSizeY(GetLog2BlockSize(mMemoryBlockSizeY)),
    mLog2BlockSizeZ(GetLog2BlockSize(mMemoryBlockSizeZ)),
    mNBlocksX((aSizeX + mMemoryBlockSizeX - 1) / mMemoryBlockSizeX),
    mNBlocksY((aSizeY + mMemoryBlockSizeY - 1) / mMemoryBlockSizeY),
    mNBlocksZ((aSizeZ + mMemoryBlockSizeZ - 1) / mMemoryBlockSizeZ)
{
  // initialize memory blocks
  bpSize vNumberOfBlocks = mNBlocksX * mNBlocksY * mNBlocksZ;
  mBlocks.reserve(vNumberOfBlocks);
  for (bpSize vIndex = 0; vIndex < vNumberOfBlocks; vIndex++) {
    mBlocks.emplace_back(aMemoryBlockSizeX * aMemoryBlockSizeY * aMemoryBlockSizeZ, aManager);
  }
  mHistograms.resize(std::min<bpSize>(16, (vNumberOfBlocks + 63) / 64));
}

template<typename TDataType>
bpImsImage3D<TDataType>::~bpImsImage3D()
{
}


template<typename TDataType>
void bpImsImage3D<TDataType>::CopyData(bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlockXY)
{
  RegionToMemOperation(aIndexZ, aBeginXY, aEndXY, aDataBlockXY);
}


template<typename TDataType>
bpVec3 bpImsImage3D<TDataType>::GetMemoryBlockSize() const
{
  return{ mMemoryBlockSizeX, mMemoryBlockSizeY, mMemoryBlockSizeZ };
}

template<typename TDataType>
bpVec3 bpImsImage3D<TDataType>::GetNBlocks() const
{
  return{ mNBlocksX, mNBlocksY, mNBlocksZ };
}

template<typename TDataType>
bpVec3 bpImsImage3D<TDataType>::GetImageSize() const
{
  return{ mSizeX, mSizeY, mSizeZ };
}

template<typename TDataType>
bpImsImageBlock<TDataType>& bpImsImage3D<TDataType>::GetBlock(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) {
  return mBlocks[ConvertBlockIndex(aBlockIndexX, aBlockIndexY, aBlockIndexZ)];
}

template<typename TDataType>
bpHistogram bpImsImage3D<TDataType>::GetHistogram(bpSize aMaxNumberOfBins) const
{
  bpHistogram vHistogram = mHistograms.size() == 1 && mHistograms[0] ? mHistograms[0]->GetHistogram() : GetMergedHistogram();
  return vHistogram.GetNumberOfBins() <= aMaxNumberOfBins ? vHistogram : bpResampleHistogram(vHistogram, aMaxNumberOfBins);
}

template<typename TDataType>
bpHistogram bpImsImage3D<TDataType>::GetMergedHistogram() const
{
  bpHistogramBuilder<TDataType> vResult;
  for (const auto& vHistogram : mHistograms) {
    if (vHistogram) {
      vResult.Merge(*vHistogram);
    }
  }
  return vResult.GetHistogram();
}

template<typename TDataType>
bpSize bpImsImage3D<TDataType>::GetHistogramBuilderIndexForBlock(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) const
{
  return (aBlockIndexX + 4 * aBlockIndexY + 3 * aBlockIndexZ) % mHistograms.size();
}

template<typename TDataType>
bpHistogramBuilder<TDataType>& bpImsImage3D<TDataType>::GetHistogramBuilderForBlock(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ)
{
  auto& vHistogram = mHistograms[GetHistogramBuilderIndexForBlock(aBlockIndexX, aBlockIndexY, aBlockIndexZ)];
  if (!vHistogram) {
    vHistogram = std::make_unique<bpHistogramBuilder<TDataType>>();
  }
  return *vHistogram;
}

template<typename TDataType>
bpSize bpImsImage3D<TDataType>::GetMemoryBlockIndexX(bpSize aMemoryIndexX) const {
  return aMemoryIndexX >> mLog2BlockSizeX;
}

template<typename TDataType>
bpSize bpImsImage3D<TDataType>::GetMemoryBlockIndexY(bpSize aMemoryIndexY) const {
  return aMemoryIndexY >> mLog2BlockSizeY;
}

template<typename TDataType>
bpSize bpImsImage3D<TDataType>::GetMemoryBlockIndexZ(bpSize aMemoryIndexZ) const {
  return aMemoryIndexZ >> mLog2BlockSizeZ;
}

template<typename TDataType>
bpSize bpImsImage3D<TDataType>::GetLog2BlockSize(bpSize aBlockSize) const {
  bpSize vLog2BlockSize = 0;
  while ((((bpSize)1) << vLog2BlockSize) < aBlockSize) {
    ++vLog2BlockSize;
  }
  return vLog2BlockSize;
}


template<typename TDataType>
bpSize bpImsImage3D<TDataType>::ConvertBlockIndex(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) const {
  return aBlockIndexX + aBlockIndexY * mNBlocksX + aBlockIndexZ * mNBlocksX * mNBlocksY;
}


template<typename TDataType>
void bpImsImage3D<TDataType>::RegionToMemOperation(bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlockXY)
{
  bpSize vRegionBeginX = aBeginXY[0];
  bpSize vRegionBeginY = aBeginXY[1];
  bpSize vRegionBeginZ = aIndexZ;

  bpSize vRegionEndX = aEndXY[0];
  bpSize vRegionEndY = aEndXY[1];

  if (vRegionEndX <= vRegionBeginX || vRegionEndY <= vRegionBeginY) {
    return;
  }

  bpSize vBlockBeginX = GetMemoryBlockIndexX(vRegionBeginX);
  bpSize vBlockBeginY = GetMemoryBlockIndexY(vRegionBeginY);
  bpSize vBlockBeginZ = GetMemoryBlockIndexZ(vRegionBeginZ);

  bpSize vBlockEndX = std::min(GetMemoryBlockIndexX(vRegionEndX - 1) + 1, mNBlocksX);
  bpSize vBlockEndY = std::min(GetMemoryBlockIndexY(vRegionEndY - 1) + 1, mNBlocksY);

  bpSize vBlockSizeX = mMemoryBlockSizeX;
  bpSize vBlockSizeY = mMemoryBlockSizeY;
  bpSize vBlockSizeZ = mMemoryBlockSizeZ;
  bpSize vBlockSizeXY = vBlockSizeX * vBlockSizeY;

  bpSize vRegionSizeX = vRegionEndX - vRegionBeginX;
  bpSize vRegionSizeXY = vRegionSizeX * (vRegionEndY - vRegionBeginY);

  bpSize vBlockIndexZ = vBlockBeginZ;
  bpSize vZFirst = vBlockIndexZ * vBlockSizeZ;
  bpSize vBlockBeginOffsetZ = vRegionBeginZ - vZFirst;

  for (bpSize vBlockIndexY = vBlockBeginY; vBlockIndexY < vBlockEndY; ++vBlockIndexY) {
    bpSize vYFirst = vBlockIndexY * vBlockSizeY;
    bpSize vYLast = vYFirst + vBlockSizeY;
    bpSize vBlockBeginOffsetY = vRegionBeginY > vYFirst ? vRegionBeginY - vYFirst : 0;
    bpSize vBlockEndOffsetY = vYLast > vRegionEndY ? vBlockSizeY - (vYLast - vRegionEndY) : vBlockSizeY;
    for (bpSize vBlockIndexX = vBlockBeginX; vBlockIndexX < vBlockEndX; ++vBlockIndexX) {
      bpSize vXFirst = vBlockIndexX * vBlockSizeX;
      bpSize vXLast = vXFirst + vBlockSizeX;
      bpSize vBlockBeginOffsetX = vRegionBeginX > vXFirst ? vRegionBeginX - vXFirst : 0;
      bpSize vBlockEndOffsetX = vXLast > vRegionEndX ? vBlockSizeX - (vXLast - vRegionEndX) : vBlockSizeX;

      bpImsImageBlock<TDataType>& vMemoryBlock = GetBlock(vBlockIndexX, vBlockIndexY, vBlockIndexZ);

      bpSize vBlockRegionOffsetX = vXFirst + vBlockBeginOffsetX - vRegionBeginX;
      bpSize vBlockRegionSizeX = vBlockEndOffsetX - vBlockBeginOffsetX;
      bpSize vOffsetZ = vBlockBeginOffsetZ;

      if (vBlockBeginOffsetX == 0 && vRegionSizeX == mMemoryBlockSizeX) {

        bpSize vBlockBeginOffset = vOffsetZ * vBlockSizeXY + vBlockBeginOffsetY * vBlockSizeX + vBlockBeginOffsetX;
        bpSize vBlockRegionSizeXY = vBlockRegionSizeX * (vBlockEndOffsetY - vBlockBeginOffsetY);

        if (aDataBlockXY) {
          bpSize vBlockRegionOffset =
            (vZFirst + vOffsetZ - vRegionBeginZ) * vRegionSizeXY +
            (vYFirst + vBlockBeginOffsetY - vRegionBeginY) * vRegionSizeX +
            vBlockRegionOffsetX;

          const TDataType* vData = aDataBlockXY + vBlockRegionOffset;
          vMemoryBlock.CopyLinePartToBlock(vBlockBeginOffset, vBlockRegionSizeXY, vData);
        }
        else {
          vMemoryBlock.CopyLinePartToBlock(vBlockBeginOffset, vBlockRegionSizeXY, nullptr);
        }

        continue;
      }

      for (bpSize vOffsetY = vBlockBeginOffsetY; vOffsetY < vBlockEndOffsetY; ++vOffsetY) {

        bpSize vBlockBeginOffset = vOffsetZ * vBlockSizeXY + vOffsetY * vBlockSizeX + vBlockBeginOffsetX;

        if (aDataBlockXY) {
          bpSize vBlockRegionOffset =
            (vZFirst + vOffsetZ - vRegionBeginZ) * vRegionSizeXY +
            (vYFirst + vOffsetY - vRegionBeginY) * vRegionSizeX +
            vBlockRegionOffsetX;

          const TDataType* vData = aDataBlockXY + vBlockRegionOffset;
          vMemoryBlock.CopyLinePartToBlock(vBlockBeginOffset, vBlockRegionSizeX, vData);
        }
        else {
          vMemoryBlock.CopyLinePartToBlock(vBlockBeginOffset, vBlockRegionSizeX, nullptr);
        }

      }
    }
  }
}

template<typename TDataType>
bool bpImsImage3D<TDataType>::PadBorderBlockWithZeros(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ)
{
  bool vNeedsPadding = false;
  if (aBlockIndexX+1 == mNBlocksX && (aBlockIndexX + 1)*mMemoryBlockSizeX > mSizeX) {
    vNeedsPadding = true;
    bpVec2 vBeginXY = { mSizeX, aBlockIndexY*mMemoryBlockSizeY };
    bpVec2 vEndXY = { mNBlocksX*mMemoryBlockSizeX, (aBlockIndexY+1)*mMemoryBlockSizeY };
    for (bpSize vIndexZ = aBlockIndexZ*mMemoryBlockSizeZ; vIndexZ < (aBlockIndexZ + 1)*mMemoryBlockSizeZ; vIndexZ++) {
      RegionToMemOperation(vIndexZ, vBeginXY, vEndXY, nullptr);
    }
  }
  if (aBlockIndexY+1 == mNBlocksY && (aBlockIndexY + 1)*mMemoryBlockSizeY > mSizeY) {
    vNeedsPadding = true;
    bpVec2 vBeginXY = { aBlockIndexX*mMemoryBlockSizeX, mSizeY };
    bpVec2 vEndXY = { (aBlockIndexX + 1)*mMemoryBlockSizeX, mNBlocksY*mMemoryBlockSizeY };
    for (bpSize vIndexZ = aBlockIndexZ*mMemoryBlockSizeZ; vIndexZ < (aBlockIndexZ + 1)*mMemoryBlockSizeZ; vIndexZ++) {
        RegionToMemOperation(vIndexZ, vBeginXY, vEndXY, nullptr);
    }
  }
  if (aBlockIndexZ+1 == mNBlocksZ && (aBlockIndexZ + 1)*mMemoryBlockSizeZ > mSizeZ) {
    vNeedsPadding = true;
    bpVec2 vBeginXY = { aBlockIndexX*mMemoryBlockSizeX, aBlockIndexY*mMemoryBlockSizeY };
    bpVec2 vEndXY = { (aBlockIndexX+1)*mMemoryBlockSizeX, (aBlockIndexY+1)*mMemoryBlockSizeY };
    for (bpSize vIndexZ = mSizeZ; vIndexZ < mNBlocksZ*mMemoryBlockSizeZ; vIndexZ++) {
      RegionToMemOperation(vIndexZ, vBeginXY, vEndXY, nullptr);
    }
  }
  return vNeedsPadding;
}


template class bpImsImage3D<bpUInt8>;
template class bpImsImage3D<bpUInt16>;
template class bpImsImage3D<bpUInt32>;
template class bpImsImage3D<bpFloat>;
