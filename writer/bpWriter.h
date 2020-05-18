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
#ifndef __BP_WRITER__
#define __BP_WRITER__


#include "bpMemoryBlock.h"
#include "bpHistogram.h"
#include "bpThumbnail.h"

#include <functional>

class bpWriter
{
public:
  using tPreFunction = std::function<void()>;

  virtual ~bpWriter() = default;

  virtual void WriteHistogram(const bpHistogram& aHistogram, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR) = 0;

  virtual void WriteMetadata(
    const bpString& aApplicationName,
    const bpString& aApplicationVersion,
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel) = 0;

  virtual void WriteThumbnail(const bpThumbnail& aThumbnail) = 0;

  virtual void WriteDataBlock(
    const void* aData, bpSize aDataSize,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR) = 0;

  virtual void StartWriteDataBlock(
    bpMemoryHandle aData,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR, tPreFunction aPreFunction)
  {
    if (aPreFunction) {
      aPreFunction();
    }

    WriteDataBlock(aData.GetData(), aData.GetSize(), aBlockIndexX, aBlockIndexY, aBlockIndexZ, aIndexT, aIndexC, aIndexR);
  }

  virtual void FinishWriteDataBlocks()
  {
  }
};

#endif // __BP_WRITER__
