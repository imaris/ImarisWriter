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
#include "bpWriterThreads.h"
#include "bpMemoryManager.h"
#include "bpThreadPool.h"


class bpWriterThreads::cImpl
{
public:
  cImpl(bpSize aMaxBufferSizeMB, bpSize aNumberOfThreads, bpCompressionAlgorithm::tPtr aCompressionAlgorithm)
    : mCompressionThreads(aNumberOfThreads),
      mWriterThread(1),
      mFreeMemory(aMaxBufferSizeMB * 1024 * 1024),
      mCompressionAlgorithm(std::move(aCompressionAlgorithm))
  {
  }

  void StartWrite(bpMemoryHandle aData, tWrite aWrite, tPreFunction aPreFunction)
  {
    if (!mCompressionAlgorithm) {
      bpThreadPool::tCallback vReturnMemory = WaitReserveMemory(aData.GetSize());
      bpThreadPool::tFunction vWrite = [aData, vOriginalWrite = std::move(aWrite), vPreFunction = std::move(aPreFunction)]{
        if (vPreFunction) {
          vPreFunction();
        }
        vOriginalWrite(aData.GetData(), aData.GetSize());
      };
      mWriterThread.Run(std::move(vWrite), std::move(vReturnMemory));
      return;
    }

    bpSize vMaxCompressedDataSize = mCompressionAlgorithm->GetMaxCompressedSize(aData.GetSize());
    bpSize vAllocSize = aData.GetSize() + vMaxCompressedDataSize;

    bpThreadPool::tCallback vReturnMemory = WaitReserveMemory(vAllocSize);

    bpMemoryBlock<bpUInt8> vBuffer = mManager.GetMemory(vMaxCompressedDataSize);
    bpSharedPtr<bpSize> vCompressedDataSize = std::make_shared<bpSize>();

    auto vCompress = [this, aData, vBuffer, vCompressedDataSize] {
      bpSize vResultSize = vBuffer.GetSize();
      mCompressionAlgorithm->Compress(aData.GetData(), aData.GetSize(), vBuffer.GetData(), vResultSize);
      *vCompressedDataSize = vResultSize;
    };

    bpThreadPool::tFunction vWrite = [vBuffer, vOriginalWrite = std::move(aWrite), vCompressedDataSize]{
      vOriginalWrite(vBuffer.GetData(), *vCompressedDataSize);
    };

    bpThreadPool::tFunction vFunction = [this, vDoCompress = std::move(vCompress), vDoWrite = std::move(vWrite), vDoReturnMemory = std::move(vReturnMemory), vPreFunction = std::move(aPreFunction)] () mutable {
      if (vPreFunction) {
        vPreFunction();
      }
      vDoCompress();
      mWriterThread.Run(vDoWrite, vDoReturnMemory);
    };
    mCompressionThreads.Run(std::move(vFunction));
  }

  void FinishWrite()
  {
    mCompressionThreads.WaitAll();
    mWriterThread.WaitAll();
    mWriterThread.CallFinishedCallbacks();
  }

private:
  bpThreadPool::tCallback WaitReserveMemory(bpSize aSize)
  {
    mWriterThread.CallFinishedCallbacks();
    while (static_cast<bpInt64>(aSize) > mFreeMemory) {
      mWriterThread.WaitOne();
      mWriterThread.CallFinishedCallbacks();
    }

    mFreeMemory -= aSize;
    bpThreadPool::tCallback vReturnMemory = [this, aSize] {
      mFreeMemory += aSize;
    };
    return vReturnMemory;
  }

  bpThreadPool mCompressionThreads;
  bpThreadPool mWriterThread;
  bpInt64 mFreeMemory;
  bpMemoryManager<bpUInt8> mManager;
  bpCompressionAlgorithm::tPtr mCompressionAlgorithm;
};


bpWriterThreads::bpWriterThreads(bpSize aMaxBufferSizeMB, bpSize aNumberOfThreads, bpCompressionAlgorithmFactory::tPtr aCompressionAlgorithmFactory, bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType, bpConverterTypes::tDataType aDataType)
  : mImpl(std::make_shared<cImpl>(aMaxBufferSizeMB, aNumberOfThreads, std::move(aCompressionAlgorithmFactory->Create(aCompressionAlgorithmType, aDataType))))
{
}


void bpWriterThreads::StartWrite(bpMemoryHandle aData, bpSize aCompressionLevel, tWrite aWrite, tPreFunction aPreFunction)
{
  mImpl->StartWrite(std::move(aData), std::move(aWrite), std::move(aPreFunction));
}


void bpWriterThreads::FinishWrite()
{
  mImpl->FinishWrite();
}
