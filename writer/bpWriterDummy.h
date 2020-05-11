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
#ifndef __BP_WRITER_DUMMY__
#define __BP_WRITER_DUMMY__


#include "bpMemoryBlock.h"
#include "bpHistogram.h"
#include "bpThumbnail.h"

#include <functional>

class bpWriterDummy : public bpWriter
{
public:
  using tPreFunction = std::function<void()>;

  bpWriterDummy()
  {}

  virtual ~bpWriterDummy() = default;

  virtual void WriteHistogram(const bpHistogram& aHistogram, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR) {};

  virtual void WriteMetadata(
    const bpString& aApplicationName,
    const bpString& aApplicationVersion,
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel) {};

  virtual void WriteThumbnail(const bpThumbnail& aThumbnail) {};

  virtual void WriteDataBlock(
    const void* aData, bpSize aDataSize,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR) {
    // do some fast dummy operation with the data
    bpUInt8 vHash = ((bpUInt8*)aData)[aDataSize / 2];
    if (vHash > 0) { // pretend to use local variable
    }
  };

};


#endif // __BP_WRITER_DUMMY__
