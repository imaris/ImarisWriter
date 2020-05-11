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
#ifndef __BP_IMS_IMAGE_3D__
#define __BP_IMS_IMAGE_3D__

#include "bpImsImageBlock.h"
#include "bpMemoryManager.h"
#include "bpHistogram.h"

/**
* Ims image representing the dimensions X,Y,Z for one timepoint and one channel.
*/
template<typename TDataType>
class bpImsImage3D
{
public:
  bpImsImage3D(bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ,
    bpSize aBlockSizeX, bpSize aBlockSizeY, bpSize aBlockSizeZ, bpSharedPtr<bpMemoryManager<TDataType> > aManager);

  bpImsImage3D(const bpImsImage3D&) = delete;
  //bpImsImage3D& operator=(const bpImsImage3D&) = delete;
  bpImsImage3D(bpImsImage3D&&) = default;

  ~bpImsImage3D();

  void CopyData(bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlock);

  bpVec3 GetMemoryBlockSize() const;
  bpVec3 GetNBlocks() const;
  bpVec3 GetImageSize() const;

  bpImsImageBlock<TDataType>& GetBlock(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ);

  bpHistogram GetHistogram(bpSize aMaxNumberOfBins) const;

  bpSize GetHistogramBuilderIndexForBlock(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) const;
  bpHistogramBuilder<TDataType>& GetHistogramBuilderForBlock(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ);

  bool PadBorderBlockWithZeros(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ);
private:
  void RegionToMemOperation(bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlockXY);

  bpHistogram GetMergedHistogram() const;

  bpSize GetMemoryBlockIndexX(bpSize aMemoryIndexX) const;
  bpSize GetMemoryBlockIndexY(bpSize aMemoryIndexY) const;
  bpSize GetMemoryBlockIndexZ(bpSize aMemoryIndexZ) const;

  bpSize GetLog2BlockSize(bpSize aBlockSize) const;

  bpSize ConvertBlockIndex(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) const;

  std::vector<bpUniquePtr<bpHistogramBuilder<TDataType>>> mHistograms;

  std::vector<bpImsImageBlock<TDataType>> mBlocks;

  const bpSize mMemoryBlockSizeX;
  const bpSize mMemoryBlockSizeY;
  const bpSize mMemoryBlockSizeZ;
  const bpSize mLog2BlockSizeX;
  const bpSize mLog2BlockSizeY;
  const bpSize mLog2BlockSizeZ;
  const bpSize mSizeX;
  const bpSize mSizeY;
  const bpSize mSizeZ;
  const bpSize mNBlocksX;
  const bpSize mNBlocksY;
  const bpSize mNBlocksZ;
};

#endif // __BP_IMS_IMAGE_3D__
