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
#ifndef __BP_MEMORY_BLOCK__
#define __BP_MEMORY_BLOCK__

#include "../interface/bpConverterTypes.h"

#include <functional>


class bpMemoryBlockCleanUp
{
public:
  using tCleanUp = std::function<void()>;

  using tPtr = bpSharedPtr<bpMemoryBlockCleanUp>;

  static tPtr Create(tCleanUp&& aCleanUp)
  {
    return std::make_shared<bpMemoryBlockCleanUp>(std::forward<tCleanUp&&>(aCleanUp));
  }

  explicit bpMemoryBlockCleanUp(tCleanUp&& aCleanUp)
    : mCleanUp(aCleanUp)
  {
  }

  ~bpMemoryBlockCleanUp()
  {
    mCleanUp();
  }

private:
  tCleanUp mCleanUp;
};


class bpBaseMemoryBlock
{
public:
  using tCleanUp = bpMemoryBlockCleanUp::tCleanUp;

  bpBaseMemoryBlock()
    : mSize(0)
  {
  }

  bpBaseMemoryBlock(bpSize aSize, tCleanUp&& aCleanUp)
    : mSize(aSize),
      mCleanUp(bpMemoryBlockCleanUp::Create(std::forward<tCleanUp&&>(aCleanUp)))
  {
  }

  bpSize GetSize() const
  {
    return mSize;
  }

private:
  bpSize mSize;
  bpSharedPtr<bpMemoryBlockCleanUp> mCleanUp;
};


/**
* Memory block in RAM.
*/
template<typename TDataType>
class bpMemoryBlock : public bpBaseMemoryBlock
{
public:
  bpMemoryBlock()
    : mData(nullptr)
  {
  }

  bpMemoryBlock(TDataType* aData, bpSize aSize, tCleanUp aCleanUp)
    : mData(aData),
      bpBaseMemoryBlock(aSize, std::move(aCleanUp))
  {
  }

  TDataType* GetData() const
  {
    return mData;
  }

private:
  TDataType* mData;
};


/**
* Memory block in RAM.
*/
template<typename TDataType>
class bpConstMemoryBlock : public bpBaseMemoryBlock
{
public:
  bpConstMemoryBlock()
    : mData(nullptr)
  {
  }

  bpConstMemoryBlock(const TDataType* aData, bpSize aSize, tCleanUp aCleanUp)
    : mData(aData),
      bpBaseMemoryBlock(aSize, std::move(aCleanUp))
  {
  }

  bpConstMemoryBlock(const bpMemoryBlock<TDataType>& aOther)
    : bpBaseMemoryBlock(aOther),
      mData(aOther.GetData())
  {
  }

  const TDataType* GetData() const
  {
    return mData;
  }

private:
  const TDataType* mData;
};


/**
* Memory block in RAM.
*/
class bpMemoryHandle
{
public:
  using tType = char;

  bpMemoryHandle()
    : mSize(0),
      mData(nullptr)
  {
  }

  template<typename TDataType>
  bpMemoryHandle(const bpConstMemoryBlock<TDataType>& aOther)
    : mSize(aOther.GetSize() * sizeof(TDataType) / sizeof(tType)),
      mData(reinterpret_cast<const tType*>(aOther.GetData())),
      mBaseMemoryBlock(aOther)
  {
  }

  bpSize GetSize() const
  {
    return mSize;
  }

  const tType* GetData() const
  {
    return mData;
  }

private:
  bpSize mSize;
  const tType* mData;
  bpBaseMemoryBlock mBaseMemoryBlock;
};

#endif // __BP_MEMORY_BLOCK__
