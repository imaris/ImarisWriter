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
