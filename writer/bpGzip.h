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
#ifndef __BP_GZIP__
#define __BP_GZIP__


#include "bpCompressionAlgorithm.h"


class bpGzip : public bpCompressionAlgorithm
{
public:
  explicit bpGzip(bpSize aCompressionLevel);

  bpSize GetMaxCompressedSize(bpSize aDataSize);
  void Compress(const void* aData, bpSize aDataSize, void* aCompressedData, bpSize& aCompressedDataSize);

private:
  bpSize mCompressionLevel;
};


#endif
