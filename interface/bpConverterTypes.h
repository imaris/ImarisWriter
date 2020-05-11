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
#ifndef __BP_CONVERTER_TYPES__
#define __BP_CONVERTER_TYPES__

#include <exception>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include <string>
#include <set>
#include <cmath>
#include <functional>


typedef char bpChar;
typedef signed char bpInt8;
typedef short int bpInt16;
typedef int bpInt32;
typedef long long bpInt64;

typedef unsigned char bpUChar;
typedef unsigned char bpUInt8;
typedef unsigned short int bpUInt16;
typedef unsigned int bpUInt32;
typedef unsigned long long bpUInt64;

typedef size_t bpSize;
typedef intptr_t bpSizeSigned;
typedef float bpFloat;
typedef double bpDouble;

typedef bpUInt64 bpId;

using bpException = std::exception;
using bpError = std::runtime_error;

using bpString = std::string;
using bpVec3 = std::array<bpSize, 3>;
using bpVec2 = std::array<bpSize, 2>;
using bpFloatVec3 = std::array<bpFloat, 3>;
using bpIntVec3 = std::array<bpInt32, 3>;
using bpBoolVec3 = std::array<bool, 3>;
template<typename T>
using bpSharedPtr = std::shared_ptr<T>;
template<typename T>
using bpUniquePtr = std::unique_ptr<T>;

struct bpVector3Float : public bpFloatVec3
{
public:
  bpVector3Float()
    : bpFloatVec3{ 0, 0, 0 }
  {
  }

  bpVector3Float(bpFloat aX, bpFloat aY, bpFloat aZ)
    : bpFloatVec3{ aX, aY, aZ }
  {
  }
};


template<typename T>
class bpSequence5
{
private:
  std::vector<T> mSequence5;

public:
  //bpSequence5(const T& aValue0, const T& aValue1, const T& aValue2, const T& aValue3, const T& aValue4) throw bpDuplicatedArgumentException;
  bpSequence5(const T& aValue0, const T& aValue1, const T& aValue2, const T& aValue3, const T& aValue4)
    : mSequence5{ aValue0, aValue1, aValue2, aValue3, aValue4 }
  {
    if (std::set<T>(mSequence5.begin(), mSequence5.end()).size() != 5) {
      throw "duplicates detected!";
    }
  }

  //const T& operator [] (bpSize aIndex) const throw bpIndexGreaterThan4Exception;
  const T& operator [] (bpSize aIndex) const
  {
    return mSequence5[aIndex];
  }
};


template<typename Key, typename T>
class bpMap5
{
private:
  std::map<Key, T> mMap;

public:

  //bpMap5(const Key& aKey0, const T& aValue0, const Key& aKey1, const T& aValue1, const Key& aKey2, const T& aValue2, const Key& aKey3, const T& aValue3, const Key& aKey4, const T& aValue4) throw bpDuplicatedKeyException;
  bpMap5(const Key& aKey0, const T& aValue0, const Key& aKey1, const T& aValue1, const Key& aKey2, const T& aValue2, const Key& aKey3, const T& aValue3, const Key& aKey4, const T& aValue4)
    : bpMap5{ { aKey0, aValue0 },{ aKey1, aValue1 },{ aKey2, aValue2 },{ aKey3, aValue3 },{ aKey4, aValue4 } }
  {
  }

  using tKeyValue = std::pair<Key, T>;

  bpMap5(const tKeyValue& aKeyValue0, const tKeyValue& aKeyValue1, const tKeyValue& aKeyValue2, const tKeyValue& aKeyValue3, const tKeyValue& aKeyValue4)
    : mMap{ aKeyValue0, aKeyValue1, aKeyValue2, aKeyValue3, aKeyValue4 }
  {
    if (mMap.size() != 5) {
      throw "duplicates detected!";
    }
  }

  T& operator [] (Key aKey)
  {
    return mMap.at(aKey);
  }

  //const T& operator [] (Key aKey) const throw bpKeyNotFoundException;
  const T& operator [] (Key aKey) const
  {
    return mMap.at(aKey);
  }
};


namespace bpConverterTypes
{
  enum tDataType
  {
    bpNoType,
    bpInt8Type,
    bpUInt8Type,
    bpInt16Type,
    bpUInt16Type,
    bpInt32Type,
    bpUInt32Type,
    bpFloatType,
    bpDoubleType,
    bpInt64Type,
    bpUInt64Type
  };

  enum Dimension {
    X,
    Y,
    Z,
    C,
    T
  };

  typedef bpSequence5<Dimension> tDimensionSequence5D;
  typedef bpMap5<Dimension, bpSize> tSize5D;
  typedef tSize5D tIndex5D;
  typedef bpMap5<Dimension, bool> tBool5D;

  struct cColor {
    float mRed;
    float mGreen;
    float mBlue;
    float mAlpha;
  };

  struct cColorInfo {
    bool mIsBaseColorMode = true;
    cColor mBaseColor = cColor{ 0, 0, 0, 1 };
    std::vector<cColor> mColorTable;
    float mOpacity = 1;
    float mRangeMin = 0;
    float mRangeMax = 255;
    float mGammaCorrection = 1;

    cColor GetColor(bpFloat aValue) const
    {
      if (mIsBaseColorMode) {
        if (aValue <= mRangeMin) {
          return cColor{ 0, 0, 0, 1 };
        }
        else if (aValue >= mRangeMax) {
          return mBaseColor;
        }
        else {
          bpFloat t = (aValue - mRangeMin) / (mRangeMax - mRangeMin);
          if (mGammaCorrection != 1.0f) {
            t = powf(t, 1.0f / mGammaCorrection);
          }
          return cColor{ t*mBaseColor.mRed, t*mBaseColor.mGreen, t*mBaseColor.mBlue, 1 };
        }
      }
      else {
        if (aValue <= mRangeMin) {
          return mColorTable[0];
        }
        else if (aValue >= mRangeMax) {
          return mColorTable[mColorTable.size() - 1];
        }
        else {
          bpFloat t = (aValue - mRangeMin) / (mRangeMax - mRangeMin);
          if (mGammaCorrection != 1.0f) {
            t = powf(t, 1.0f / mGammaCorrection);
          }
          return mColorTable[(bpSize)(t*(mColorTable.size() - 1))];
        }
      }
    }
  };

  struct cTimeInfo {
    bpInt32 mJulianDay;
    bpInt64 mNanosecondsOfDay;
  };

  typedef std::vector<cColorInfo> tColorInfoVector;
  typedef std::vector<cTimeInfo> tTimeInfoVector;
  typedef std::map<std::string, std::map<std::string, std::string> > tParameters;

  struct cImageExtent {
    float mExtentMinX;
    float mExtentMinY;
    float mExtentMinZ;
    float mExtentMaxX;
    float mExtentMaxY;
    float mExtentMaxZ;
  };

  enum tCompressionAlgorithmType {
    eCompressionAlgorithmNone = 0,
    eCompressionAlgorithmGzipLevel1 = 1,
    eCompressionAlgorithmGzipLevel2 = 2,
    eCompressionAlgorithmGzipLevel3 = 3,
    eCompressionAlgorithmGzipLevel4 = 4,
    eCompressionAlgorithmGzipLevel5 = 5,
    eCompressionAlgorithmGzipLevel6 = 6,
    eCompressionAlgorithmGzipLevel7 = 7,
    eCompressionAlgorithmGzipLevel8 = 8,
    eCompressionAlgorithmGzipLevel9 = 9,
    eCompressionAlgorithmShuffleGzipLevel1 = 11,
    eCompressionAlgorithmShuffleGzipLevel2 = 12,
    eCompressionAlgorithmShuffleGzipLevel3 = 13,
    eCompressionAlgorithmShuffleGzipLevel4 = 14,
    eCompressionAlgorithmShuffleGzipLevel5 = 15,
    eCompressionAlgorithmShuffleGzipLevel6 = 16,
    eCompressionAlgorithmShuffleGzipLevel7 = 17,
    eCompressionAlgorithmShuffleGzipLevel8 = 18,
    eCompressionAlgorithmShuffleGzipLevel9 = 19,
    eCompressionAlgorithmLZ4 = 21,
    eCompressionAlgorithmShuffleLZ4 = 31
  };

  struct cOptions
  {
    bpSize mThumbnailSizeXY = 256;
    bool mForceFileBlockSizeZ1 = false;
    bpBoolVec3 mFlipDimensionXYZ{ false, false, false };
    bool mEnableLogProgress = false;
    bpSize mNumberOfThreads = 8;
    tCompressionAlgorithmType mCompressionAlgorithmType = eCompressionAlgorithmGzipLevel2;
  };

  using tProgressCallback = std::function<void(bpFloat aProgress, bpUInt64 aTotalBytesWritten)>;
};


#endif // __BP_CONVERTER_TYPES__
