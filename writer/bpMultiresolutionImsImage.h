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
#ifndef __BP_MULTIRESOLUTION_IMS_IMAGE__
#define __BP_MULTIRESOLUTION_IMS_IMAGE__

#include "bpImsImage5D.h"
#include "bpWriterFactory.h"
#include "../interface/bpConverterTypes.h"
#include "bpMemoryManager.h"
#include "bpThumbnailBuilder.h"

#include <functional>
#include <atomic>


class bpThreadPool;

/**
* Ims image representing all resolution levels.
*/
template<typename TDataType>
class bpMultiresolutionImsImage
{
public:
  bpMultiresolutionImsImage(
    bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ, bpSize aSizeC, bpSize aSizeT, bpConverterTypes::tDataType aDataType,
    const bpVec2& aCopyBlockSizeXY, const bpVec2& aSampleXY,
    const bpSharedPtr<bpWriterFactory>& aWriterFactory,
    const bpString& aOutputFile, bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType,
    bpSize aThumbnailSizeXY, bool aForceFileBlockSizeZ1, bpSize aNumberOfThreads);

  bpMultiresolutionImsImage(const bpMultiresolutionImsImage&) = delete;
  bpMultiresolutionImsImage& operator=(const bpMultiresolutionImsImage&) = delete;

  bpMultiresolutionImsImage(bpMultiresolutionImsImage&&) = default;

  ~bpMultiresolutionImsImage();

  void CopyData(bpSize aIndexT, bpSize aIndexC, bpSize aIndexZ, const bpVec2& aBlockIndexXY, const TDataType* aDataBlockXY, bpSize aIndexR = 0);

  void FinishWriteDataBlocks();

  void WriteMetadata(
    const bpString& aApplicationName,
    const bpString& aApplicationVersion,
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel);

  bpHistogram GetChannelHistogram(bpSize aIndexC) const;

private:
  bpSize GetMemoryBlockIndex(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aIndexZ, bpSize aIndexC, bpSize aIndexT, bpSize aIndexR) const;

  void InitCopyBlocksLeft(bpSize aIndexR);

  void OnCopiedData(bpSize aIndexT, bpSize aIndexC, const bpVec3& aBlockIndexY, bpSize aIndexR);
  void OnCopiedDataImpl(bpSize aIndexT, bpSize aIndexC, const bpVec3& aBlockIndexXYZ, bpSize aIndexR);

  static std::vector<bpVec3> GetOptimalImagePyramid(const bpVec3& aImageSize, bool aReduceZ);
  static std::vector<bpVec3> ComputeMemoryBlockSizes(const std::vector<bpVec3>& aResolutionSizes, bpSize aSizeT);

  bpVec3 GetStrideToNextResolution(bpSize aIndexR) const;
  void InitLowResBlock(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC);
  void ResampleBlock(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData);

  template<bpSize Dim = 0, bpSize... Stride>
  std::enable_if_t<(Dim < 3)> ResampleBlockD(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData, const bpVec3& aStride);

  template<bpSize Dim, bpSize... Stride>
  std::enable_if_t<(Dim == 3)> ResampleBlockD(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData, const bpVec3& aStride);

  template<bpSize StrideX, bpSize StrideY, bpSize StrideZ, bool ShouldAddHistogramValues>
  void ResampleBlockT(const bpVec3& aHigherResBlockIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpConstMemoryBlock<TDataType>& aBlockData);

  void AddHistogramValues(bpImsImage3D<TDataType>& aImage, const bpVec3& aHigherResBlockIndex, const bpConstMemoryBlock<TDataType>& aBlockData);

  bpSharedPtr<bpThreadPool> GetHistogramThread(bpSize aIndexR, bpSize aIndexT, bpSize aIndexC, const bpVec3& aBlockIndex) const;

  // one image for each resolution level
  std::vector<bpImsImage5D<TDataType>> mImages;
  std::vector<std::vector<bpSize>> mCopyBlocksLeft;

  const bpVec2 mCopyBlockSizeXY;
  const bpVec2 mSampleXY;

  bpSharedPtr<bpWriter> mWriter;
  bpSharedPtr<bpThumbnailBuilder<TDataType>> mThumbnailBuilder;

  bpSharedPtr<bpThreadPool> mComputeThread;
  std::vector<bpSharedPtr<bpThreadPool>> mHistogramThreads;

  std::atomic_size_t mResampleCount;

  bpSize mMaxRunningJobsPerThread;
};

#endif // __BP_MULTIRESOLUTION_IMS_IMAGE__


