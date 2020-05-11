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

#include "bpImsImageBlock.h"


template<typename TDataType>
bpImsImageBlock<TDataType>::bpImsImageBlock(bpSize aSize, bpSharedPtr<bpMemoryManager<TDataType> > aManager)
  : mSize(aSize), mManager(aManager)
{
}


template<typename TDataType>
bpImsImageBlock<TDataType>::~bpImsImageBlock()
{
  ReleaseMemory();
}

template<typename TDataType>
void bpImsImageBlock<TDataType>::CopyLinePartToBlock(bpSize aBeginOffsetInBlock, bpSize aRangeToCopy, const TDataType* aDataBlockXLinePart)
{
  TDataType* vBeginInBuffer = GetData() + aBeginOffsetInBlock;

  if (aDataBlockXLinePart) {
    const TDataType* vDataBlockPartStart = aDataBlockXLinePart;
    const TDataType* vDataBlockPartEnd = vDataBlockPartStart + aRangeToCopy;
    std::copy(vDataBlockPartStart, vDataBlockPartEnd, vBeginInBuffer);
  }
  else {
    std::fill(vBeginInBuffer, vBeginInBuffer + aRangeToCopy, 0);
  }
}


template<typename TDataType>
typename bpImsImageBlock<TDataType>::tData bpImsImageBlock<TDataType>::ReleaseMemory()
{
  tData vData = mData;
  mData = {};
  mSize = 0;
  return vData;
}


template<typename TDataType>
TDataType* bpImsImageBlock<TDataType>::GetData()
{
  if (mSize == 0) throw 0;
  if (mData.GetSize() != mSize) {
    mData = mManager->GetMemory(mSize);
  }

  return mData.GetData();
}


template<typename TDataType>
const TDataType* bpImsImageBlock<TDataType>::GetData() const
{
  return mData.GetData();
}


template class bpImsImageBlock<bpUInt8>;
template class bpImsImageBlock<bpUInt16>;
template class bpImsImageBlock<bpUInt32>;
template class bpImsImageBlock<bpFloat>;
