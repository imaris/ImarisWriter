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
#include "bpMemoryManager.h"

#include <mutex>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

#ifdef BP_DEBUG
#ifdef _WIN32
#include <Windows.h>
#endif
#endif

template<typename TDataType>
class bpMemoryManager<TDataType>::cImpl
{
public:
  using tData = std::pair<TDataType*, bpSize>;

  bpMemoryBlock<TDataType> GetMemory(bpSize aSize)
  {
    tData vData{ nullptr, 0 };

    {
      std::unique_lock<std::mutex> vLock(mMutex);
      if (!mDataCache.empty()) {
        vData = std::move(mDataCache.top());
        mDataCache.pop();

        if (aSize > vData.second) {
          mMemoryReallocated += vData.second;
          mMemoryCapacity += aSize - vData.second;
        }
      }
      else {
        mMemoryCapacity += aSize;
      }

      mMemoryRequested += aSize;
    }

    if (aSize > vData.second) {
      delete[] vData.first;
      vData.first = new TDataType[aSize];
      vData.second = aSize;
    }

    return bpMemoryBlock<TDataType>(vData.first, aSize, [vData, this]() { ReturnMemory(vData); });
  }

  ~cImpl()
  {
////#ifdef BP_DEBUG
//#ifdef _WIN32
//    std::ostringstream vOut;
//    vOut << "Amount of memory requested: " << mMemoryRequested * sizeof(TDataType) / 1024 / 1024 << " MB\n";
//    vOut << "Amount of memory allocated: " << mMemoryCapacity * sizeof(TDataType) / 1024 / 1024 << " MB\n";
//    vOut << "Amount of memory reallocated: " << mMemoryReallocated * sizeof(TDataType) / 1024 / 1024 << " MB\n";
//    bpString vText = vOut.str();
//
//    std::cout << vText;
//    //OutputDebugString(vText.c_str());
//#endif
////#endif

    while (!mDataCache.empty()) {
      delete[] mDataCache.top().first;
      mDataCache.pop();
    }
  }

private:
  void ReturnMemory(tData aData)
  {
    std::unique_lock<std::mutex> vLock(mMutex);
    mDataCache.emplace();
    mDataCache.top().swap(aData);
  }

  std::mutex mMutex;
  std::stack<tData> mDataCache;
  bpSize mMemoryCapacity = 0;
  bpSize mMemoryRequested = 0;
  bpSize mMemoryReallocated = 0;
};


template<typename TDataType>
bpMemoryManager<TDataType>::bpMemoryManager()
  : mImpl(std::make_shared<cImpl>())
{
}


template<typename TDataType>
bpMemoryBlock<TDataType> bpMemoryManager<TDataType>::GetMemory(bpSize aSize)
{
  return mImpl->GetMemory(aSize);
}


template class bpMemoryManager<bpUInt8>;
template class bpMemoryManager<bpUInt16>;
template class bpMemoryManager<bpUInt32>;
template class bpMemoryManager<bpFloat>;
