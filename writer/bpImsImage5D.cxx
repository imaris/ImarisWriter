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

#include "bpImsImage5D.h"


template<typename TDataType>
bpImsImage5D<TDataType>::bpImsImage5D(bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ, bpSize aSizeC, bpSize aSizeT,
  bpSize aBlockSizeX, bpSize aBlockSizeY, bpSize aBlockSizeZ, bpSharedPtr<bpMemoryManager<TDataType> > aManager)
{
  // initialize vector mImages with 3D images
  mImages.resize(aSizeT);
  for (bpSize vIndexT = 0; vIndexT < aSizeT; vIndexT++) {
    std::vector<bpImsImage3D<TDataType> >& vChannels = mImages[vIndexT];
    vChannels.reserve(aSizeC);
    for (bpSize vIndexC = 0; vIndexC < aSizeC; vIndexC++) {
      vChannels.emplace_back(aSizeX, aSizeY, aSizeZ, aBlockSizeX, aBlockSizeY, aBlockSizeZ, aManager);
    }
  }
}


template<typename TDataType>
bpImsImage5D<TDataType>::~bpImsImage5D()
{
}


template<typename TDataType>
bpSize bpImsImage5D<TDataType>::GetSizeT() const
{
  return mImages.size();
}


template<typename TDataType>
bpSize bpImsImage5D<TDataType>::GetSizeC() const
{
  return mImages[0].size();
}


template<typename TDataType>
void bpImsImage5D<TDataType>::CopyData(bpSize aIndexT, bpSize aIndexC, bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlockXY)
{
  bpImsImage3D<TDataType>& vImage3D = mImages[aIndexT][aIndexC];
  vImage3D.CopyData(aIndexZ, aBeginXY, aEndXY, aDataBlockXY);
}

template<typename TDataType>
bool bpImsImage5D<TDataType>::PadBorderBlockWithZeros(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ, bpSize aIndexC, bpSize aIndexT)
{
  bpImsImage3D<TDataType>& vImage3D = mImages[aIndexT][aIndexC];
  return vImage3D.PadBorderBlockWithZeros(aBlockIndexX, aBlockIndexY, aBlockIndexZ);
}

template<typename TDataType>
bpImsImage3D<TDataType>& bpImsImage5D<TDataType>::GetImage3D(bpSize aIndexT, bpSize aIndexC)
{
  return mImages[aIndexT][aIndexC];
}


template<typename TDataType>
const bpImsImage3D<TDataType>& bpImsImage5D<TDataType>::GetImage3D(bpSize aIndexT, bpSize aIndexC) const
{
  return mImages[aIndexT][aIndexC];
}


template<typename TDataType>
bpImsImageBlock<TDataType>& bpImsImage5D<TDataType>::GetBlock(bpSize aIndexT, bpSize aIndexC, bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ)
{
  return mImages[aIndexT][aIndexC].GetBlock(aBlockIndexX, aBlockIndexY, aBlockIndexZ);
}


template class bpImsImage5D<bpUInt8>;
template class bpImsImage5D<bpUInt16>;
template class bpImsImage5D<bpUInt32>;
template class bpImsImage5D<bpFloat>;
