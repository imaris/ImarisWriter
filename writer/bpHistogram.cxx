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
#include "bpHistogram.h"


bpHistogram bpResampleHistogram(const bpHistogram& aHistogram, bpSize aNewNumberOfBins)
{
  bpSize vOldNumberOfBins = aHistogram.GetNumberOfBins();
  if (vOldNumberOfBins == aNewNumberOfBins) {
    return aHistogram;
  }

  std::vector<bpUInt64> vBins(aNewNumberOfBins);
  for (bpSize vI = 0; vI < vOldNumberOfBins; ++vI) {
    bpSize vJ = vI * aNewNumberOfBins / vOldNumberOfBins;
    vBins[vJ] += aHistogram.GetCount(vI);
  }
  return bpHistogram(aHistogram.GetMin(), aHistogram.GetMax(), vBins);
}


class bpHistogramBuilderAdaptive::cImpl
{
public:
  explicit cImpl(bpSize aNumberOfBins = 1000)
    : mCountTotal(0),
      mBinCounts(aNumberOfBins),
      mValueMin(bpFloat()),
      mValueMax(bpFloat()),
      mBinValueMin(bpFloat()),
      mBinValueMax(bpFloat())
  {
  }

  bpSize GetNumberOfBins() const
  {
    return mBinCounts.size();
  }

  bpUInt64 GetCount(bpSize aBinId) const
  {
    return mBinCounts[aBinId];
  }

  bpUInt64 GetCountTotal() const
  {
    return mCountTotal;
  }

  bool Empty() const
  {
    return mCountTotal == 0;
  }

  bpFloat GetValueDelta() const
  {
    return (mBinValueMax - mBinValueMin) / GetNumberOfBins();
  }

  bpFloat GetBinValueMin() const
  {
    return mBinValueMin;
  }

  bpFloat GetBinValueMax() const
  {
    return mBinValueMax;
  }

  bpFloat GetValue(bpSize aBinId) const
  {
    return static_cast<bpFloat>(mBinValueMin + (aBinId + 0.5f) * GetValueDelta());
  }

  bpSize GetBinId(const bpFloat& aValue) const
  {
    bpSizeSigned vBinId = GetBinIdSigned(aValue);
    if (vBinId < 0) {
      return 0;
    }
    else if (vBinId >= static_cast<bpSizeSigned>(GetNumberOfBins())) {
      return GetNumberOfBins() - 1;
    }
    else {
      return static_cast<bpSize>(vBinId);
    }
  }

  void AddValue(const bpFloat& aValue, bpUInt64 aCount = 1)
  {
    if (aCount == 0) {
      return;
    }

    if (Empty()) {
      mValueMin = aValue;
      mValueMax = aValue;
      mBinValueMin = aValue;
      mBinValueMax = aValue;
    }
    else {
      while (aValue < mBinValueMin) {
        ExpandLeft(aValue);
      }
      while (aValue > mBinValueMax || (mValueMin < mValueMax && aValue == mBinValueMax)) {
        ExpandRight(aValue);
      }
    }

    bpSize vBinId = GetBinId(aValue);
    mBinCounts[vBinId] += aCount;
    mCountTotal += aCount;
    if (aValue < mValueMin) {
      mValueMin = aValue;
    }
    if (aValue >= mValueMax) {
      mValueMax = aValue;
    }
  }

  void Merge(const cImpl& aOther)
  {
    // remember actual min and max
    bpFloat vValueMin = mValueMin;
    bpFloat vValueMax = mValueMax;
    // extend data range as much as needed by adding the last non-zero value to avoid subsequent expansions to the right
    bpSize vLastBinId = aOther.GetNumberOfBins() - 1;
    while (vLastBinId > 0 && aOther.GetCount(vLastBinId) == 0) {
      --vLastBinId;
    }
    AddValue(aOther.GetValue(vLastBinId), aOther.GetCount(vLastBinId));
    // add all the other values from other bins (might be, that min and max are exceeded)
    for (bpSize vBinId = 0; vBinId < vLastBinId; vBinId++) {
      AddValue(aOther.GetValue(vBinId), aOther.GetCount(vBinId));
    }
    // set the "true" min and max from both histograms
    mValueMin = std::min(vValueMin, aOther.mValueMin);
    mValueMax = std::max(vValueMax, aOther.mValueMax);
  }

  void Reset()
  {
    mCountTotal = 0;
    std::fill(mBinCounts.begin(), mBinCounts.end(), 0);
    mValueMin = bpFloat();
    mValueMax = bpFloat();
    mBinValueMin = bpFloat();
    mBinValueMax = bpFloat();
  }

private:
  template<typename T>
  static bpSizeSigned ToInt(T aValue)
  {
    return static_cast<bpSizeSigned>(floor(aValue));
  }

  void ShiftAndMergeBinCountsToLeft()
  {
    bpSize vCount = mBinCounts.size() / 2;
    for (bpSize vBinId = 0; vBinId < vCount; ++vBinId) {
      mBinCounts[vBinId] = mBinCounts[2 * vBinId] + mBinCounts[2 * vBinId + 1];
    }
    if (vCount * 2 < mBinCounts.size()) {
      mBinCounts[vCount] = mBinCounts[2 * vCount];
      ++vCount;
    }
    std::fill(mBinCounts.begin() + vCount, mBinCounts.end(), 0);
  }

  void ShiftAndMergeBinCountsToLeftWithOffset()
  {
    bpSize vCount = mBinCounts.size() / 2;
    mBinCounts[0] = mBinCounts[0];
    for (bpSize vBinId = 1; vBinId < vCount; ++vBinId) {
      mBinCounts[vBinId] = mBinCounts[2 * vBinId] + mBinCounts[2 * vBinId - 1];
    }
    mBinCounts[vCount] = mBinCounts[2 * vCount - 1];
    if (vCount * 2 < mBinCounts.size()) {
      mBinCounts[vCount] += mBinCounts[2 * vCount];
    }
    ++vCount;
    std::fill(mBinCounts.begin() + vCount, mBinCounts.end(), 0);
  }

  void ShiftAndMergeBinCountsToRight()
  {
    bpSize vSize = mBinCounts.size();
    bpSize vCount = vSize / 2;
    bpSize vLast = vSize - 1;
    for (bpSize vBinId = 0; vBinId < vCount; ++vBinId) {
      mBinCounts[vLast - vBinId] = mBinCounts[vLast - 2 * vBinId] + mBinCounts[vLast - 2 * vBinId - 1];
    }
    if (vCount * 2 < mBinCounts.size()) {
      mBinCounts[vLast - vCount] = mBinCounts[vLast - 2 * vCount];
      ++vCount;
    }
    std::fill(mBinCounts.begin(), mBinCounts.begin() + (vSize - vCount), 0);
  }

  void ShiftAndMergeBinCountsToRightWithOffset()
  {
    bpSize vSize = mBinCounts.size();
    bpSize vCount = vSize / 2;
    bpSize vLast = vSize - 1;
    mBinCounts[vLast] = mBinCounts[vLast];
    for (bpSize vBinId = 1; vBinId < vCount; ++vBinId) {
      mBinCounts[vLast - vBinId] = mBinCounts[vLast - 2 * vBinId] + mBinCounts[vLast - 2 * vBinId + 1];
    }
    mBinCounts[vLast - vCount] = mBinCounts[vLast - 2 * vCount + 1];
    if (vCount * 2 < mBinCounts.size()) {
      mBinCounts[vLast - vCount] += mBinCounts[vLast - 2 * vCount];
    }
    ++vCount;
    std::fill(mBinCounts.begin(), mBinCounts.begin() + (vSize - vCount), 0);
  }

  void MoveEmptyBinsToLeft()
  {
    bpSize vEmptyCount = 0;
    bpSize vSize = mBinCounts.size();
    bpSize vLast = vSize - 1;
    while (mBinCounts[vLast - vEmptyCount] == 0) {
      ++vEmptyCount;
    }
    if (vEmptyCount == 0) {
      return;
    }
    for (bpSize vBinId = vEmptyCount; vBinId < vSize; ++vBinId) {
      mBinCounts[vLast - vBinId + vEmptyCount] = mBinCounts[vLast - vBinId];
    }
    for (bpSize vBinId = 0; vBinId < vEmptyCount; ++vBinId) {
      mBinCounts[vBinId] = 0;
    }
    bpFloat vOffset = GetValueDelta() * vEmptyCount;
    mBinValueMin -= vOffset;
    mBinValueMax -= vOffset;
  }

  void MoveEmptyBinsToRight()
  {
    bpSize vEmptyCount = 0;
    bpSize vSize = mBinCounts.size();
    while (mBinCounts[vEmptyCount] == 0) {
      ++vEmptyCount;
    }
    if (vEmptyCount == 0) {
      return;
    }
    for (bpSize vBinId = vEmptyCount; vBinId < vSize; ++vBinId) {
      mBinCounts[vBinId - vEmptyCount] = mBinCounts[vBinId];
    }
    for (bpSize vBinId = vSize - vEmptyCount; vBinId < vSize; ++vBinId) {
      mBinCounts[vBinId] = 0;
    }
    bpFloat vOffset = GetValueDelta() * vEmptyCount;
    mBinValueMin += vOffset;
    mBinValueMax += vOffset;
  }

  void ExpandRight(const bpFloat& aValue)
  {
    if (mValueMin >= mValueMax) {
      ResetBins(mValueMin, aValue);
      return;
    }

    MoveEmptyBinsToRight();
    if (aValue < mBinValueMax) {
      return;
    }

    bpFloat vValueDelta = 2 * GetValueDelta();
    bpFloat vBinValueMin = ToInt(mBinValueMin / vValueDelta) * vValueDelta;
    bpFloat vBinValueMax = vBinValueMin + GetNumberOfBins() * vValueDelta;
    if (vBinValueMax > mBinValueMax) {
      bool vOffset = std::abs(vBinValueMin - mBinValueMin) > vValueDelta / 4;
      mBinValueMin = vBinValueMin;
      mBinValueMax = vBinValueMax;
      if (vOffset) {
        ShiftAndMergeBinCountsToLeftWithOffset();
      }
      else {
        ShiftAndMergeBinCountsToLeft();
      }
    }
    else {
      ResetBins(mValueMin, aValue);
    }
  }

  void ExpandLeft(const bpFloat& aValue)
  {
    if (mValueMin >= mValueMax) {
      ResetBins(aValue, mValueMax);
      return;
    }

    MoveEmptyBinsToLeft();
    if (aValue >= mBinValueMin) {
      return;
    }

    bpFloat vValueDelta = 2 * GetValueDelta();
    bpFloat vBinValueMax = ToInt(ceil(mBinValueMax / vValueDelta)) * vValueDelta;
    bpFloat vBinValueMin = vBinValueMax - GetNumberOfBins() * vValueDelta;
    if (vBinValueMin < mBinValueMin) {
      bool vOffset = std::abs(vBinValueMax - mBinValueMax) > vValueDelta / 4;
      mBinValueMin = vBinValueMin;
      mBinValueMax = vBinValueMax;
      if (vOffset) {
        ShiftAndMergeBinCountsToRightWithOffset();
      }
      else {
        ShiftAndMergeBinCountsToRight();
      }
    }
    else {
      ResetBins(aValue, mValueMax);
    }
  }

  static bool CanReduceBinSize(bpFloat aBinSize)
  {
    return aBinSize / 2 > 0 && aBinSize / 2 < aBinSize;
  }

  void ResetBins(bpFloat aValueMin, bpFloat aValueMax)
  {
    if (aValueMin >= aValueMax) {
      return;
    }

    bpSize vLast = mBinCounts.size() - 1;
    bpFloat vValueDelta = 1;
    while (CanReduceBinSize(vValueDelta) && ToInt(aValueMin / vValueDelta + vLast) > ToInt(aValueMax / vValueDelta)) {
      vValueDelta /= 2;
    }
    while (ToInt(aValueMin / vValueDelta + vLast) < ToInt(aValueMax / vValueDelta)) {
      vValueDelta *= 2;
    }

    mBinValueMin = ToInt(aValueMin / vValueDelta) * vValueDelta;
    mBinValueMax = mBinValueMin + (vLast + 1) * vValueDelta;
    std::fill(mBinCounts.begin(), mBinCounts.end(), 0);
    mBinCounts[GetBinId((mValueMin + mValueMax) * 0.5f)] = mCountTotal;
  }

  bpSizeSigned GetBinIdSigned(const bpFloat& aValue) const
  {
    bpFloat vValueDelta = GetValueDelta();
    if (vValueDelta == 0) {
      return static_cast<bpSizeSigned>(GetNumberOfBins() / 2);
    }
    return ToInt((aValue - mBinValueMin) / vValueDelta);
  }

  bpUInt64 mCountTotal;
  bpFloat mValueMin;
  bpFloat mValueMax;
  bpFloat mBinValueMin;
  bpFloat mBinValueMax;

  std::vector<bpUInt64> mBinCounts;
};


bpHistogramBuilderAdaptive::bpHistogramBuilderAdaptive()
  : mImpl(std::make_unique<cImpl>())
{
}

bpHistogramBuilderAdaptive::~bpHistogramBuilderAdaptive()
{
}

bpHistogram bpHistogramBuilderAdaptive::GetHistogram() const
{
  bpSize vSize = mImpl->GetNumberOfBins();
  bpSize vMin = 0;
  bpSize vMax = vSize - 1;
  for (; vMin + 256 < vMax && mImpl->GetCount(vMax) == 0; --vMax);
  for (; vMin + 256 < vMax && mImpl->GetCount(vMin) == 0; ++vMin);
  std::vector<bpUInt64> vBins(vMax - vMin + 1);
  for (bpSize vBinId = vMin; vBinId <= vMax; ++vBinId) {
    vBins[vBinId - vMin] = mImpl->GetCount(vBinId);
  }
  bpFloat vBinValueMin = mImpl->GetBinValueMin();
  bpFloat vBinValueMax = mImpl->GetBinValueMax();
  bpFloat vValueMin = vBinValueMin + (vBinValueMax - vBinValueMin) * vMin / (vSize - 1);
  bpFloat vValueMax = vBinValueMin + (vBinValueMax - vBinValueMin) * vMax / (vSize - 1);
  return bpHistogram(vValueMin, vValueMax, std::move(vBins));
}

void bpHistogramBuilderAdaptive::AddValue(bpFloat aValue)
{
  mImpl->AddValue(aValue);
}

void bpHistogramBuilderAdaptive::Merge(const bpHistogramBuilderAdaptive& aOther)
{
  mImpl->Merge(*aOther.mImpl);
}
