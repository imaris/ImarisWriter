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
#define EXPORT_IMARISWRITER_DLL
#include "../interfaceC/bpImageConverterInterfaceC.h"

#include "../interface/bpImageConverter.h"


#include <mutex>
#include <thread>


class bpThreadLocalException
{
public:
  const bpString& GetThisThreadException()
  {
    tLock vLock(mMutex);
    return ThisThreadException();
  }

  void SetThisThreadException(bpString aExceptionMessage)
  {
    tLock vLock(mMutex);
    ThisThreadException() = std::move(aExceptionMessage);
  }

private:
  bpString& ThisThreadException()
  {
    return mExceptionMessages[std::this_thread::get_id()];
  }

  using tMutex = std::mutex;
  using tLock = std::lock_guard<tMutex>;

  tMutex mMutex;
  std::map<std::thread::id, bpString> mExceptionMessages;
};


struct bpImageConverterC
{
  template<typename... Args>
  void Init(bpConverterTypesC_DataType aDataType, Args&&... aArgs)
  {
    SelectImplByType<cCreateImpl>(aDataType, std::forward<Args>(aArgs)...);
  }

  template<typename... Args>
  bool NeedCopyBlock(Args&&... aArgs)
  {
    bool vResult = false;
    SelectImpl<cNeedCopyBlockImpl, bool&>(vResult, std::forward<Args>(aArgs)...);
    return vResult;
  }

  template<typename... Args>
  void CopyBlock(Args&&... aArgs)
  {
    SelectImpl<cCopyBlockImpl>(std::forward<Args>(aArgs)...);
  }

  template<typename... Args>
  void Finish(Args&&... aArgs)
  {
    SelectImpl<cFinishImpl>(std::forward<Args>(aArgs)...);
  }

  template<typename Func>
  void TryExecute(const Func& aFunc)
  {
    TryExecuteImpl(aFunc);
  }

  const char* GetLastException()
  {
    const bpString& vExceptionMessage = mException.GetThisThreadException();
    if (vExceptionMessage.empty()) {
      return nullptr;
    }
    return vExceptionMessage.c_str();
  }

private:
  template<typename Func>
  void TryExecuteImpl(const Func& aFunc)
  {
    bpString vExceptionMessage;
    try {
      aFunc();
    }
    catch (const std::exception& vException) {
      vExceptionMessage = vException.what();
      if (vExceptionMessage.empty()) {
        vExceptionMessage = "Fatal error: Empty exception.";
      }
    }
    catch (const std::string& vException) {
      vExceptionMessage = vException;
      if (vExceptionMessage.empty()) {
        vExceptionMessage = "Fatal error: Empty exception.";
      }
    }
    catch (const char* vException) {
      vExceptionMessage = vException;
      if (vExceptionMessage.empty()) {
        vExceptionMessage = "Fatal error: Empty exception.";
      }
    }
    catch (...) {
      vExceptionMessage = "Fatal error: Unknown exception.";
    }
    mException.SetThisThreadException(vExceptionMessage);
  }

  template<class cImpl, typename... Args>
  void SelectImplByType(bpConverterTypesC_DataType aDataType, Args&&... aArgs)
  {
    if (aDataType == bpConverterTypesC_UInt8Type) {
      Do<cImpl>(mUInt8, std::forward<Args>(aArgs)...);
    }
    else if (aDataType == bpConverterTypesC_UInt16Type) {
      Do<cImpl>(mUInt16, std::forward<Args>(aArgs)...);
    }
    else if (aDataType == bpConverterTypesC_UInt32Type) {
      Do<cImpl>(mUInt32, std::forward<Args>(aArgs)...);
    }
    else if (aDataType == bpConverterTypesC_FloatType) {
      Do<cImpl>(mFloat, std::forward<Args>(aArgs)...);
    }
    else {
      mException.SetThisThreadException("Unsupported data type.");
    }
  }

  template<class cImpl, typename... Args>
  void SelectImpl(Args&&... aArgs)
  {
    if (!mUInt8 && !mUInt16 && !mUInt32 && !mFloat) {
      mException.SetThisThreadException("Unsupported data type.");
      return;
    }
    CheckImpl<cImpl>(mUInt8, std::forward<Args>(aArgs)...);
    CheckImpl<cImpl>(mUInt16, std::forward<Args>(aArgs)...);
    CheckImpl<cImpl>(mUInt32, std::forward<Args>(aArgs)...);
    CheckImpl<cImpl>(mFloat, std::forward<Args>(aArgs)...);
  }

  template<class cImpl, typename... Args, typename T>
  void CheckImpl(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, Args&&... aArgs)
  {
    if (aImpl) {
      Do<cImpl>(aImpl, std::forward<Args>(aArgs)...);
    }
  }

  template<class cImpl, typename... Args, typename T>
  void Do(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, Args&&... aArgs)
  {
    cImpl::Do(aImpl, std::forward<Args>(aArgs)...);
  }

  struct cCreateImpl
  {
    template<typename T, typename... Args>
    static void Do(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, Args&&... aArgs)
    {
      aImpl = std::make_shared<bpImageConverter<T>>(std::forward<Args>(aArgs)...);
    }
  };

  struct cNeedCopyBlockImpl
  {
    template<typename T, typename... Args>
    static void Do(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, bool& aResult, Args&&... aArgs)
    {
      aResult = aImpl->NeedCopyBlock(std::forward<Args>(aArgs)...);
    }
  };

  struct cCopyBlockImpl
  {
    template<typename T, typename WrongT, typename... Args>
    static void Do(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, const WrongT* aFileDataBlock, Args&&... aArgs)
    {
      throw "Block data type does not match converter data type";
    }

    template<typename T, typename... Args>
    static void Do(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, const T* aFileDataBlock, Args&&... aArgs)
    {
      aImpl->CopyBlock(aFileDataBlock, std::forward<Args>(aArgs)...);
    }
  };

  struct cFinishImpl
  {
    template<typename T, typename... Args>
    static void Do(bpSharedPtr<bpImageConverterInterface<T>>& aImpl, Args&&... aArgs)
    {
      aImpl->Finish(std::forward<Args>(aArgs)...);
    }
  };

  bpSharedPtr<bpImageConverterInterface<bpUInt8>> mUInt8;
  bpSharedPtr<bpImageConverterInterface<bpUInt16>> mUInt16;
  bpSharedPtr<bpImageConverterInterface<bpUInt32>> mUInt32;
  bpSharedPtr<bpImageConverterInterface<bpFloat>> mFloat;

  bpThreadLocalException mException;
};


static bpConverterTypes::tDataType Convert(bpConverterTypesC_DataType aDataType)
{
  if (aDataType == bpConverterTypesC_UInt8Type) {
    return bpConverterTypes::bpUInt8Type;
  }
  else if (aDataType == bpConverterTypesC_UInt16Type) {
    return bpConverterTypes::bpUInt16Type;
  }
  else if (aDataType == bpConverterTypesC_UInt32Type) {
    return bpConverterTypes::bpUInt32Type;
  }
  else if (aDataType == bpConverterTypesC_FloatType) {
    return bpConverterTypes::bpFloatType;
  }
  throw "Unsupported data type";
}

static bpConverterTypes::cImageExtent Convert(bpConverterTypesC_ImageExtentPtr aImageExtent)
{
  if (!aImageExtent) {
    throw "Image extent is not optional.";
  }

  bpConverterTypes::cImageExtent vImageExtent;
  vImageExtent.mExtentMinX = aImageExtent->mExtentMinX;
  vImageExtent.mExtentMinY = aImageExtent->mExtentMinY;
  vImageExtent.mExtentMinZ = aImageExtent->mExtentMinZ;
  vImageExtent.mExtentMaxX = aImageExtent->mExtentMaxX;
  vImageExtent.mExtentMaxY = aImageExtent->mExtentMaxY;
  vImageExtent.mExtentMaxZ = aImageExtent->mExtentMaxZ;
  return vImageExtent;
}

static bpConverterTypes::Dimension Convert(bpConverterTypesC_Dimension aDimension)
{
  if (aDimension == bpConverterTypesC_DimensionX) {
    return bpConverterTypes::X;
  }
  else if (aDimension == bpConverterTypesC_DimensionY) {
    return bpConverterTypes::Y;
  }
  else if (aDimension == bpConverterTypesC_DimensionZ) {
    return bpConverterTypes::Z;
  }
  else if (aDimension == bpConverterTypesC_DimensionC) {
    return bpConverterTypes::C;
  }
  else if (aDimension == bpConverterTypesC_DimensionT) {
    return bpConverterTypes::T;
  }
  throw "Unsupported dimension.";
}

static bpConverterTypes::tDimensionSequence5D Convert(bpConverterTypesC_DimensionSequence5DPtr aDimensionSequence)
{
  if (!aDimensionSequence) {
    throw "Dimension sequence is not optional.";
  }

  bpConverterTypes::tDimensionSequence5D vDimensionSequence(
    Convert(aDimensionSequence->mDimension0),
    Convert(aDimensionSequence->mDimension1),
    Convert(aDimensionSequence->mDimension2),
    Convert(aDimensionSequence->mDimension3),
    Convert(aDimensionSequence->mDimension4));
  return vDimensionSequence;
}

static bpConverterTypes::tSize5D Convert(bpConverterTypesC_Size5DPtr aSize)
{
  if (!aSize) {
    throw "Size is not optional.";
  }

  bpConverterTypes::tSize5D vSize{
    bpConverterTypes::X, aSize->mValueX,
    bpConverterTypes::Y, aSize->mValueY,
    bpConverterTypes::Z, aSize->mValueZ,
    bpConverterTypes::C, aSize->mValueC,
    bpConverterTypes::T, aSize->mValueT
  };
  return vSize;
}

static bpConverterTypes::tIndex5D Convert(bpConverterTypesC_Index5DPtr aIndex)
{
  if (!aIndex) {
    throw "Index is not optional.";
  }

  bpConverterTypes::tIndex5D vIndex{
    bpConverterTypes::X, aIndex->mValueX,
    bpConverterTypes::Y, aIndex->mValueY,
    bpConverterTypes::Z, aIndex->mValueZ,
    bpConverterTypes::C, aIndex->mValueC,
    bpConverterTypes::T, aIndex->mValueT
  };
  return vIndex;
}

static bpString Convert(bpConverterTypesC_String aString)
{
  return bpString(aString ? aString : "");
}

static bpConverterTypes::cOptions Convert(bpConverterTypesC_OptionsPtr aOptions)
{
  if (!aOptions) {
    return{};
  }

  bpConverterTypes::cOptions vOptions;
  vOptions.mThumbnailSizeXY = aOptions->mThumbnailSizeXY;
  vOptions.mFlipDimensionXYZ = { aOptions->mFlipDimensionX, aOptions->mFlipDimensionY, aOptions->mFlipDimensionZ };
  vOptions.mForceFileBlockSizeZ1 = aOptions->mForceFileBlockSizeZ1;
  vOptions.mEnableLogProgress = aOptions->mEnableLogProgress;
  vOptions.mNumberOfThreads = aOptions->mNumberOfThreads;
  vOptions.mCompressionAlgorithmType = (bpConverterTypes::tCompressionAlgorithmType)aOptions->mCompressionAlgorithmType;
  vOptions.mDisablePyramid = aOptions->mDisablePyramid;
  return vOptions;
}

static bpConverterTypes::tProgressCallback Convert(bpConverterTypesC_ProgressCallback aProgressCallback, void* aCallbackUserData)
{
  if (!aProgressCallback) {
    return{};
  }
  bpConverterTypes::tProgressCallback vProgressCallback = [aProgressCallback, aCallbackUserData](bpFloat aProgress, bpUInt64 aTotalBytesWritten) {
    aProgressCallback(aProgress, aTotalBytesWritten, aCallbackUserData);
  };
  return vProgressCallback;
}

static bpConverterTypes::tParameters Convert(bpConverterTypesC_ParametersPtr aParameters)
{
  if (!aParameters) {
    return{};
  }

  bpConverterTypes::tParameters vParameters;
  for (bpSize vSectionIndex = 0; vSectionIndex < aParameters->mValuesCount; ++vSectionIndex) {
    const auto& vSourceSection = aParameters->mValues[vSectionIndex];
    auto& vDestSection = vParameters[vSourceSection.mName];
    for (bpSize vIndex = 0; vIndex < vSourceSection.mValuesCount; ++vIndex) {
      const auto& vParameter = vSourceSection.mValues[vIndex];
      vDestSection[vParameter.mName] = vParameter.mValue;
    }
  }
  return vParameters;
}

static bpConverterTypes::cTimeInfo Convert(const bpConverterTypesC_TimeInfo& aTimeInfo)
{
  bpConverterTypes::cTimeInfo vTimeInfo;
  vTimeInfo.mJulianDay = aTimeInfo.mJulianDay;
  vTimeInfo.mNanosecondsOfDay = aTimeInfo.mNanosecondsOfDay;
  return vTimeInfo;
}

static bpConverterTypes::tTimeInfoVector Convert(bpConverterTypesC_TimeInfoVector aTimeInfoPerTimePoint)
{
  if (!aTimeInfoPerTimePoint) {
    return{};
  }

  bpConverterTypes::tTimeInfoVector vTimeInfoPerTimePoint;
  vTimeInfoPerTimePoint.reserve(aTimeInfoPerTimePoint->mValuesCount);
  for (bpSize vIndex = 0; vIndex < aTimeInfoPerTimePoint->mValuesCount; ++vIndex) {
    vTimeInfoPerTimePoint.push_back(Convert(aTimeInfoPerTimePoint->mValues[vIndex]));
  }
  return vTimeInfoPerTimePoint;
}

static bpConverterTypes::cColor Convert(const bpConverterTypesC_Color& aColor)
{
  bpConverterTypes::cColor vColor;
  vColor.mRed = aColor.mRed;
  vColor.mGreen = aColor.mGreen;
  vColor.mBlue = aColor.mBlue;
  vColor.mAlpha = aColor.mAlpha;
  return vColor;
}

static bpConverterTypes::cColorInfo Convert(const bpConverterTypesC_ColorInfo& aColorInfo)
{
  bpConverterTypes::cColorInfo vColorInfo;
  vColorInfo.mBaseColor = Convert(aColorInfo.mBaseColor);
  vColorInfo.mColorTable.resize(aColorInfo.mColorTableSize);
  for (bpSize vIndex = 0; vIndex < aColorInfo.mColorTableSize; ++vIndex) {
    vColorInfo.mColorTable[vIndex] = Convert(aColorInfo.mColorTable[vIndex]);
  }
  vColorInfo.mGammaCorrection = aColorInfo.mGammaCorrection;
  vColorInfo.mIsBaseColorMode = aColorInfo.mIsBaseColorMode;
  vColorInfo.mOpacity = aColorInfo.mOpacity;
  vColorInfo.mRangeMin = aColorInfo.mRangeMin;
  vColorInfo.mRangeMax = aColorInfo.mRangeMax;
  return vColorInfo;
}

static bpConverterTypes::tColorInfoVector Convert(bpConverterTypesC_ColorInfoVector aColorInfoPerChannel)
{
  if (!aColorInfoPerChannel) {
    return{};
  }

  bpConverterTypes::tColorInfoVector vColorInfoPerChannel;
  vColorInfoPerChannel.reserve(aColorInfoPerChannel->mValuesCount);
  for (bpSize vIndex = 0; vIndex < aColorInfoPerChannel->mValuesCount; ++vIndex) {
    vColorInfoPerChannel.push_back(Convert(aColorInfoPerChannel->mValues[vIndex]));
  }
  return vColorInfoPerChannel;
}


bpImageConverterCPtr bpImageConverterC_Create(
  bpConverterTypesC_DataType aDataType, bpConverterTypesC_Size5DPtr aImageSize, bpConverterTypesC_Size5DPtr aSample,
  bpConverterTypesC_DimensionSequence5DPtr aDimensionSequence, bpConverterTypesC_Size5DPtr aFileBlockSize,
  bpConverterTypesC_String aOutputFile, bpConverterTypesC_OptionsPtr aOptions,
  bpConverterTypesC_String aApplicationName, bpConverterTypesC_String aApplicationVersion,
  bpConverterTypesC_ProgressCallback aProgressCallback, void* aCallbackUserData)
{
  bpImageConverterC* vImageConverterC = new bpImageConverterC();
  vImageConverterC->TryExecute([&] {
    bpConverterTypes::tDataType vDataType = Convert(aDataType);
    bpConverterTypes::tDimensionSequence5D vDimensionSequence = Convert(aDimensionSequence);
    bpConverterTypes::tSize5D vImageSize = Convert(aImageSize);
    bpConverterTypes::tSize5D vSample = Convert(aSample);
    bpConverterTypes::tSize5D vFileBlockSize = Convert(aFileBlockSize);

    bpString vOutputFile = Convert(aOutputFile);
    bpConverterTypes::cOptions vOptions;
    if (aOptions) {
      vOptions = Convert(aOptions);
    }

    bpString vApplicationName = Convert(aApplicationName);
    bpString vApplicationVersion = Convert(aApplicationVersion);
    bpConverterTypes::tProgressCallback vProgressCallback = Convert(aProgressCallback, aCallbackUserData);

    vImageConverterC->Init(
      aDataType, vDataType, vImageSize, vSample, vDimensionSequence, vFileBlockSize,
      vOutputFile, vOptions, vApplicationName, vApplicationVersion, std::move(vProgressCallback));
  });
  return vImageConverterC;
}


void bpImageConverterC_Destroy(bpImageConverterCPtr aImageConverterC)
{
  delete aImageConverterC;
}


const char* bpImageConverterC_GetLastException(bpImageConverterCPtr aImageConverterC)
{
  return aImageConverterC ? aImageConverterC->GetLastException() : nullptr;
}


bool bpImageConverterC_NeedCopyBlock(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_Index5DPtr aBlockIndex)
{
  if (!aImageConverterC) {
    return false;
  }

  bool vResult = false;
  aImageConverterC->TryExecute([&] {
    vResult = aImageConverterC->NeedCopyBlock(Convert(aBlockIndex));
  });
  return vResult;
}


template<typename T>
static void bpImageConverterC_CopyBlock(bpImageConverterCPtr aImageConverterC, T* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex)
{
  if (!aImageConverterC) {
    return;
  }

  aImageConverterC->TryExecute([&] {
    aImageConverterC->CopyBlock(aFileDataBlock, Convert(aBlockIndex));
  });
}


void bpImageConverterC_CopyBlockUInt8(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_UInt8* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex)
{
  bpImageConverterC_CopyBlock(aImageConverterC, aFileDataBlock, aBlockIndex);
}


void bpImageConverterC_CopyBlockUInt16(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_UInt16* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex)
{
  bpImageConverterC_CopyBlock(aImageConverterC, aFileDataBlock, aBlockIndex);
}


void bpImageConverterC_CopyBlockUInt32(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_UInt32* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex)
{
  bpImageConverterC_CopyBlock(aImageConverterC, aFileDataBlock, aBlockIndex);
}


void bpImageConverterC_CopyBlockFloat(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_Float* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex)
{
  bpImageConverterC_CopyBlock(aImageConverterC, aFileDataBlock, aBlockIndex);
}


void bpImageConverterC_Finish(
  bpImageConverterCPtr aImageConverterC,
  bpConverterTypesC_ImageExtentPtr aImageExtent,
  bpConverterTypesC_ParametersPtr aParameters,
  bpConverterTypesC_TimeInfoVector aTimeInfoPerTimePoint,
  bpConverterTypesC_ColorInfoVector aColorInfoPerChannel,
  bool aAutoAdjustColorRange)
{
  if (!aImageConverterC) {
    return;
  }

  aImageConverterC->TryExecute([&] {
    bpConverterTypes::cImageExtent vImageExtent;
    if (aImageExtent) {
      vImageExtent = Convert(aImageExtent);
    }
    else {
      vImageExtent.mExtentMinX = 0;
      vImageExtent.mExtentMinY = 0;
      vImageExtent.mExtentMinZ = 0;
      vImageExtent.mExtentMaxX = 0;
      vImageExtent.mExtentMaxY = 0;
      vImageExtent.mExtentMaxZ = 0;
    }

    bpConverterTypes::tParameters vParameters = Convert(aParameters);
    bpConverterTypes::tTimeInfoVector vTimeInfoPerTimePoint = Convert(aTimeInfoPerTimePoint);
    bpConverterTypes::tColorInfoVector vColorInfoPerChannel = Convert(aColorInfoPerChannel);
    aImageConverterC->Finish(vImageExtent, vParameters, vTimeInfoPerTimePoint, vColorInfoPerChannel, aAutoAdjustColorRange);
  });
}
