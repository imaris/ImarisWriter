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
#include "bpGzip.h"


#include <zlib.h>


bpGzip::bpGzip(bpSize aCompressionLevel)
  : mCompressionLevel(aCompressionLevel)
{
}


bpSize bpGzip::GetMaxCompressedSize(bpSize aDataSize)
{
  return static_cast<bpSize>(compressBound(static_cast<uLongf>(aDataSize)));
}


void bpGzip::Compress(const void* aData, bpSize aDataSize, void* aCompressedData, bpSize& aCompressedDataSize)
{
  uLong vDestLen = static_cast<uLongf>(aCompressedDataSize);
  compress2(static_cast<Bytef*>(aCompressedData), &vDestLen, static_cast<const Bytef*>(aData), static_cast<uLongf>(aDataSize), mCompressionLevel);
  aCompressedDataSize = static_cast<bpSize>(vDestLen);
}

