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
#ifndef __BP_IMS_UTILS__
#define __BP_IMS_UTILS__

#include "../interface/ImarisWriterDllAPI.h"
#include "../interface/bpConverterTypes.h"

class bpImsUtils
{
public:
  static bpString TimeInfoToString(const bpConverterTypes::cTimeInfo& aTimeInfo);
  static bpString DateTimeToString(bpInt32 aYear, bpInt32 aMonth, bpInt32 aDay, bpInt32 aHour, bpInt32 aMinute, bpInt32 aSecond, bpInt32 aMillisecond = 0);

  static bpString bpToString(bpUInt64 value);

private:
  static void FromJulianDay(bpInt32 aJulianDay, bpInt32& aYear, bpInt32& aMonth, bpInt32& aDay);

  static bpString DateToString(bpInt32 aYear, bpInt32 aMonth, bpInt32 aDay);
  static bpString TimeToString(bpInt32 aHour, bpInt32 aMinute, bpInt32 aSecond, bpInt32 aMillisecond);

  static bpString ToString(bpInt64 aValue, bpUInt16 aDecimals = 2, bool aLeadingZeros = true);

  static bpInt32 GetHour(bpInt64 aNanosecondsOfDay);
  static bpInt32 GetMinute(bpInt64 aNanosecondsOfDay);
  static bpInt32 GetSecond(bpInt64 aNanosecondsOfDay);
  static bpInt32 GetMillisecond(bpInt64 aNanosecondsOfDay);
};


#endif // __BP_IMS_UTILS__
