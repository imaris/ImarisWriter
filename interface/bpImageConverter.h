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
#ifndef __BP_IMAGE_CONVERTER__
#define __BP_IMAGE_CONVERTER__

#include "../interface/ImarisWriterDllAPI.h"
#include "../interface/bpImageConverterInterface.h"


template<class TDataType>
class bpImageConverterImpl;


template<class TDataType>
class BP_IMARISWRITER_DLL_API bpImageConverter : public bpImageConverterInterface<TDataType>
{
public:
  bpImageConverter(
    bpConverterTypes::tDataType aDataType, const bpConverterTypes::tSize5D& aImageSize, const bpConverterTypes::tSize5D& aSample,
    bpConverterTypes::tDimensionSequence5D aBlockDataDimensionSequence, const bpConverterTypes::tSize5D& aFileBlockSize,
    const bpString& aOutputFile, const bpConverterTypes::cOptions& aOptions,
    const bpString& aApplicationName, const bpString& aApplicationVersion, bpConverterTypes::tProgressCallback aProgressCallback);

  ~bpImageConverter();

  bool NeedCopyBlock(const bpConverterTypes::tIndex5D& aBlockIndex) const override;

  void CopyBlock(const TDataType* aFileDataBlock, const bpConverterTypes::tIndex5D& aBlockIndex) override;

  void Finish(
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
    bool aAutoAdjustColorRange) override;

private:
  class cThreadSafeDecorator;

  bpUniquePtr<bpImageConverterInterface<TDataType>> mImpl;
};

#endif // __BP_IMAGE_CONVERTER__
