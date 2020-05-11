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
#ifndef __BP_WRITER_THREADS__
#define __BP_WRITER_THREADS__


#include "bpMemoryBlock.h"
#include "bpCompressionAlgorithmFactory.h"

/*
* Uses a specified number of threads to compress and a dedicated thread to write.
* The write data function will be called with compressed or uncompressed data, in both cases from the writer thread.
*/
class bpWriterThreads
{
public:

  // a function to run in the compression threads. in practice, this is going to be the resampling to the next level of resolution
  using tPreFunction = std::function<void()>;

  bpWriterThreads(bpSize aMaxBufferSizeMB, bpSize aNumberOfThreads, bpCompressionAlgorithmFactory::tPtr aCompressionAlgorithmFactory, bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType, bpConverterTypes::tDataType aDataType);

  using tWrite = std::function<void(const void* aData, bpSize aDataSize)>;

  void StartWrite(bpMemoryHandle aData, bpSize aCompressionLevel, tWrite aWrite, tPreFunction aPreFunction);
  void FinishWrite();

private:
  class cImpl;
  bpSharedPtr<cImpl> mImpl;
};


#endif
