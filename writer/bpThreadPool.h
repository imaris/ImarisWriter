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
#ifndef __BP_THREAD_POOL__
#define __BP_THREAD_POOL__


#include "../interface/bpConverterTypes.h"

#include <functional>


class bpThreadPool
{
public:
  explicit bpThreadPool(bpSize aNumberOfThreads);

  using tFunction = std::function<void()>;
  using tCallback = std::function<void()>;

  void Run(tFunction aFuntion, tCallback aCallback = {}, bool aHighPriority = false);

  void WaitOne();

  void WaitAll();

  bpSize WaitSome(bpSize aMaxNumberOfWaitingFunctions);

  void CallFinishedCallbacks();

private:
  class cImpl;
  bpSharedPtr<cImpl> mImpl;
};

#endif
