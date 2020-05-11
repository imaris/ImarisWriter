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
#include "bpLZ4.h"

#include <lz4.h>


bpSize bpLZ4::GetMaxCompressedSize(bpSize aDataSize)
{
  return static_cast<bpSize>(LZ4_compressBound(static_cast<int>(aDataSize))) + 16;
}


static void WriteBytes(char* aDest, bpSize aValue, bpSize aNBytes)
{
  for (bpSize vIndex = 0; vIndex < aNBytes; vIndex++) {
    aDest[aNBytes - 1 - vIndex] = static_cast<char>((aValue >> (8 * vIndex)) & 0xff);
  }
}


void bpLZ4::Compress(const void* aData, bpSize aDataSize, void* aCompressedData, bpSize& aCompressedDataSize)
{
  char* vDest = static_cast<char*>(aCompressedData);
  aCompressedDataSize = static_cast<bpSize>(LZ4_compress_default(
    static_cast<const char*>(aData),
    vDest + 16,
    static_cast<int>(aDataSize),
    static_cast<int>(aCompressedDataSize)));

  WriteBytes(vDest, aDataSize, 8);
  vDest += 8;
  WriteBytes(vDest, aDataSize, 4);
  vDest += 4;
  WriteBytes(vDest, aCompressedDataSize, 4);

  aCompressedDataSize += 16;
}

