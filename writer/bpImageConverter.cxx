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
#include "../interface/bpImageConverter.h"
#include "bpImageConverterImpl.h"


#include <mutex>


template <typename TDataType>
class bpImageConverter<TDataType>::cThreadSafeDecorator : public bpImageConverterInterface<TDataType>
{
public:
  explicit cThreadSafeDecorator(bpUniquePtr<bpImageConverterInterface<TDataType>> aImpl)
    : mImpl(std::move(aImpl))
  {
  }

  bool NeedCopyBlock(const bpConverterTypes::tIndex5D& aBlockIndex) const
  {
    tLock vLock(mMutex);
    return mImpl->NeedCopyBlock(aBlockIndex);
  }

  void CopyBlock(const TDataType* aFileDataBlock, const bpConverterTypes::tIndex5D& aBlockIndex)
  {
    tLock vLock(mMutex);
    mImpl->CopyBlock(aFileDataBlock, aBlockIndex);
  }

  void Finish(
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
    bool aAutoAdjustColorRange)
  {
    tLock vLock(mMutex);
    mImpl->Finish(aImageExtent, aParameters, aTimeInfoPerTimePoint, aColorInfoPerChannel, aAutoAdjustColorRange);
  }

private:
  using tMutex = std::mutex;
  using tLock = std::lock_guard<tMutex>;

  mutable tMutex mMutex;

  bpUniquePtr<bpImageConverterInterface<TDataType>> mImpl;
};


template <typename TDataType>
bpImageConverter<TDataType>::bpImageConverter(
  bpConverterTypes::tDataType aDataType, const bpConverterTypes::tSize5D& aImageSize, const bpConverterTypes::tSize5D& aSample,
  bpConverterTypes::tDimensionSequence5D aBlockDataDimensionSequence, const bpConverterTypes::tSize5D& aFileBlockSize,
  const bpString& aOutputFile, const bpConverterTypes::cOptions& aOptions,
  const bpString& aApplicationName, const bpString& aApplicationVersion, bpConverterTypes::tProgressCallback aProgressCallback)
{
  auto vImpl = std::make_unique<bpImageConverterImpl<TDataType>>(aDataType, aBlockDataDimensionSequence, aImageSize, aSample, aFileBlockSize, aOutputFile, aOptions, aApplicationName, aApplicationVersion, std::move(aProgressCallback));
  mImpl = std::make_unique<cThreadSafeDecorator>(std::move(vImpl));
}


template<typename TDataType>
bpImageConverter<TDataType>::~bpImageConverter()
{
}


template<typename TDataType>
bool bpImageConverter<TDataType>::NeedCopyBlock(const bpConverterTypes::tIndex5D& aBlockIndex) const
{
  return mImpl->NeedCopyBlock(aBlockIndex);
}


template<typename TDataType>
void bpImageConverter<TDataType>::CopyBlock(const TDataType* aFileDataBlock, const bpConverterTypes::tIndex5D& aBlockIndex)
{
  mImpl->CopyBlock(aFileDataBlock, aBlockIndex);
}


template<typename TDataType>
void bpImageConverter<TDataType>::Finish(
  const bpConverterTypes::cImageExtent& aImageExtent,
  const bpConverterTypes::tParameters& aParameters,
  const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
  const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
  bool aAutoAdjustColorRange)
{
  mImpl->Finish(aImageExtent, aParameters, aTimeInfoPerTimePoint, aColorInfoPerChannel, aAutoAdjustColorRange);
}


template class bpImageConverter<bpUInt8>;
template class bpImageConverter<bpUInt16>;
template class bpImageConverter<bpUInt32>;
template class bpImageConverter<bpFloat>;
