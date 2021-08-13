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
#include "bpWriterCompressor.h"
#include "bpWriterThreads.h"
#include "bpThreadPool.h"


bpWriterCompressor::bpWriterCompressor(
  bpSharedPtr<bpWriterFactory> aWriterFactory,
  const bpString& aFilename,
  const bpImsLayout& aImageLayout,
  bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType,
  bpSize aNumberOfCompressionThreads,
  bpConverterTypes::tProgressCallback aProgressCallback)
: mThreads(std::make_unique<bpWriterThreads>(aNumberOfCompressionThreads * 3 + 32, aNumberOfCompressionThreads, std::make_shared<bpCompressionAlgorithmFactory>(), aCompressionAlgorithmType, aImageLayout.GetDataType())),
  mCallbackThread(std::make_unique<bpThreadPool>(1)),
  mProgressCallback(std::move(aProgressCallback)),
  mNumberOfBlocks(0),
  mNumberOfIncrements(0),
  mTotalBytesWritten(0)
{
  if (mProgressCallback) {
    for (bpSize vIndex = 0; vIndex < aImageLayout.GetNumberOfResolutionLevels(); vIndex++) {
      const auto& vSize = aImageLayout.GetNBlocks(vIndex);
      mNumberOfBlocks += vSize[0] * vSize[1] * vSize[2];
    }
    mNumberOfBlocks *= aImageLayout.GetNumberOfChannels() * aImageLayout.GetNumberOfTimePoints();
  }

  auto vInit = [this, aWriterFactory, aFilename, aImageLayout, aCompressionAlgorithmType] {
    mWriter = aWriterFactory->CreateWriter(aFilename, aImageLayout, aCompressionAlgorithmType);
  };
  RunInWriteThread(std::move(vInit));
}


bpWriterCompressor::~bpWriterCompressor()
{
  try {
    FinishWriteDataBlocks();
  }
  catch (bpException&) {
  }
}


void bpWriterCompressor::WriteMetadata(
  const bpString& aApplicationName,
  const bpString& aApplicationVersion,
  const bpConverterTypes::cImageExtent& aImageExtent,
  const bpConverterTypes::tParameters& aParameters,
  const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
  const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel)
{
  mWriter->WriteMetadata(aApplicationName, aApplicationVersion, aImageExtent, aParameters, aTimeInfoPerTimePoint, aColorInfoPerChannel);
}


void bpWriterCompressor::WriteHistogram(const bpHistogram& aHistogram, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  auto vWriteHistogram = [this, aHistogram, aIndexT, aIndexC, aIndexR] {
    mWriter->WriteHistogram(aHistogram, aIndexT, aIndexC, aIndexR);
  };

  RunInWriteThread(std::move(vWriteHistogram));
}


void bpWriterCompressor::RunInWriteThread(std::function<void()> aFunction)
{
  bpWriterThreads::tWrite vPretendWrite = [vFunction = std::move(aFunction)](const void* aCompressedData, bpSize aDataSize) {
    vFunction();
  };

  mThreads->StartWrite(bpMemoryHandle(), 0, std::move(vPretendWrite), {});
}


void bpWriterCompressor::WriteThumbnail(const bpThumbnail& aThumbnail)
{
  mWriter->WriteThumbnail(aThumbnail);
}


void bpWriterCompressor::WriteDataBlock(
  const void* aData, bpSize aDataSize,
  bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
  bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  mWriter->WriteDataBlock(aData, aDataSize, aBlockIndexX, aBlockIndexY, aBlockIndexZ, aIndexT, aIndexC, aIndexR);
}


void bpWriterCompressor::StartWriteDataBlock(
  bpMemoryHandle aData,
  bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
  bpSize aIndexT, bpSize aIndexC, bpSize aIndexR, tPreFunction aPreFunction)
{
  bpWriterThreads::tWrite vWriteBlock = [this, aBlockIndexX, aBlockIndexY, aBlockIndexZ, aIndexT, aIndexC, aIndexR](const void* aCompressedData, bpSize aDataSize) {
    WriteDataBlock(aCompressedData, aDataSize, aBlockIndexX, aBlockIndexY, aBlockIndexZ, aIndexT, aIndexC, aIndexR);
    if (mProgressCallback) {
      mCallbackThread->Run([this, aDataSize] { IncrementProgress(aDataSize); });
    }
  };

  mThreads->StartWrite(std::move(aData), 0, std::move(vWriteBlock), std::move(aPreFunction));
}


void bpWriterCompressor::FinishWriteDataBlocks()
{
  try {
    mThreads->FinishWrite();
  }
  catch (bpException&) {
    mCallbackThread->WaitAll();
    throw;
  }
  mCallbackThread->WaitAll();
}


void bpWriterCompressor::IncrementProgress(bpUInt64 aAdditionalBytesWritten)
{
  mNumberOfIncrements++;
  bpFloat vProgressPercent = static_cast<bpFloat>(mNumberOfIncrements) / mNumberOfBlocks;
  mTotalBytesWritten += aAdditionalBytesWritten;
  mProgressCallback(vProgressPercent, mTotalBytesWritten);
}
