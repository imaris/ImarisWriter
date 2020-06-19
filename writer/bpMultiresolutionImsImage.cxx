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

#include "bpMultiresolutionImsImage.h"
#include "bpWriterFactoryHDF5.h"
#include "bpOptimalBlockLayout.h"
#include "bpThreadPool.h"


static inline bpSize DivEx(bpSize aNum, bpSize aDiv)
{
  return (aNum + aDiv - 1) / aDiv;
}


template<typename TDataType>
bpMultiresolutionImsImage<TDataType>::bpMultiresolutionImsImage(
  bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ, bpSize aSizeC, bpSize aSizeT,
  bpConverterTypes::tDataType aDataType,
  const bpVec2& aCopyBlockSizeXY, const bpVec2& aSampleXY,
  const bpSharedPtr<bpWriterFactory>& aWriterFactory,
  const bpString& aOutputFile, bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType,
  bpSize aThumbnailSizeXY, bool aForceFileBlockSizeZ1, bpSize aNumberOfThreads)
: mMaxRunningJobsPerThread(32),
  mCopyBlockSizeXY(aCopyBlockSizeXY),
  mSampleXY(aSampleXY)
{
  bool vReduceZ = !aForceFileBlockSizeZ1;
  std::vector<bpVec3> vResolutionSizes = GetOptimalImagePyramid(bpVec3{ aSizeX, aSizeY, aSizeZ }, vReduceZ);
  std::vector<bpVec3> vResolutionBlockSizes = ComputeMemoryBlockSizes(vResolutionSizes, aSizeT);

  if (aForceFileBlockSizeZ1) {
    for (bpVec3& vBlockSize : vResolutionBlockSizes) {
      vBlockSize[2] = 1;
    }
  }

  bpImsLayout vLayout(vResolutionSizes, aSizeT, aSizeC, vResolutionBlockSizes, aDataType);

  mWriter = aWriterFactory->CreateWriter(aOutputFile, vLayout, aCompressionAlgorithmType);

  bpSharedPtr<bpMemoryManager<TDataType> > vMemoryManager = std::make_shared<bpMemoryManager<TDataType> >();

  bpSize vResolutionLevels = vResolutionSizes.size();
  mImages.reserve(vResolutionLevels);
  for (bpSize vIndex = 0; vIndex < vResolutionLevels; vIndex++) {
    const bpVec3& vSize = vResolutionSizes[vIndex];
    const bpVec3& vBlockSize = vResolutionBlockSizes[vIndex];
    mImages.emplace_back(vSize[0], vSize[1], vSize[2], aSizeC, aSizeT, vBlockSize[0], vBlockSize[1], vBlockSize[2], vMemoryManager);
  }

  mThumbnailBuilder = std::make_shared<bpThumbnailBuilder<TDataType>>(aThumbnailSizeXY, vResolutionSizes, vResolutionBlockSizes, aSizeC);
  mComputeThread = std::make_shared<bpThreadPool>(1);

  for (bpSize vIndex = 0; vIndex < aNumberOfThreads; vIndex++) {
    mHistogramThreads.push_back(std::make_shared<bpThreadPool>(1));
  }

  mCopyBlocksLeft.resize(vResolutionLevels);
  for (bpSize vResolution = 0; vResolution < vResolutionLevels; vResolution++) {
    InitCopyBlocksLeft(vResolution);
  }
}


template<typename TDataType>
bpMultiresolutionImsImage<TDataType>::~bpMultiresolutionImsImage()
{
}


static bool IsEmpty(const bpHistogram& aHistogram)
{
  bpSize vSize = aHistogram.GetNumberOfBins();
  for (bpSize vIndex = 0; vIndex < vSize; vIndex++) {
    if (aHistogram.GetCount(vIndex) > 0) {
      return false;
    }
  }
  return true;
}


template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::FinishWriteDataBlocks()
{
  mComputeThread->WaitAll();
  for (const auto& vThread : mHistogramThreads) {
    vThread->WaitAll();
  }

  bpSize vSizeR = mImages.size();
  bpSize vSizeT = mImages[0].GetSizeT();
  bpSize vSizeC = mImages[0].GetSizeC();
  for (bpSize vIndexR = 0; vIndexR < vSizeR; ++vIndexR) {
    for (bpSize vIndexT = 0; vIndexT < vSizeT; ++vIndexT) {
      for (bpSize vIndexC = 0; vIndexC < vSizeC; ++vIndexC) {
        if (!mHistogramThreads.empty()) {
          mHistogramThreads[(vIndexT + vSizeT * vIndexC) % mHistogramThreads.size()]->Run([this, vIndexT, vIndexC, vIndexR] {
            bpHistogram vHistogram = mImages[vIndexR].GetImage3D(vIndexT, vIndexC).GetHistogram(1024);
            if (!IsEmpty(vHistogram)) {
              mWriter->WriteHistogram(vHistogram, vIndexT, vIndexC, vIndexR);
            }
          });
        }
        else {
          bpHistogram vHistogram = mImages[vIndexR].GetImage3D(vIndexT, vIndexC).GetHistogram(1024);
          if (!IsEmpty(vHistogram)) {
            mWriter->WriteHistogram(vHistogram, vIndexT, vIndexC, vIndexR);
          }
        }
      }
    }
  }

  for (const auto& vThread : mHistogramThreads) {
    vThread->WaitAll();
  }

  mWriter->FinishWriteDataBlocks();
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::WriteMetadata(
  const bpString& aApplicationName,
  const bpString& aApplicationVersion,
  const bpConverterTypes::cImageExtent& aImageExtent,
  const bpConverterTypes::tParameters& aParameters,
  const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
  const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel)
{
  mWriter->WriteMetadata(aApplicationName, aApplicationVersion, aImageExtent, aParameters, aTimeInfoPerTimePoint, aColorInfoPerChannel);
  mWriter->WriteThumbnail(mThumbnailBuilder->CreateThumbnail(aColorInfoPerChannel, aImageExtent));
}

template<typename TDataType>
bpHistogram bpMultiresolutionImsImage<TDataType>::GetChannelHistogram(bpSize aIndexC) const
{
  const bpImsImage5D<TDataType>& vImage = mImages[0];
  bpSize vSizeT = vImage.GetSizeT();
  if (vSizeT == 1) {
    return vImage.GetImage3D(0, aIndexC).GetHistogram(1024);
  }

  std::vector<bpHistogram> vHistograms;
  vHistograms.reserve(vSizeT);
  bpFloat vMin = 0;
  bpFloat vMax = 0;
  for (bpSize vIndexT = 0; vIndexT < vSizeT; vIndexT++) {
    vHistograms.push_back(vImage.GetImage3D(vIndexT, aIndexC).GetHistogram(256 * 256));
    const auto& vHist = vHistograms.back();
    if (vIndexT == 0 || vHist.GetMin() < vMin) {
      vMin = vHist.GetMin();
    }
    if (vIndexT == 0 || vHist.GetMax() > vMax) {
      vMax= vHist.GetMax();
    }
  }
  std::vector<bpUInt64> vBins(256 * 256);
  bpHistogram vLayout(vMin, vMax, vBins);
  for (const auto& vHist : vHistograms) {
    for (bpSize vBinId = 0; vBinId < vHist.GetNumberOfBins(); vBinId++) {
      bpSize vDestBinId = vLayout.GetBin(vHist.GetBinValue(vBinId));
      vBins[vDestBinId] += vHist.GetCount(vBinId);
    }
  }
  bpHistogram vHistogram(vMin, vMax, vBins);
  if (vHistogram.GetNumberOfBins() > 1024) {
    vHistogram = bpResampleHistogram(vHistogram, 1024);
  }
  return vHistogram;
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::CopyData(bpSize aIndexT, bpSize aIndexC, bpSize aIndexZ, const bpVec2& aCopyBlockIndexXY, const TDataType* aDataBlockXY, bpSize aIndexR)
{
  auto& vImage5D = mImages[aIndexR];
  auto& vImage3D = vImage5D.GetImage3D(aIndexT, aIndexC);
  bpVec3 vImageSize = vImage3D.GetImageSize();

  bpVec2 vBeginXY = { DivEx(aCopyBlockIndexXY[0] * mCopyBlockSizeXY[0], mSampleXY[0]), DivEx(aCopyBlockIndexXY[1] * mCopyBlockSizeXY[1], mSampleXY[1]) };
  bpVec2 vEndXY = { DivEx((aCopyBlockIndexXY[0] + 1) * mCopyBlockSizeXY[0], mSampleXY[0]), DivEx((aCopyBlockIndexXY[1] + 1) * mCopyBlockSizeXY[1], mSampleXY[1]) };
  if (vBeginXY[0] >= vImageSize[0] || vBeginXY[1] >= vImageSize[1] || aIndexZ >= vImageSize[2]) {
    return;
  }

  vImage5D.CopyData(aIndexT, aIndexC, aIndexZ, vBeginXY, vEndXY, aDataBlockXY);

  OnCopiedData(aIndexT, aIndexC, { aCopyBlockIndexXY[0], aCopyBlockIndexXY[1], aIndexZ }, aIndexR);
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::OnCopiedData(bpSize aIndexT, bpSize aIndexC, const bpVec3& aBlockIndexXYZ, bpSize aIndexR)
{
  bpThreadPool::tFunction vFunction = [this, aIndexT, aIndexC, aBlockIndexXYZ, aIndexR] {
    OnCopiedDataImpl(aIndexT, aIndexC, aBlockIndexXYZ, aIndexR);
  };
  if (aIndexR == 0) {
    mComputeThread->WaitSome(mMaxRunningJobsPerThread);
  }
  mComputeThread->Run(vFunction, {}, aIndexR > 0);
}

template<typename TDataType>
bpSharedPtr<bpThreadPool> bpMultiresolutionImsImage<TDataType>::GetHistogramThread(bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpVec3& aBlockIndex) const
{
  if (mHistogramThreads.empty()) {
    return{};
  }

  bpSize vHistogramIndex = mImages[aIndexR].GetImage3D(aIndexT, aIndexC).GetHistogramBuilderIndexForBlock(aBlockIndex[0], aBlockIndex[1], aBlockIndex[2]);
  bpSharedPtr<bpThreadPool> vThread = mHistogramThreads[(vHistogramIndex + aIndexR * 5 + aIndexT + mImages[0].GetSizeT() * aIndexC) % mHistogramThreads.size()];
  vThread->WaitSome(mMaxRunningJobsPerThread);
  return vThread;
}

template<typename TDataType>
bpSize bpMultiresolutionImsImage<TDataType>::GetMemoryBlockIndex(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ, bpSize aIndexC, bpSize aIndexT, bpSize aIndexR) const
{
  bpSize vSizeC = mImages[aIndexR].GetSizeC();
  bpVec3 vNBlocks = mImages[aIndexR].GetImage3D(0, 0).GetNBlocks();
  bpSize vBlockIndex = aBlockIndexX + vNBlocks[0] * (aBlockIndexY + vNBlocks[1] * (aBlockIndexZ + vNBlocks[2] * (aIndexC + vSizeC * aIndexT)));
  return vBlockIndex;
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::InitCopyBlocksLeft(bpSize aIndexR)
{
  const auto& vImage5D = mImages[aIndexR];
  const auto& vImage3D = vImage5D.GetImage3D(0, 0);
  bpVec3 vImageSize = vImage3D.GetImageSize();
  // determine CopyBlockSize
  bpVec3 vCopyBlockSize;
  bpVec3 vNCopyBlocks;
  bpVec2 vSampleXY{ 1, 1 };
  if (aIndexR == 0) {
    vSampleXY = mSampleXY;
    vCopyBlockSize[0] = mCopyBlockSizeXY[0];
    vCopyBlockSize[1] = mCopyBlockSizeXY[1];
    vCopyBlockSize[2] = 1;
    vNCopyBlocks[0] = DivEx(vImageSize[0] * vSampleXY[0], vCopyBlockSize[0]);
    vNCopyBlocks[1] = DivEx(vImageSize[1] * vSampleXY[1], vCopyBlockSize[1]);
    vNCopyBlocks[2] = DivEx(vImageSize[2], vCopyBlockSize[2]);
  }
  else {
    const auto& vImage5DHigherRes = mImages[aIndexR - 1];
    const auto& vImage3DHigherRes = vImage5DHigherRes.GetImage3D(0, 0);
    bpVec3 vMemoryBlockSizeHigherRes = vImage3DHigherRes.GetMemoryBlockSize();
    bpVec3 vStride = GetStrideToNextResolution(aIndexR - 1);
    vCopyBlockSize = {
      vMemoryBlockSizeHigherRes[0] / vStride[0] ,
      vMemoryBlockSizeHigherRes[1] / vStride[1],
      vMemoryBlockSizeHigherRes[2] / vStride[2]
    };
    vNCopyBlocks = vImage3DHigherRes.GetNBlocks();
  }

  bpSize vSizeC = vImage5D.GetSizeC();
  bpSize vSizeT = vImage5D.GetSizeT();

  // If a block, that was just modified, is full, write it to file and copy the block's data to the corresponding block of the lower resolution image.
  bpVec3 vNMemoryBlocks = vImage3D.GetNBlocks();
  bpVec3 vMemoryBlockSize = vImage3D.GetMemoryBlockSize();

  mCopyBlocksLeft[aIndexR].resize(vNMemoryBlocks[0] * vNMemoryBlocks[1] * vNMemoryBlocks[2] * vSizeC * vSizeT);
  for (bpSize vMemoryBlockIndexZ = 0; vMemoryBlockIndexZ < vNMemoryBlocks[2]; ++vMemoryBlockIndexZ) {
    for (bpSize vMemoryBlockIndexY = 0; vMemoryBlockIndexY < vNMemoryBlocks[1]; ++vMemoryBlockIndexY) {
      for (bpSize vMemoryBlockIndexX = 0; vMemoryBlockIndexX < vNMemoryBlocks[0]; ++vMemoryBlockIndexX) {
        bpSize vCopyBlockIndexBeginX = vMemoryBlockIndexX * vMemoryBlockSize[0] * vSampleXY[0] / vCopyBlockSize[0];
        bpSize vCopyBlockIndexBeginY = vMemoryBlockIndexY * vMemoryBlockSize[1] * vSampleXY[1] / vCopyBlockSize[1];
        bpSize vCopyBlockIndexBeginZ = vMemoryBlockIndexZ * vMemoryBlockSize[2] / vCopyBlockSize[2];
        bpSize vCopyBlockIndexEndX = std::min(DivEx((vMemoryBlockIndexX + 1) * vMemoryBlockSize[0] * vSampleXY[0], vCopyBlockSize[0]), vNCopyBlocks[0]);
        bpSize vCopyBlockIndexEndY = std::min(DivEx((vMemoryBlockIndexY + 1) * vMemoryBlockSize[1] * vSampleXY[1], vCopyBlockSize[1]), vNCopyBlocks[1]);
        bpSize vCopyBlockIndexEndZ = std::min(DivEx((vMemoryBlockIndexZ + 1) * vMemoryBlockSize[2], vCopyBlockSize[2]), vNCopyBlocks[2]);

        bpSize vCopyBlockCount = (vCopyBlockIndexEndX - vCopyBlockIndexBeginX) * (vCopyBlockIndexEndY - vCopyBlockIndexBeginY) * (vCopyBlockIndexEndZ - vCopyBlockIndexBeginZ);
        for (bpSize vIndexT = 0; vIndexT < vSizeT; ++vIndexT) {
          for (bpSize vIndexC = 0; vIndexC < vSizeC; ++vIndexC) {
            bpSize vMemoryBlockIndex = GetMemoryBlockIndex(vMemoryBlockIndexX, vMemoryBlockIndexY, vMemoryBlockIndexZ, vIndexC, vIndexT, aIndexR);
            mCopyBlocksLeft[aIndexR][vMemoryBlockIndex] = vCopyBlockCount;
          }
        }
      }
    }
  }
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::OnCopiedDataImpl(bpSize aIndexT, bpSize aIndexC, const bpVec3& aCopyBlockIndexXYZ, bpSize aIndexR)
{
  auto& vImage5D = mImages[aIndexR];
  auto& vImage3D = vImage5D.GetImage3D(aIndexT, aIndexC);
  bpVec3 vImageSize = vImage3D.GetImageSize();
  // determine CopyBlockSize
  bpVec3 vCopyBlockSize;
  bpVec3 vNCopyBlocks;
  bpVec2 vSampleXY{ 1, 1 };
  if (aIndexR == 0) {
    vSampleXY = mSampleXY;
    vCopyBlockSize[0] = mCopyBlockSizeXY[0];
    vCopyBlockSize[1] = mCopyBlockSizeXY[1];
    vCopyBlockSize[2] = 1;
    vNCopyBlocks[0] = DivEx(vImageSize[0] * vSampleXY[0], vCopyBlockSize[0]);
    vNCopyBlocks[1] = DivEx(vImageSize[1] * vSampleXY[1], vCopyBlockSize[1]);
    vNCopyBlocks[2] = DivEx(vImageSize[2], vCopyBlockSize[2]);
  }
  else {
    auto& vImage5DHigherRes = mImages[aIndexR-1];
    auto& vImage3DHigherRes = vImage5DHigherRes.GetImage3D(aIndexT, aIndexC);
    bpVec3 vMemoryBlockSizeHigherRes = vImage3DHigherRes.GetMemoryBlockSize();
    bpVec3 vStride = GetStrideToNextResolution(aIndexR - 1);
    vCopyBlockSize = {
      vMemoryBlockSizeHigherRes[0] / vStride[0],
      vMemoryBlockSizeHigherRes[1] / vStride[1],
      vMemoryBlockSizeHigherRes[2] / vStride[2]
    };
    vNCopyBlocks = vImage3DHigherRes.GetNBlocks();
  }
  bpVec2 vBeginXY = { DivEx(aCopyBlockIndexXYZ[0] * vCopyBlockSize[0], vSampleXY[0]), DivEx(aCopyBlockIndexXYZ[1] * vCopyBlockSize[1], vSampleXY[1]) };
  bpVec2 vEndXY = { DivEx((aCopyBlockIndexXYZ[0] + 1) * vCopyBlockSize[0], vSampleXY[0]), DivEx((aCopyBlockIndexXYZ[1] + 1) * vCopyBlockSize[1], vSampleXY[1]) };
  bpSize vBeginZ = aCopyBlockIndexXYZ[2] * vCopyBlockSize[2];
  if (vBeginXY[0] >= vImageSize[0] || vBeginXY[1] >= vImageSize[1] || vBeginZ >= vImageSize[2]) {
    return;
  }

  // If a block, that was just modified, is full, write it to file and copy the block's data to the corresponding block of the lower resolution image.
  bpVec3 vNMemoryBlocks = vImage3D.GetNBlocks();
  bpVec3 vMemoryBlockSize = vImage3D.GetMemoryBlockSize();

  // find memory blocks overlapping with aCopyBlockIndex
  bpSize vMemoryBlockIndexBeginX = vBeginXY[0] / vMemoryBlockSize[0];
  bpSize vMemoryBlockIndexBeginY = vBeginXY[1] / vMemoryBlockSize[1];
  bpSize vMemoryBlockIndexZ = aCopyBlockIndexXYZ[2] * vCopyBlockSize[2] / vMemoryBlockSize[2];
  bpSize vMemoryBlockIndexEndX = std::min(DivEx(vEndXY[0], vMemoryBlockSize[0]), vNMemoryBlocks[0]);
  bpSize vMemoryBlockIndexEndY = std::min(DivEx(vEndXY[1], vMemoryBlockSize[1]), vNMemoryBlocks[1]);
  bpSize vMemoryBlockIndexBegin = GetMemoryBlockIndex(0, 0, vMemoryBlockIndexZ, aIndexC, aIndexT, aIndexR);
  for (bpSize vMemoryBlockIndexX = vMemoryBlockIndexBeginX; vMemoryBlockIndexX < vMemoryBlockIndexEndX; ++vMemoryBlockIndexX) {
    for (bpSize vMemoryBlockIndexY = vMemoryBlockIndexBeginY; vMemoryBlockIndexY < vMemoryBlockIndexEndY; ++vMemoryBlockIndexY) {
      bpSize vMemoryBlockIndex = vMemoryBlockIndexBegin + vMemoryBlockIndexX + vMemoryBlockIndexY * vNMemoryBlocks[0];
      bpSize& vCopyBlocksLeft = mCopyBlocksLeft[aIndexR][vMemoryBlockIndex];
      --vCopyBlocksLeft;
      bool vAllCopied = vCopyBlocksLeft == 0;
      if (vAllCopied) {
        bpImsImageBlock<TDataType>& vMemoryBlock = vImage5D.GetBlock(aIndexT, aIndexC, vMemoryBlockIndexX, vMemoryBlockIndexY, vMemoryBlockIndexZ);
        vImage5D.PadBorderBlockWithZeros(vMemoryBlockIndexX, vMemoryBlockIndexY, vMemoryBlockIndexZ, aIndexC, aIndexT);
        bpConstMemoryBlock<TDataType> vData = vMemoryBlock.ReleaseMemory();
        bpSize vResolutionLevels = mImages.size();
        bpVec3 vHigherResBlockIndex = { vMemoryBlockIndexX, vMemoryBlockIndexY, vMemoryBlockIndexZ };

        bpWriter::tPreFunction vResample;
        if (aIndexR + 1 < vResolutionLevels) {
          InitLowResBlock(vHigherResBlockIndex, aIndexR, aIndexT, aIndexC);
          vResample = [this, vHigherResBlockIndex, aIndexR, aIndexT, aIndexC, vData] {
            ResampleBlock(vHigherResBlockIndex, aIndexR, aIndexT, aIndexC, vData);
          };
        }

        const auto& vHistogramThread = GetHistogramThread(aIndexR, aIndexT, aIndexC, vHigherResBlockIndex);
        if (vHistogramThread) {
          vHistogramThread->Run([this, &vImage3D, vHigherResBlockIndex, vData] {
            AddHistogramValues(vImage3D, vHigherResBlockIndex, vData);
          });
        }
        else {
          AddHistogramValues(vImage3D, vHigherResBlockIndex, vData);
        }

        mWriter->StartWriteDataBlock(vData, vMemoryBlockIndexX, vMemoryBlockIndexY, vMemoryBlockIndexZ, aIndexT, aIndexC, aIndexR, std::move(vResample));
        mThumbnailBuilder->StartCopyDataBlock(vData, vMemoryBlockIndexX, vMemoryBlockIndexY, vMemoryBlockIndexZ, aIndexT, aIndexC, aIndexR);
      }
    }
  }
}

template<typename TDataType>
bpVec3 bpMultiresolutionImsImage<TDataType>::GetStrideToNextResolution(bpSize aIndexR) const
{
  const bpImsImage3D<TDataType>& vHigherResImage = mImages[aIndexR].GetImage3D(0, 0);
  const bpImsImage3D<TDataType>& vLowerResImage = mImages[aIndexR + 1].GetImage3D(0, 0);
  bpVec3 vStride{ 1, 1, 1 };
  for (bpSize vDim = 0; vDim < 3; vDim++) {
    if (vLowerResImage.GetImageSize()[vDim] < vHigherResImage.GetImageSize()[vDim]) {
      vStride[vDim] = 2;
    }
  }
  return vStride;
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::InitLowResBlock(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC)
{
  bpVec3 vStride = GetStrideToNextResolution(aIndexR);
  bpSize vSmallIndexR = aIndexR + 1;
  const bpImsImage3D<TDataType>& vHigherResImage = mImages[aIndexR].GetImage3D(aIndexT, aIndexC);
  bpImsImage3D<TDataType>& vLowerResImage = mImages[vSmallIndexR].GetImage3D(aIndexT, aIndexC);
  bpVec3 vMemoryBlockSize = vHigherResImage.GetMemoryBlockSize();
  bpVec3 vLowerResBlockSize = vLowerResImage.GetMemoryBlockSize();

  //minIndex is first voxel in block, maxIndex is last (+1) in block or last voxel (+1) of image
  bpVec3 aLargeIndexMin;
  bpVec3 aLargeIndexMax;
  for (bpSize vDim = 0; vDim < 3; vDim++) {
    aLargeIndexMin[vDim] = aHigherResBlockIndex[vDim] * vMemoryBlockSize[vDim];
    aLargeIndexMax[vDim] = std::min((aHigherResBlockIndex[vDim] + 1) * vMemoryBlockSize[vDim], vHigherResImage.GetImageSize()[vDim]);
  }

  if (aLargeIndexMin[0] >= aLargeIndexMax[0] || aLargeIndexMin[1] >= aLargeIndexMax[1] || aLargeIndexMin[2] >= aLargeIndexMax[2]) {
    return;
  }

  // special case: if higher resolution block only has one row and there is no corresponding lower resolution block anymore
  bpVec3 vLowerResNBlocks = vLowerResImage.GetNBlocks();

  bpSize vBlockIndexSmallMinX = (aLargeIndexMin[0] / vStride[0]) / vLowerResBlockSize[0];
  bpSize vBlockIndexSmallMaxX = std::min(((aLargeIndexMax[0] - 1) / vStride[0]) / vLowerResBlockSize[0] + 1, vLowerResNBlocks[0]);
  bpSize vBlockIndexSmallMinY = (aLargeIndexMin[1] / vStride[1]) / vLowerResBlockSize[1];
  bpSize vBlockIndexSmallMaxY = std::min(((aLargeIndexMax[1] - 1) / vStride[1]) / vLowerResBlockSize[1] + 1, vLowerResNBlocks[1]);
  bpSize vBlockIndexSmallMinZ = (aLargeIndexMin[2] / vStride[2]) / vLowerResBlockSize[2];
  bpSize vBlockIndexSmallMaxZ = std::min(((aLargeIndexMax[2] - 1) / vStride[2]) / vLowerResBlockSize[2] + 1, vLowerResNBlocks[2]);

  bpSize vNBlocksX = vBlockIndexSmallMaxX - vBlockIndexSmallMinX;
  bpSize vNBlocksY = vBlockIndexSmallMaxY - vBlockIndexSmallMinY;
  bpSize vNBlocksZ = vBlockIndexSmallMaxZ - vBlockIndexSmallMinZ;

  if (vNBlocksX == 0 || vNBlocksY == 0 || vNBlocksZ == 0) {
    return;
  }
  if (vNBlocksX > 1 || vNBlocksY > 1 || vNBlocksZ > 1) {
    throw "image layout";
  }

  bpImsImageBlock<TDataType>& vSmallMemoryBlock = vLowerResImage.GetBlock(vBlockIndexSmallMinX, vBlockIndexSmallMinY, vBlockIndexSmallMinZ);

  //tLock vLock(mMutexInitBlocks);
  vSmallMemoryBlock.GetData();
}

template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::ResampleBlock(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData)
{
  bpVec3 vStride = GetStrideToNextResolution(aIndexR);
  ResampleBlockD(aHigherResBlockIndex, aIndexR, aIndexT, aIndexC, aBlockData, vStride);
}


template<typename TDataType>
template<bpSize Dim, bpSize... Stride>
std::enable_if_t<(Dim < 3)> bpMultiresolutionImsImage<TDataType>::ResampleBlockD(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData, const bpVec3& aStride)
{
  if (aStride[Dim] == 2) {
    ResampleBlockD<Dim + 1, Stride..., 2>(aHigherResBlockIndex, aIndexR, aIndexT, aIndexC, aBlockData, aStride);
  }
  else {
    ResampleBlockD<Dim + 1, Stride..., 1>(aHigherResBlockIndex, aIndexR, aIndexT, aIndexC, aBlockData, aStride);
  }
}


template<typename TDataType>
template<bpSize Dim, bpSize... Stride>
std::enable_if_t<(Dim == 3)> bpMultiresolutionImsImage<TDataType>::ResampleBlockD(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData, const bpVec3& aStride)
{
  bool vAddHistogramValues = false;
  if (vAddHistogramValues) {
    ResampleBlockT<Stride..., true>(aHigherResBlockIndex, aIndexR, aIndexT, aIndexC, aBlockData);
  }
  else {
    ResampleBlockT<Stride..., false>(aHigherResBlockIndex, aIndexR, aIndexT, aIndexC, aBlockData);
  }
}


template<typename TDataType>
template<bpSize StrideX, bpSize StrideY, bpSize StrideZ, bool ShouldAddHistogramValues>
void bpMultiresolutionImsImage<TDataType>::ResampleBlockT(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData)
{
  bpSize vSmallIndexR = aIndexR + 1;
  bpImsImage3D<TDataType>& vHigherResImage = mImages[aIndexR].GetImage3D(aIndexT, aIndexC);
  bpImsImage3D<TDataType>& vLowerResImage = mImages[vSmallIndexR].GetImage3D(aIndexT, aIndexC);
  bpVec3 vMemoryBlockSize = vHigherResImage.GetMemoryBlockSize();
  bpVec3 vLowerResBlockSize = vLowerResImage.GetMemoryBlockSize();
  const TDataType* vData = aBlockData.GetData();

  //minIndex is first voxel in block, maxIndex is last (+1) in block or last voxel (+1) of image
  bpVec3 aLargeIndexMin;
  bpVec3 aLargeIndexMax;
  for (bpSize vDim = 0; vDim < 3; vDim++) {
    aLargeIndexMin[vDim] = aHigherResBlockIndex[vDim] * vMemoryBlockSize[vDim];
    aLargeIndexMax[vDim] = std::min((aHigherResBlockIndex[vDim] + 1) * vMemoryBlockSize[vDim], vHigherResImage.GetImageSize()[vDim]);
  }

  if (aLargeIndexMin[0] >= aLargeIndexMax[0] || aLargeIndexMin[1] >= aLargeIndexMax[1] || aLargeIndexMin[2] >= aLargeIndexMax[2]) {
    return;
  }

  // special case: if higher resolution block only has one row and there is no corresponding lower resolution block anymore
  bpVec3 vLowerResNBlocks = vLowerResImage.GetNBlocks();

  bpSize vBlockIndexSmallMinX = (aLargeIndexMin[0] / StrideX) / vLowerResBlockSize[0];
  bpSize vBlockIndexSmallMaxX = std::min(((aLargeIndexMax[0] - 1) / StrideX) / vLowerResBlockSize[0] + 1, vLowerResNBlocks[0]);
  bpSize vBlockIndexSmallMinY = (aLargeIndexMin[1] / StrideY) / vLowerResBlockSize[1];
  bpSize vBlockIndexSmallMaxY = std::min(((aLargeIndexMax[1] - 1) / StrideY) / vLowerResBlockSize[1] + 1, vLowerResNBlocks[1]);
  bpSize vBlockIndexSmallMinZ = (aLargeIndexMin[2] / StrideZ) / vLowerResBlockSize[2];
  bpSize vBlockIndexSmallMaxZ = std::min(((aLargeIndexMax[2] - 1) / StrideZ) / vLowerResBlockSize[2] + 1, vLowerResNBlocks[2]);

  bpSize vNBlocksX = vBlockIndexSmallMaxX - vBlockIndexSmallMinX;
  bpSize vNBlocksY = vBlockIndexSmallMaxY - vBlockIndexSmallMinY;
  bpSize vNBlocksZ = vBlockIndexSmallMaxZ - vBlockIndexSmallMinZ;

  if (vNBlocksX == 0 || vNBlocksY == 0 || vNBlocksZ == 0) {
    if (ShouldAddHistogramValues) {
      AddHistogramValues(vHigherResImage, aHigherResBlockIndex, aBlockData);
    }
    return;
  }
  if (vNBlocksX > 1 || vNBlocksY > 1 || vNBlocksZ > 1) {
    throw "image layout";
  }

  // get memoryBlockSize of lowerResBlock
  bpSize vSmallBlockSizeX = vLowerResBlockSize[0];
  bpSize vSmallBlockSizeY = vLowerResBlockSize[1];
  bpSize vSmallBlockSizeZ = vLowerResBlockSize[2];

  // get memoryBlockSize of higherResBlock
  bpSize vLargeBlockSizeX = vMemoryBlockSize[0];
  bpSize vLargeBlockSizeXY = vLargeBlockSizeX * vMemoryBlockSize[1];

  // vNBlocks is 1 for our case, what is the middle case here?
  bpVec3 vLowerResImageSize = vLowerResImage.GetImageSize();
  bpSize vLargeMaxX = std::min(aLargeIndexMax[0], std::min(aLargeIndexMin[0] + vNBlocksX * vSmallBlockSizeX * StrideX, vLowerResImageSize[0] * StrideX));
  bpSize vLargeMaxY = std::min(aLargeIndexMax[1], std::min(aLargeIndexMin[1] + vNBlocksY * vSmallBlockSizeY * StrideY, vLowerResImageSize[1] * StrideY));
  bpSize vLargeMaxZ = std::min(aLargeIndexMax[2], std::min(aLargeIndexMin[2] + vNBlocksZ * vSmallBlockSizeZ * StrideZ, vLowerResImageSize[2] * StrideZ));

  // as we don't need absolute index, we can compute HigherResRegionSizes
  bpSize vLargeRegionX = vLargeMaxX - aLargeIndexMin[0];
  bpSize vLargeRegionY = vLargeMaxY - aLargeIndexMin[1];
  bpSize vLargeRegionZ = vLargeMaxZ - aLargeIndexMin[2];

  // special case: if higher resolution block has only one row, but there is a corresponding lower resolution block
  if (vLargeRegionX == 0 || vLargeRegionY == 0 || vLargeRegionZ == 0) {
    if (ShouldAddHistogramValues) {
      AddHistogramValues(vHigherResImage, aHigherResBlockIndex, aBlockData);
    }
    return;
  }

  bpSize vSmallBeginX = aLargeIndexMin[0] / StrideX;
  bpSize vSmallBeginY = aLargeIndexMin[1] / StrideY;
  bpSize vSmallBeginZ = aLargeIndexMin[2] / StrideZ;

  bpSize vSmallOffsetX = vSmallBeginX - vBlockIndexSmallMinX * vLowerResBlockSize[0];
  bpSize vSmallOffsetY = vSmallBeginY - vBlockIndexSmallMinY * vLowerResBlockSize[1];
  bpSize vSmallOffsetZ = vSmallBeginZ - vBlockIndexSmallMinZ * vLowerResBlockSize[2];

  bpImsImageBlock<TDataType>& vSmallMemoryBlock = vLowerResImage.GetBlock(vBlockIndexSmallMinX, vBlockIndexSmallMinY, vBlockIndexSmallMinZ);
  TDataType* vResult = vSmallMemoryBlock.GetData() + vSmallOffsetX + vSmallBlockSizeX * (vSmallOffsetY + vSmallBlockSizeY * vSmallOffsetZ);

  bpHistogramBuilder<TDataType>* vHighResHistogram = nullptr;
  if (ShouldAddHistogramValues) {
    vHighResHistogram = &vHigherResImage.GetHistogramBuilderForBlock(aHigherResBlockIndex[0], aHigherResBlockIndex[1], aHigherResBlockIndex[2]);
  }

  bpSize vSmallIndexZ = 0;
  for (bpSize vLargeIndexZ = 0; vLargeIndexZ < vLargeRegionZ; vLargeIndexZ += StrideZ, ++vSmallIndexZ) {
    bpSize vLargeOffsetZ = vLargeIndexZ * vLargeBlockSizeXY;
    bpSize vSmallOffsetZT = vSmallIndexZ * vSmallBlockSizeX * vSmallBlockSizeY;

    bpSize vSmallIndexY = 0;
    for (bpSize vLargeIndexY = 0; vLargeIndexY < vLargeRegionY; vLargeIndexY += StrideY, ++vSmallIndexY) {
      bpSize vLargeOffsetYZT = vLargeIndexY * vLargeBlockSizeX + vLargeOffsetZ;
      bpSize vSmallOffsetYZT = vSmallIndexY * vSmallBlockSizeX + vSmallOffsetZT;
      bpSize vSmallIndexX = 0;
      for (bpSize vLargeIndexX = 0; vLargeIndexX < vLargeRegionX; vLargeIndexX += StrideX, ++vSmallIndexX) {
        bpSize vLargeOffset = vLargeIndexX + vLargeOffsetYZT;
        bpSize vSmallOffset = vSmallIndexX + vSmallOffsetYZT;

        bpFloat vSumOfLarge = 0;  // or TDataType?
        for (bpSize vOffsetZ = 0; vOffsetZ < StrideZ; ++vOffsetZ) {
          bpSize vLargeOffsetStrideZ = vLargeOffset + vOffsetZ * vLargeBlockSizeXY;
          for (bpSize vOffsetY = 0; vOffsetY < StrideY; ++vOffsetY) {
            bpSize vLargeOffsetStrideYZ = vLargeOffsetStrideZ + vOffsetY * vLargeBlockSizeX;
            for (bpSize vOffsetX = 0; vOffsetX < StrideX; ++vOffsetX) {
              bpSize vLargeOffsetStrideXYZ = vLargeOffsetStrideYZ + vOffsetX;

              TDataType vValue = vData[vLargeOffsetStrideXYZ];
              vSumOfLarge += static_cast<bpFloat>(vValue);
              if (ShouldAddHistogramValues) {
                vHighResHistogram->AddValue(vValue);
              }
            }
          }
        }
        vSumOfLarge /= StrideZ * StrideY * StrideX;
        vResult[vSmallOffset] = static_cast<TDataType>(vSumOfLarge);
      }
    }
  }

  OnCopiedData(aIndexT, aIndexC, aHigherResBlockIndex, vSmallIndexR);

  if (!ShouldAddHistogramValues) {
    return;
  }

  bool vIsLastX = vLargeMaxX < aLargeIndexMax[0];
  bool vIsLastY = vLargeMaxY < aLargeIndexMax[1];
  bool vIsLastZ = vLargeMaxZ < aLargeIndexMax[2];

  if (vIsLastX) {
    bpSize vLargeOffset0 = vLargeRegionX;
    for (bpSize vLargeIndexZ = 0; vLargeIndexZ < vLargeRegionZ; ++vLargeIndexZ) {
      bpSize vLargeOffset1 = vLargeOffset0 + vLargeIndexZ * vLargeBlockSizeXY;
      for (bpSize vLargeIndexY = 0; vLargeIndexY < vLargeRegionY; ++vLargeIndexY) {
        vHighResHistogram->AddValue(vData[vLargeOffset1 + vLargeIndexY * vLargeBlockSizeX]);
      }
    }
  }

  if (vIsLastY) {
    bpSize vLargeOffset0 = vLargeRegionY * vLargeBlockSizeX;
    bpSize vExtendedLargeRegionX = aLargeIndexMax[0] - aLargeIndexMin[0];
    for (bpSize vLargeIndexZ = 0; vLargeIndexZ < vLargeRegionZ; ++vLargeIndexZ) {
      bpSize vLargeOffset1 = vLargeOffset0 + vLargeIndexZ * vLargeBlockSizeXY;
      for (bpSize vLargeIndexX = 0; vLargeIndexX < vExtendedLargeRegionX; ++vLargeIndexX) {
        vHighResHistogram->AddValue(vData[vLargeOffset1 + vLargeIndexX]);
      }
    }
  }

  if (vIsLastZ) {
    bpSize vLargeOffset0 = vLargeRegionZ * vLargeBlockSizeXY;
    bpSize vExtendedLargeRegionX = aLargeIndexMax[0] - aLargeIndexMin[0];
    bpSize vExtendedLargeRegionY = aLargeIndexMax[1] - aLargeIndexMin[1];
    for (bpSize vLargeIndexY = 0; vLargeIndexY < vExtendedLargeRegionY; ++vLargeIndexY) {
      bpSize vLargeOffset1 = vLargeOffset0 + vLargeIndexY * vLargeBlockSizeX;
      for (bpSize vLargeIndexX = 0; vLargeIndexX < vExtendedLargeRegionX; ++vLargeIndexX) {
        vHighResHistogram->AddValue(vData[vLargeOffset1 + vLargeIndexX]);
      }
    }
  }
}


template<typename TDataType>
void bpMultiresolutionImsImage<TDataType>::AddHistogramValues(bpImsImage3D<TDataType>& aImage, const bpVec3& aHigherResBlockIndex, const bpConstMemoryBlock<TDataType>& aBlockData)
{
  bpVec3 vMemoryBlockSize = aImage.GetMemoryBlockSize();
  const TDataType* vData = aBlockData.GetData();

  //minIndex is first voxel in block, maxIndex is last (+1) in block or last voxel (+1) of image
  bpVec3 aLargeIndexMin;
  bpVec3 aLargeIndexMax;
  for (bpSize vDim = 0; vDim < 3; vDim++) {
    aLargeIndexMin[vDim] = aHigherResBlockIndex[vDim] * vMemoryBlockSize[vDim];
    aLargeIndexMax[vDim] = std::min((aHigherResBlockIndex[vDim] + 1) * vMemoryBlockSize[vDim], aImage.GetImageSize()[vDim]);
  }

  if (aLargeIndexMin[0] >= aLargeIndexMax[0] || aLargeIndexMin[1] >= aLargeIndexMax[1] || aLargeIndexMin[2] >= aLargeIndexMax[2]) {
    return;
  }

  auto& vHistogram = aImage.GetHistogramBuilderForBlock(aHigherResBlockIndex[0], aHigherResBlockIndex[1], aHigherResBlockIndex[2]);

  bpSize vLargeBlockSizeX = vMemoryBlockSize[0];
  bpSize vLargeBlockSizeXY = vLargeBlockSizeX * vMemoryBlockSize[1];

  bpSize vExtendedLargeRegionX = aLargeIndexMax[0] - aLargeIndexMin[0];
  bpSize vExtendedLargeRegionY = aLargeIndexMax[1] - aLargeIndexMin[1];
  bpSize vExtendedLargeRegionZ = aLargeIndexMax[2] - aLargeIndexMin[2];
  for (bpSize vLargeIndexZ = 0; vLargeIndexZ < vExtendedLargeRegionZ; ++vLargeIndexZ) {
    for (bpSize vLargeIndexY = 0; vLargeIndexY < vExtendedLargeRegionY; ++vLargeIndexY) {
      bpSize vLargeOffset = vLargeIndexZ * vLargeBlockSizeXY + vLargeIndexY * vLargeBlockSizeX;
      for (bpSize vLargeIndexX = 0; vLargeIndexX < vExtendedLargeRegionX; ++vLargeIndexX) {
        vHistogram.AddValue(vData[vLargeOffset + vLargeIndexX]);
      }
    }
  }
}


template<typename TDataType>
std::vector<bpVec3> bpMultiresolutionImsImage<TDataType>::ComputeMemoryBlockSizes(const std::vector<bpVec3>& aResolutionSizes, bpSize aSizeT)
{
  bpSize vImageBlockSize = 1024 * 1024;
  bpSize vImageElementSize = sizeof(TDataType);
  return GetOptimalBlockSizes(vImageBlockSize / vImageElementSize, aResolutionSizes, aSizeT);
}


template<typename TDataType>
std::vector<bpVec3> bpMultiresolutionImsImage<TDataType>::GetOptimalImagePyramid(const bpVec3& aImageSize, bool aReduceZ)
{
  bpSize vMaxImagSize = 1024 * 1024;
  bpSize vImageElementSize = sizeof(TDataType);
  return ::GetOptimalImagePyramid(vMaxImagSize / vImageElementSize, aImageSize, aReduceZ);
}


template class bpMultiresolutionImsImage<bpUInt8>;
template class bpMultiresolutionImsImage<bpUInt16>;
template class bpMultiresolutionImsImage<bpUInt32>;
template class bpMultiresolutionImsImage<bpFloat>;
