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
#include "bpWriterFactoryCompressor.h"
#include "bpWriterCompressor.h"


bpWriterFactoryCompressor::bpWriterFactoryCompressor(bpSharedPtr<bpWriterFactory> aWriterFactory, bpSize aNumberOfCompressionThreads, bpConverterTypes::tProgressCallback aProgressCallback)
  : mWriterFactory(std::move(aWriterFactory)),
    mNumberOfCompressionThreads(aNumberOfCompressionThreads),
    mProgressCallback(std::move(aProgressCallback))
{
}

bpSharedPtr<bpWriter> bpWriterFactoryCompressor::CreateWriter(const bpString& aFilename, const bpImsLayout& aImageLayout, bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType)
{
  return std::make_shared<bpWriterCompressor>(mWriterFactory, aFilename, aImageLayout, aCompressionAlgorithmType, mNumberOfCompressionThreads, mProgressCallback);
}
