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
#include "bpImsLayout.h"


bpImsLayout::bpImsLayout(
  const std::vector<bpVec3>& aResolutionSizes, bpSize aSizeT, bpSize aSizeC,
  const std::vector<bpVec3>& aResolutionBlockSizes, bpConverterTypes::tDataType aDataType)
  : mSizeR(aResolutionSizes.size()), mSizeT(aSizeT), mSizeC(aSizeC), mDataType(aDataType)
{
  bpSize vResolutionLevels = aResolutionSizes.size();
  mLayout3D.reserve(vResolutionLevels);
  for (bpSize vIndexR = 0; vIndexR < vResolutionLevels; vIndexR++) {
    mLayout3D.emplace_back(aResolutionSizes[vIndexR], aResolutionBlockSizes[vIndexR]);
  }
}


bpImsLayout::~bpImsLayout()
{
}


bpSize bpImsLayout::GetNumberOfTimePoints() const
{
  return mSizeT;
}


bpSize bpImsLayout::GetNumberOfChannels() const
{
  return mSizeC;
}


bpSize bpImsLayout::GetNumberOfResolutionLevels() const
{
  return mLayout3D.size();
}


const bpVec3& bpImsLayout::GetBlockSize(bpSize aIndexR) const
{
  return mLayout3D[aIndexR].GetMemoryBlockSize();
}


const bpVec3& bpImsLayout::GetImageSize(bpSize aIndexR) const
{
  return mLayout3D[aIndexR].GetImageSize();
}


bpVec3 bpImsLayout::GetNBlocks(bpSize aIndexR) const
{
  return mLayout3D[aIndexR].GetNBlocks();
}


bpConverterTypes::tDataType bpImsLayout::GetDataType() const
{
  return mDataType;
}
