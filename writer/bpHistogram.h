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
#ifndef __BP_HISTOGRAM__
#define __BP_HISTOGRAM__


#include "../interface/bpConverterTypes.h"


class bpHistogram
{
public:
  bpHistogram(bpFloat aMin, bpFloat aMax, std::vector<bpUInt64> aBins)
    : mMin(aMin), mMax(aMax), mBins(std::move(aBins))
  {
  }

  bpFloat GetMin() const
  {
    return mMin;
  }

  bpFloat GetMax() const
  {
    return mMax;
  }

  bpSize GetNumberOfBins() const
  {
    return mBins.size();
  }

  bpUInt64 GetCount(bpSize aBin) const
  {
    return mBins[aBin];
  }

  bpFloat GetBinValue(bpSize aBin) const
  {
    return mMin + (mMax - mMin) * aBin / mBins.size();
  }

  bpSize GetBin(bpFloat aValue) const
  {
    return aValue <= mMin ? 0 : aValue >= mMax ? mBins.size() - 1 : static_cast<bpSize>((aValue - mMin) * mBins.size() / (mMax - mMin + 1));
  }

private:
  bpFloat mMin;
  bpFloat mMax;
  std::vector<bpUInt64> mBins;
};


bpHistogram bpResampleHistogram(const bpHistogram& aHistogram, bpSize aNewNumberOfBins);


static inline void bpMergeBins(std::vector<bpUInt64>& aBins, const std::vector<bpUInt64>& aOtherBins)
{
  bpSize vSize = aBins.size();
  for (bpSize vIndex = 0; vIndex < vSize; ++vIndex) {
    aBins[vIndex] += aOtherBins[vIndex];
  }
}


class bpHistogramBuilderAdaptive
{
public:
  bpHistogramBuilderAdaptive();
  ~bpHistogramBuilderAdaptive();

  bpHistogram GetHistogram() const;
  void AddValue(bpFloat aValue);
  void Merge(const bpHistogramBuilderAdaptive& aOther);

private:
  class cImpl;
  bpUniquePtr<cImpl> mImpl;
};


template<typename T>
class bpHistogramBuilder : public bpHistogramBuilderAdaptive
{
};


template<>
class bpHistogramBuilder<bpUInt8>
{
public:
  bpHistogram GetHistogram() const
  {
    return bpHistogram(0, 255, mBins);
  }

  void AddValue(bpUInt8 aValue)
  {
    ++mBins[aValue];
  }

  void Merge(const bpHistogramBuilder& aOther)
  {
    bpMergeBins(mBins, aOther.mBins);
  }

private:
  std::vector<bpUInt64> mBins = std::vector<bpUInt64>(256);
};


template<>
class bpHistogramBuilder<bpUInt16>
{
public:
  bpHistogram GetHistogram() const
  {
    bpUInt16 vMin = 0;
    bpUInt16 vMax = mBins.size() - 1;
    for (; vMin + 256 < vMax && mBins[vMax] == 0; --vMax);
    //for (; vMin + 256 < vMax && mBins[vMin] == 0; ++vMin); // let it start from zero
    std::vector<bpUInt64> vBins(mBins.begin() + vMin, mBins.begin() + (vMax + 1));
    return bpHistogram(vMin, vMax, std::move(vBins));
  }

  void AddValue(bpUInt16 aValue)
  {
    ++mBins[aValue];
  }

  void Merge(const bpHistogramBuilder& aOther)
  {
    bpMergeBins(mBins, aOther.mBins);
  }

private:
  std::vector<bpUInt64> mBins = std::vector<bpUInt64>(256 * 256);
};


#endif
