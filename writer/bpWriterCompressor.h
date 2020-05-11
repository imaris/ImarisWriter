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
#ifndef __BP_WRITER_COMPRESSOR__
#define __BP_WRITER_COMPRESSOR__


#include "bpImsLayout.h"
#include "bpWriterFactory.h"

#include <functional>


class bpWriterThreads;
class bpThreadPool;


class bpWriterCompressor : public bpWriter
{
public:
  bpWriterCompressor(
    bpSharedPtr<bpWriterFactory> aWriterFactory,
    const bpString& aFilename,
    const bpImsLayout& aImageLayout,
    bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType,
    bpSize aNumberOfCompressionThreads,
    bpConverterTypes::tProgressCallback aProgressCallback);

  virtual ~bpWriterCompressor();

  virtual void WriteHistogram(const bpHistogram& aHistogram, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  virtual void WriteMetadata(
    const bpString& aApplicationName,
    const bpString& aApplicationVersion,
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel);

  virtual void WriteThumbnail(const bpThumbnail& aThumbnail);

  virtual void WriteDataBlock(
    const void* aData, bpSize aDataSize,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  virtual void StartWriteDataBlock(
    bpMemoryHandle aData,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR, tPreFunction aPreFunction);

  virtual void FinishWriteDataBlocks();

private:
  void IncrementProgress(bpUInt64 aAdditionalBytesWritten);

  void RunInWriteThread(std::function<void()> aFunction);

  bpSharedPtr<bpWriter> mWriter;

  bpUniquePtr<bpWriterThreads> mThreads;
  bpUniquePtr<bpThreadPool> mCallbackThread;
  bpConverterTypes::tProgressCallback mProgressCallback;
  bpSize mNumberOfBlocks;
  bpSize mNumberOfIncrements;
  bpUInt64 mTotalBytesWritten;
};

#endif // __BP_WRITER_COMPRESSOR__
