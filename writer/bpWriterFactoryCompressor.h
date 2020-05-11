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
#ifndef __BP_WRITER_FACTORY_COMPRESSOR__
#define __BP_WRITER_FACTORY_COMPRESSOR__

#include "bpWriterFactory.h"

class bpWriterFactoryCompressor : public bpWriterFactory
{
public:
  bpWriterFactoryCompressor(bpSharedPtr<bpWriterFactory> aWriterFactory, bpSize aNumberOfCompressionThreads, bpConverterTypes::tProgressCallback aProgressCallback);

  bpSharedPtr<bpWriter> CreateWriter(const bpString& aFilename, const bpImsLayout& aImageLayout, bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType);

private:
  bpSharedPtr<bpWriterFactory> mWriterFactory;
  bpSize mNumberOfCompressionThreads;
  bpConverterTypes::tProgressCallback mProgressCallback;
};

#endif // __BP_WRITER_FACTORY_HDF5__

