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
#ifndef __BP_IMAGE_CONVERTER_IMPL_
#define __BP_IMAGE_CONVERTER_IMPL_

#include "../interface/bpImageConverterInterface.h"
#include "bpMultiresolutionImsImage.h"
#include "bpWriterFactory.h"


template<typename TDataType>
class bpImageConverterImpl : public bpImageConverterInterface<TDataType>
{
public:
  bpImageConverterImpl(
    bpConverterTypes::tDataType aDataType, bpConverterTypes::tDimensionSequence5D aDimensionSequence,
    const bpConverterTypes::tSize5D& aImageSize, const bpConverterTypes::tSize5D& aSample, const bpConverterTypes::tSize5D& aFileBlockSize,
    const bpString& aOutputFile, const bpConverterTypes::cOptions& aOptions,
    const bpString& aApplicationName, const bpString& aApplicationVersion, bpConverterTypes::tProgressCallback aProgressCallback);

  ~bpImageConverterImpl();

  bool NeedCopyBlock(const bpConverterTypes::tIndex5D& aBlockIndex) const override;

  void CopyBlock(const TDataType* aFileDataBlock, const bpConverterTypes::tIndex5D& aBlockIndex) override;

  void Finish(
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
    bool aAutoAdjustColorRange) override;

private:
  using tSize5D = bpConverterTypes::tSize5D;

  static tSize5D InitMapWithConstant(bpSize aValue);
  static bpSize Div(bpSize aNum, bpSize aDiv);

  bpSize GetFileBlockIndex1D(const bpConverterTypes::tIndex5D& aBlockIndex) const;

  void GetRangeOfFileBlock(bpSize aFileBlockIndex, bpConverterTypes::Dimension aDimension, bpSize& aBeginInBlock, bpSize& aEndInBlock) const;
  void GetFullRangeOfFileBlock(bpSize aFileBlockIndex, bpConverterTypes::Dimension aDimension, bpSize& aBegin, bpSize& aEnd) const;
  void CopyFileBlockToImage(const tSize5D& aFileBlockIndices, const TDataType* aDataBlock);
  bpHistogram GetConversionImageHistogram(bpSize aIndexC) const;
  void AdjustColorRange(std::vector<bpConverterTypes::cColorInfo>& aColorInfo) const;
  static std::vector<bpFloat> GetFilteredBins(const bpHistogram& aHistogram, bpFloat aFilterWidth);

  bpString mApplicationName;
  bpString mApplicationVersion;

  bpConverterTypes::tDimensionSequence5D mBlockDataDimensionSequence;
  tSize5D mImageSize;
  tSize5D mFileBlockSize;
  tSize5D mNumberOfBlocks;

  std::vector<bool> mBlockCopied;

  tSize5D mSample;
  tSize5D mMinLimit;
  tSize5D mMaxLimit;
  bool mIsFlipped[3];  // for X,Y,Z dim

  bpMultiresolutionImsImage<TDataType> mMultiresolutionImage;

  std::vector<TDataType> mTempBuffer;
  std::map<bpConverterTypes::Dimension, bpSize> mDimensionIndicesFromSequence;
};

#endif // __BP_IMAGE_CONVERTER__
