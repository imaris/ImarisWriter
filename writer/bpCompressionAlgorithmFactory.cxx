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
#include "bpCompressionAlgorithmFactory.h"
#include "bpGzip.h"
#include "bpLZ4.h"
#include "bpShuffle.h"


bpSize bpCompressionAlgorithmFactory::GetNBytesShuffle(bpConverterTypes::tDataType aDataType)
{
  switch (aDataType) {
  case bpConverterTypes::bpUInt16Type:
  case bpConverterTypes::bpInt16Type:
    return 2;
  case bpConverterTypes::bpUInt32Type:
  case bpConverterTypes::bpInt32Type:
    return 4;
  case bpConverterTypes::bpUInt8Type:
  case bpConverterTypes::bpInt8Type:
  case bpConverterTypes::bpUInt64Type:
  case bpConverterTypes::bpInt64Type:
  case bpConverterTypes::bpNoType:
  case bpConverterTypes::bpFloatType:
  case bpConverterTypes::bpDoubleType:
  default:
    return 1;
  }
}

bpCompressionAlgorithm::tPtr bpCompressionAlgorithmFactory::Create(bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType, bpConverterTypes::tDataType aDataType)
{
  switch (aCompressionAlgorithmType) {
  case bpConverterTypes::eCompressionAlgorithmGzipLevel1:
    return std::make_shared<bpGzip>(1);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel2:
    return std::make_shared<bpGzip>(2);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel3:
    return std::make_shared<bpGzip>(3);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel4:
    return std::make_shared<bpGzip>(4);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel5:
    return std::make_shared<bpGzip>(5);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel6:
    return std::make_shared<bpGzip>(6);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel7:
    return std::make_shared<bpGzip>(7);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel8:
    return std::make_shared<bpGzip>(8);
  case bpConverterTypes::eCompressionAlgorithmGzipLevel9:
    return std::make_shared<bpGzip>(9);
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel1:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(1));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel2:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(2));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel3:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(3));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel4:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(4));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel5:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(5));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel6:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(6));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel7:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(7));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel8:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(8));
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel9:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpGzip>(9));
  case bpConverterTypes::eCompressionAlgorithmLZ4:
    return std::make_shared<bpLZ4>();
  case bpConverterTypes::eCompressionAlgorithmShuffleLZ4:
    return std::make_shared<bpShuffle>(GetNBytesShuffle(aDataType), std::make_shared<bpLZ4>());
  case bpConverterTypes::eCompressionAlgorithmNone:
  default:
    return{};
  }
}

