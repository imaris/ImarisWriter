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
#ifndef __BP_COMPRESSION_ALGORITHM__
#define __BP_COMPRESSION_ALGORITHM__


#include "../interface/bpConverterTypes.h"


class bpCompressionAlgorithm
{
public:
  using tPtr = bpSharedPtr<bpCompressionAlgorithm>;

  virtual ~bpCompressionAlgorithm() = default;

  virtual bpSize GetMaxCompressedSize(bpSize aDataSize) = 0;
  virtual void Compress(const void* aData, bpSize aDataSize, void* aCompressedData, bpSize& aCompressedDataSize) = 0;
};


#endif

