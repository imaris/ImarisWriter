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
#include "bpShuffle.h"


bpShuffle::bpShuffle(bpSize aNBytes, bpCompressionAlgorithm::tPtr aCompressionAlgorithm)
  : mNBytes(aNBytes),
    mCompressionAlgorithm(std::move(aCompressionAlgorithm))
{
}


bool bpShuffle::DoesShuffle() const
{
  return mNBytes == 2 || mNBytes ==4;
}


bpSize bpShuffle::GetMaxCompressedSize(bpSize aDataSize)
{
  bpSize vMaxCompressedSize = mCompressionAlgorithm->GetMaxCompressedSize(aDataSize);
  return DoesShuffle() ? vMaxCompressedSize + aDataSize : vMaxCompressedSize;
}


void bpShuffle::Compress(const void* aData, bpSize aDataSize, void* aCompressedData, bpSize& aCompressedDataSize)
{
  if (!DoesShuffle()) {
    mCompressionAlgorithm->Compress(aData, aDataSize, aCompressedData, aCompressedDataSize);
    return;
  }

  const unsigned char* vDataSrc = static_cast<const unsigned char*>(aData);
  const unsigned char* vDataSrcEnd = vDataSrc + aDataSize;
  unsigned char* vDataDest = static_cast<unsigned char*>(aCompressedData) + (aCompressedDataSize - aDataSize);
  if (mNBytes == 2) {
    unsigned char* vDataDest0 = vDataDest;
    unsigned char* vDataDest1 = vDataDest0 + aDataSize / 2;
    while (vDataSrc + 16 < vDataSrcEnd) {
      for (bpSize vIndex = 0; vIndex < 8; vIndex++) {
        *vDataDest0++ = *vDataSrc++;
        *vDataDest1++ = *vDataSrc++;
      }
    }
    while (vDataSrc < vDataSrcEnd) {
      *vDataDest0++ = *vDataSrc++;
      *vDataDest1++ = *vDataSrc++;
    }
  }
  else if (mNBytes == 4) {
    unsigned char* vDataDest0 = vDataDest;
    unsigned char* vDataDest1 = vDataDest0 + aDataSize / 4;
    unsigned char* vDataDest2 = vDataDest1 + aDataSize / 4;
    unsigned char* vDataDest3 = vDataDest2 + aDataSize / 4;
    while (vDataSrc + 32 < vDataSrcEnd) {
      for (bpSize vIndex = 0; vIndex < 8; vIndex++) {
        *vDataDest0++ = *vDataSrc++;
        *vDataDest1++ = *vDataSrc++;
        *vDataDest2++ = *vDataSrc++;
        *vDataDest3++ = *vDataSrc++;
      }
    }
    while (vDataSrc < vDataSrcEnd) {
      *vDataDest0++ = *vDataSrc++;
      *vDataDest1++ = *vDataSrc++;
      *vDataDest2++ = *vDataSrc++;
      *vDataDest3++ = *vDataSrc++;
    }
  }
  bpSize vCompressedDataSize = aCompressedDataSize - aDataSize;
  mCompressionAlgorithm->Compress(vDataDest, aDataSize, aCompressedData, vCompressedDataSize);
  aCompressedDataSize = vCompressedDataSize;
}
