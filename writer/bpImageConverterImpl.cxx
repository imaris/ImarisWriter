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
#include "bpImageConverterImpl.h"
#include "bpDeriche.h"
#include "bpWriterFactoryHDF5.h"
#include "bpWriterFactoryCompressor.h"


using namespace bpConverterTypes;


template<typename TDataType>
bpImageConverterImpl<TDataType>::bpImageConverterImpl(
  tDataType aDataType, tDimensionSequence5D aBlockDataDimensionSequence, const tSize5D& aImageSize, const tSize5D& aSample,
  const tSize5D& aFileBlockSize, const bpString& aOutputFile, const cOptions& aOptions,
  const bpString& aApplicationName, const bpString& aApplicationVersion, tProgressCallback aProgressCallback)
  : mBlockDataDimensionSequence(aBlockDataDimensionSequence), mImageSize(aImageSize), mFileBlockSize(aFileBlockSize),
    mSample(aSample), mMinLimit(InitMapWithConstant(0)), mMaxLimit(aImageSize), mNumberOfBlocks(InitMapWithConstant(1)),
    mApplicationName(aApplicationName),
    mApplicationVersion(aApplicationVersion),
    mMultiresolutionImage(
    Div(aImageSize[X], aSample[X]), Div(aImageSize[Y], aSample[Y]), Div(aImageSize[Z], aSample[Z]),
    Div(aImageSize[C], aSample[C]), Div(aImageSize[T], aSample[T]), aDataType,
    { aFileBlockSize[X], aFileBlockSize[Y] }, { aSample[X], aSample[Y] },
    std::make_shared<bpWriterFactoryCompressor>(std::make_shared<bpWriterFactoryHDF5>(), aOptions.mNumberOfThreads, aOptions.mEnableLogProgress ? std::move(aProgressCallback) : tProgressCallback()),
    aOutputFile, aOptions.mCompressionAlgorithmType, aOptions.mThumbnailSizeXY, aOptions.mForceFileBlockSizeZ1, aOptions.mNumberOfThreads, aOptions.mDisablePyramid)
{
  mIsFlipped[0] = aOptions.mFlipDimensionXYZ[0];
  mIsFlipped[1] = aOptions.mFlipDimensionXYZ[1];
  mIsFlipped[2] = aOptions.mFlipDimensionXYZ[2];

  bpSize vNumberOfBlocks = 1;
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    Dimension vDim = mBlockDataDimensionSequence[vDimIndex];
    mNumberOfBlocks[vDim] = Div(mImageSize[vDim], mFileBlockSize[vDim]);
    vNumberOfBlocks *= mNumberOfBlocks[vDim];
  }
  mBlockCopied.resize(vNumberOfBlocks, false);
}


template<typename TDataType>
bpImageConverterImpl<TDataType>::~bpImageConverterImpl()
{
}


template<typename TDataType>
tSize5D bpImageConverterImpl<TDataType>::InitMapWithConstant(bpSize aValue)
{
  return tSize5D(X, aValue, Y, aValue, Z, aValue, C, aValue, T, aValue);
}


template<typename TDataType>
bpSize bpImageConverterImpl<TDataType>::Div(bpSize aNum, bpSize aDiv)
{
  return (aNum + aDiv - 1) / aDiv;
}


template<typename TDataType>
bool bpImageConverterImpl<TDataType>::NeedCopyBlock(const tIndex5D& aBlockIndex) const
{
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    bpSize vBegin;
    bpSize vEnd;
    Dimension vDim = mBlockDataDimensionSequence[vDimIndex];
    GetFullRangeOfFileBlock(aBlockIndex[vDim], vDim, vBegin, vEnd);
    if (vEnd <= vBegin) {
      return false;
    }
  }
  return true;
}


template<typename TDataType>
void bpImageConverterImpl<TDataType>::CopyBlock(const TDataType* aFileDataBlock, const tIndex5D& aBlockIndex)
{
  if (!aFileDataBlock) {
    return;
  }

  bpSize vBlockIndex = GetFileBlockIndex1D(aBlockIndex);
  if (mBlockCopied[vBlockIndex]) {
    throw bpError("Block data has already been copied");
  }
  mBlockCopied[vBlockIndex] = true;

  CopyFileBlockToImage(aBlockIndex, aFileDataBlock);
}


template<typename TDataType>
bpSize bpImageConverterImpl<TDataType>::GetFileBlockIndex1D(const bpConverterTypes::tIndex5D& aBlockIndex) const
{
  bpSize vBlockIndex = 0;
  bpSize vNumberOfBlocks = 1;
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    Dimension vDim = mBlockDataDimensionSequence[vDimIndex];
    vBlockIndex += aBlockIndex[vDim] * vNumberOfBlocks;
    vNumberOfBlocks *= mNumberOfBlocks[vDim];
  }
  return vBlockIndex;
}


template<typename TDataType>
void bpImageConverterImpl<TDataType>::Finish(
  const cImageExtent& aImageExtent,
  const tParameters& aParameters,
  const tTimeInfoVector& aTimeInfoPerTimePoint,
  const tColorInfoVector& aColorInfoPerChannel,
  bool aAutoAdjustColorRange)
{
  mMultiresolutionImage.FinishWriteDataBlocks();
  tColorInfoVector vColorInfoPerChannel(aColorInfoPerChannel);
  if (aAutoAdjustColorRange) {
    AdjustColorRange(vColorInfoPerChannel);
  }
  mMultiresolutionImage.WriteMetadata(mApplicationName, mApplicationVersion, aImageExtent, aParameters, aTimeInfoPerTimePoint, vColorInfoPerChannel);
}


template <typename TDataType>
bpHistogram bpImageConverterImpl<TDataType>::GetConversionImageHistogram(bpSize aIndexC) const
{
  return mMultiresolutionImage.GetChannelHistogram(aIndexC);
}


template <typename TDataType>
void bpImageConverterImpl<TDataType>::GetRangeOfFileBlock(bpSize aFileBlockIndex, Dimension aDimension,  bpSize& aBeginInBlock, bpSize& aEndInBlock) const
{
  Dimension vDim = aDimension;
  bpSize vBegin = aFileBlockIndex * mFileBlockSize[vDim];
  if (vBegin > mMinLimit[vDim]) {
    bpSize vOffset = vBegin - mMinLimit[vDim];
    bpSize vCount = (vOffset + mSample[vDim] - 1) / mSample[vDim];
    aBeginInBlock = vCount * mSample[vDim] - vOffset;
  }
  else {
    aBeginInBlock = mMinLimit[vDim] - vBegin;
  }
  bpSize vEnd = (aFileBlockIndex + 1) * mFileBlockSize[vDim];
  if (vEnd <= mMaxLimit[vDim]) {
    aEndInBlock = mFileBlockSize[vDim];
  }
  else {
    aEndInBlock = mMaxLimit[vDim] - vBegin;
  }
}


template <typename TDataType>
void bpImageConverterImpl<TDataType>::GetFullRangeOfFileBlock(bpSize aFileBlockIndex, Dimension aDimension, bpSize& aBegin, bpSize& aEnd) const
{
  bpSize vBeginInBlock;
  bpSize vEndInBlock;
  Dimension vDim = aDimension;
  GetRangeOfFileBlock(aFileBlockIndex, vDim, vBeginInBlock, vEndInBlock);

  bpSize vBegin = aFileBlockIndex * mFileBlockSize[vDim] + vBeginInBlock;
  aBegin = (vBegin - mMinLimit[vDim]) / mSample[vDim];
  bpSize vEnd = (aFileBlockIndex + 1) * mFileBlockSize[vDim];
  aEnd = (vEnd - mMinLimit[vDim] + mSample[vDim] - 1) / mSample[vDim];
}


template<typename TDataType>
void bpImageConverterImpl<TDataType>::CopyFileBlockToImage(const tSize5D& aFileBlockIndices, const TDataType* aDataBlock)
{
  bpSize vDimNrX;
  bpSize vDimNrY;
  bpSize vDimNrZ;

  // find position of DimX and DimY in BlockDataDimensionSequence
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    if (mBlockDataDimensionSequence[vDimIndex] == X) {
      vDimNrX = vDimIndex;
    }
    else if (mBlockDataDimensionSequence[vDimIndex] == Y) {
      vDimNrY = vDimIndex;
    }
    else if (mBlockDataDimensionSequence[vDimIndex] == Z) {
      vDimNrZ = vDimIndex;
    }
  }

  bpSize vFileBlockSize[5];
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    Dimension vDimension = mBlockDataDimensionSequence[vDimIndex];
    bpSize vBlockSize = mFileBlockSize[vDimension];
    vFileBlockSize[vDimIndex] = vBlockSize;
  }

  bpSize vFileBlockIndices[5];
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    Dimension vDimension = mBlockDataDimensionSequence[vDimIndex];
    bpSize vBlockIndex = aFileBlockIndices[vDimension];
    vFileBlockIndices[vDimIndex] = vBlockIndex;
  }

  bpSize vDimensionMap[5];
  for (bpSize vDimIndex = 0; vDimIndex < 5; ++vDimIndex) {
    Dimension vDimension = mBlockDataDimensionSequence[vDimIndex];
    switch (vDimension) {
    case X:
      vDimensionMap[vDimIndex] = 0;
      continue;
    case Y:
      vDimensionMap[vDimIndex] = 1;
      continue;
    case Z:
      vDimensionMap[vDimIndex] = 2;
      continue;
    case C:
      vDimensionMap[vDimIndex] = 3;
      continue;
    case T:
      vDimensionMap[vDimIndex] = 4;
      continue;
    }
  }

  // needs to be in order of memoryBlockDataDimensionSequence
  bpSize vMemSample[5];
  vMemSample[0] = mSample[X];
  vMemSample[1] = mSample[Y];
  vMemSample[2] = mSample[Z];
  vMemSample[3] = mSample[C];
  vMemSample[4] = mSample[T];

  bpSize vMemMinLimits[5];
  vMemMinLimits[0] = mMinLimit[X];
  vMemMinLimits[1] = mMinLimit[Y];
  vMemMinLimits[2] = mMinLimit[Z];
  vMemMinLimits[3] = mMinLimit[C];
  vMemMinLimits[4] = mMinLimit[T];

  bpSize vMemMaxLimits[5];
  vMemMaxLimits[0] = mMaxLimit[X];
  vMemMaxLimits[1] = mMaxLimit[Y];
  vMemMaxLimits[2] = mMaxLimit[Z];
  vMemMaxLimits[3] = mMaxLimit[C];
  vMemMaxLimits[4] = mMaxLimit[T];

  bpSize vDimWeight[5];
  bpSize vBeginInBlock[5]; // Offset in the block to the beginning of the data for the nth dimension (min is 0 and max is blocksize - 1)
  bpSize vEndInBlock[5]; // Offset in the block to the end of the data (of the block) for the nth dimension (min is 1 and max is blocksize)
  bpSize vNonXYDim[3];
  bpSize vNonXYDimIndex = 0;

  bpSize vDimX = vDimNrX;
  bpSize vDimY = vDimNrY;
  for (bpSize vDim = 0; vDim < 5; ++vDim) {
    GetRangeOfFileBlock(vFileBlockIndices[vDim], mBlockDataDimensionSequence[vDim], vBeginInBlock[vDim], vEndInBlock[vDim]);
    if (vBeginInBlock[vDim] >= vEndInBlock[vDim]) {
      throw bpError("Block data has no overlap with result image");
    }

    vDimWeight[vDim] = vDim > 0 ? vFileBlockSize[vDim - 1] * vDimWeight[vDim - 1] : 1;

    if (vDim != vDimX && vDim != vDimY) {
      vNonXYDim[vNonXYDimIndex++] = vDim;
    }
  }

  bpSize vDim2 = vNonXYDim[2];
  bpSize vD2 = vDimensionMap[vDim2];
  bpSize vDim1 = vNonXYDim[1];
  bpSize vD1 = vDimensionMap[vDim1];
  bpSize vDim0 = vNonXYDim[0];
  bpSize vD0 = vDimensionMap[vDim0];

  bpSize vBeginX;
  bpSize vEndX;
  GetFullRangeOfFileBlock(vFileBlockIndices[vDimX], mBlockDataDimensionSequence[vDimX], vBeginX, vEndX);

  bpSize vBeginY;
  bpSize vEndY;
  GetFullRangeOfFileBlock(vFileBlockIndices[vDimY], mBlockDataDimensionSequence[vDimY], vBeginY, vEndY);

  bpVec2 vBegin{
    mIsFlipped[0] ? mImageSize[X] - vEndX : vBeginX,
    mIsFlipped[1] ? mImageSize[Y] - vEndY : vBeginY
  };
  bpVec2 vEnd{
    mIsFlipped[0] ? mImageSize[X] - vBeginX : vEndX,
    mIsFlipped[1] ? mImageSize[Y] - vBeginY : vEndY
  };

  bpSize vDimWeightX = vDimWeight[vDimX];
  bpSize vDimWeightY = vDimWeight[vDimY];
  bpSize vStepX = vMemSample[0] * vDimWeightX;
  bpSize vStepY = vMemSample[1] * vDimWeightY;

  bpSize vSizeX = vEndX - vBeginX;
  bpSize vSizeY = vEndY - vBeginY;
  bpSize vSizeXY = vSizeX * vSizeY;

  bool vIsFlippedX = mIsFlipped[0];
  bool vIsFlippedY = mIsFlipped[1];

  bool vCanRawCopy =
    vDimX == 0 && vDimY == 1 &&
    vMemSample[0] == 1 && vMemSample[1] == 1 &&
    vBeginInBlock[vDimX] == 0 && vEndInBlock[vDimX] == vFileBlockSize[vDimX] &&
    !vIsFlippedX && !vIsFlippedY;

  bpSize vDimOffset[3];
  bpSize vImageIndex[5]; // Image global index for dimension X, Y, Z, C, T (the order is 0, 1, 2, 3, 4)

  for (bpSize vIndex2 = vBeginInBlock[vDim2]; vIndex2 < vEndInBlock[vDim2]; vIndex2 += vMemSample[vD2]) {
    bpSize vImageIndex2 = vFileBlockIndices[vDim2] * vFileBlockSize[vDim2] + vIndex2;
    vImageIndex[vD2] = (vImageIndex2 - vMemMinLimits[vD2]) / vMemSample[vD2];
    vDimOffset[2] = vDimWeight[vDim2] * vIndex2;

    for (bpSize vIndex1 = vBeginInBlock[vDim1]; vIndex1 < vEndInBlock[vDim1]; vIndex1 += vMemSample[vD1]) {
      bpSize vImageIndex1 = vFileBlockIndices[vDim1] * vFileBlockSize[vDim1] + vIndex1;
      vImageIndex[vD1] = (vImageIndex1 - vMemMinLimits[vD1]) / vMemSample[vD1];
      vDimOffset[1] = vDimWeight[vDim1] * vIndex1;

      for (bpSize vIndex0 = vBeginInBlock[vDim0]; vIndex0 < vEndInBlock[vDim0]; vIndex0 += vMemSample[vD0]) {
        bpSize vImageIndex0 = vFileBlockIndices[vDim0] * vFileBlockSize[vDim0] + vIndex0;
        vImageIndex[vD0] = (vImageIndex0 - vMemMinLimits[vD0]) / vMemSample[vD0];
        vDimOffset[0] = vDimWeight[vDim0] * vIndex0;

        bpSize vDataOffset = vDimOffset[2] + vDimOffset[1] + vDimOffset[0] + vDimWeightY * vBeginInBlock[vDimY] + vDimWeightX * vBeginInBlock[vDimX];
        const TDataType* vDataBlock = aDataBlock + vDataOffset;

        if (!vCanRawCopy) {
          // start read there but with different step size
          mTempBuffer.resize(vSizeXY);
          TDataType* vBuffer = mTempBuffer.data();
          for (bpSize vIndexY = 0; vIndexY < vSizeY; ++vIndexY) {
            const TDataType* vSource = vDataBlock + (vIndexY * vStepY);
            TDataType* vPtr = vBuffer + ((!vIsFlippedY ? vIndexY : vSizeY - vIndexY - 1) * vSizeX);
            if (!vIsFlippedX) {
              if (vStepX == 1) {
                std::copy(vSource, vSource + vSizeX, vPtr);
              }
              else {
                for (bpSize vIndexX = 0; vIndexX < vSizeX; ++vIndexX) {
                  *vPtr = *vSource;
                  ++vPtr;
                  vSource += vStepX;
                }
              }
            }
            else {
              vPtr += vSizeX - 1;
              for (bpSize vIndexX = 0; vIndexX < vSizeX; ++vIndexX) {
                *vPtr = *vSource;
                --vPtr;
                vSource += vStepX;
              }
            }
          }
          vDataBlock = vBuffer;
        }

        vImageIndex[2] = mIsFlipped[2] ? mImageSize[Z] - vImageIndex[2] - 1 : vImageIndex[2];

        // The indices here are memoryIndices for each dim, not block indices!
        // T,C,Z are only one memoryIndex (for each dim), X,Y can contain span (vBegin, vEnd) over a range of memoryIndices (for each dim)
        mMultiresolutionImage.CopyData(vImageIndex[4], vImageIndex[3], vImageIndex[2], { vFileBlockIndices[vDimX], vFileBlockIndices[vDimY] }, vDataBlock, 0);
      }
    }
  }
}

template <typename TDataType>
void bpImageConverterImpl<TDataType>::AdjustColorRange(std::vector<cColorInfo>& aColorInfo) const
{
  for (bpSize vIndexC = 0; vIndexC < aColorInfo.size(); vIndexC++) {
    const bpHistogram& vHistogram = GetConversionImageHistogram(vIndexC);
    bpSize vSize = vHistogram.GetNumberOfBins();
    std::vector<bpFloat> vBins = GetFilteredBins(vHistogram, 5.0f * vSize / 256);
    bpFloat vPrevious = -1.0f;
    bpSize vFirstModeBinId = 0;
    for (bpSize vBinId = 0; vBinId < vSize - 1; ++vBinId) {
      if (vBins[vBinId] > vPrevious && vBins[vBinId] > vBins[vBinId + 1]) {
        vFirstModeBinId = vBinId;
        break;
      }
      vPrevious = vBins[vBinId];
    }

    bpSize vHighPercentileBinId = vSize - 1;
    bpUInt64 vTotalVolumeSum = 0;
    for (bpSize vBinId = 0; vBinId < vSize; vBinId++) {
      vTotalVolumeSum += vBins[vBinId];
    }
    bpUInt64 vVolumeSum = 0;
    for (bpSize vBinId = 0; vBinId < vSize; vBinId++) {
      vVolumeSum += vBins[vBinId];
      if (vVolumeSum / (bpFloat)vTotalVolumeSum > 0.998f) {
        vHighPercentileBinId = vBinId;
        break;
      }
    }

    bpFloat vMin = vHistogram.GetBinValue(vFirstModeBinId);
    bpFloat vMax = vHistogram.GetBinValue(vHighPercentileBinId);
    vMax = vMax + (vMax - vMin) * 0.2f;
    vMax = static_cast<bpInt32>(std::min(vMax, vHistogram.GetMax()));

    aColorInfo[vIndexC].mRangeMin = vMin;
    aColorInfo[vIndexC].mRangeMax = vMax;
  }
}

template <typename TDataType>
std::vector<bpFloat> bpImageConverterImpl<TDataType>::GetFilteredBins(const bpHistogram& aHistogram, bpFloat aFilterWidth)
{
  bpSize vSize = aHistogram.GetNumberOfBins();

  std::vector<bpFloat> vOriginal(vSize);
  for (bpSize vBinId = 0; vBinId < vSize; vBinId++) {
    vOriginal[vBinId] = static_cast<bpFloat>(aHistogram.GetCount(vBinId));
  }
  std::vector<bpFloat> vFiltered(vSize);
  bpDeriche::FilterGauss(vOriginal.data(), vFiltered.data(), vSize, aFilterWidth);
  return vFiltered;
}


template class bpImageConverterImpl<bpUInt8>;
template class bpImageConverterImpl<bpUInt16>;
template class bpImageConverterImpl<bpUInt32>;
template class bpImageConverterImpl<bpFloat>;
