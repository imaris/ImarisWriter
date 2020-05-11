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
#ifndef __BP_CIRCULAR_BUFFER_H__
#define __BP_CIRCULAR_BUFFER_H__


#include "../interface/bpConverterTypes.h"


template <class T>
class bpCircularBuffer
{
public:
  bpCircularBuffer(bpSize aSize);
  ~bpCircularBuffer() = default;

  void Add(T aItem);

  T Get(bpSize aIndex) const;

  bpSize GetHeadIndex() const;
  bpSize GetTailIndex() const;

  bpSize GetPreviousIndex(bpSize aIndex) const;

  bool Empty() const;

  bpSize Size() const;

private:
  bpSize mSize;
  std::vector<T> mBuffer;

  bpSize mHead;
  bpSize mTail;
};


template <class T>
bpCircularBuffer<T>::bpCircularBuffer(bpSize aSize)
  : mSize(aSize),
  mHead(0),
  mTail(0)
{
  mBuffer.resize(aSize);
}


template <class T>
void bpCircularBuffer<T>::Add(T aItem)
{
  mBuffer[mHead] = aItem;
  mHead = (mHead + 1) % mSize;

  if (mHead == mTail) {
    mTail = (mTail + 1) % mSize;
  }
}


template <class T>
T bpCircularBuffer<T>::Get(bpSize aIndex) const
{
  if (Empty()) {
    return T();
  }

  return mBuffer[aIndex % mSize];
}


template <class T>
bpSize bpCircularBuffer<T>::GetHeadIndex() const
{
  return mHead;
}


template <class T>
bpSize bpCircularBuffer<T>::GetTailIndex() const
{
  return mTail;
}


template <class T>
bpSize bpCircularBuffer<T>::GetPreviousIndex(bpSize aIndex) const
{
  return (aIndex + mSize - 1) % mSize;
}


template <class T>
bool bpCircularBuffer<T>::Empty() const
{
  return mHead == mTail;
}


template <class T>
bpSize bpCircularBuffer<T>::Size() const
{
  return mSize - 1;
}


#endif // __BP_CIRCULAR_BUFFER_H__
