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
#include "bpImsUtils.h"

#include <sstream>


bpString bpImsUtils::TimeInfoToString(const bpConverterTypes::cTimeInfo& aTimeInfo)
{
  bpInt32 vYear;
  bpInt32 vMonth;
  bpInt32 vDay;
  FromJulianDay(aTimeInfo.mJulianDay, vYear, vMonth, vDay);
  bpInt32 vHour = GetHour(aTimeInfo.mNanosecondsOfDay);
  bpInt32 vMinute = GetMinute(aTimeInfo.mNanosecondsOfDay);
  bpInt32 vSecond = GetSecond(aTimeInfo.mNanosecondsOfDay);
  return DateTimeToString(vYear, vMonth, vDay, vHour, vMinute, vSecond);
}


bpString bpImsUtils::DateTimeToString(bpInt32 aYear, bpInt32 aMonth, bpInt32 aDay, bpInt32 aHour, bpInt32 aMinute, bpInt32 aSecond)
{
  return DateToString(aYear, aMonth, aDay) + " " + TimeToString(aHour, aMinute, aSecond);
}


bpString bpImsUtils::bpToString(bpUInt64 value)
{
  std::ostringstream vStr;
  vStr << value;
  return vStr.str();
}


void bpImsUtils::FromJulianDay(bpInt32 aJulianDay, bpInt32& aYear, bpInt32& aMonth, bpInt32& aDay)
{
  long t1 = aJulianDay + 68569L;
  long t2 = 4L * t1 / 146097L;
  t1 -= (146097L * t2 + 3L) / 4L;

  long y = 4000L * (t1 + 1) / 1461001L;
  t1 = t1 - 1461L * y / 4L + 31;

  long m = 80L * t1 / 2447L;
  aDay = bpInt32(t1 - 2447L * m / 80L);

  t1 = m / 11L;
  aMonth = bpInt32(m + 2L - 12L * t1);

  aYear = bpInt32(100L * (t2 - 49L) + y + t1);
}


bpString bpImsUtils::ToString(bpInt64 aValue, bpUInt16 aDecimals, bool aLeadingZeros)
{
  bpString vString = bpToString(aValue);
  bpSize vLength = vString.length();
  if (vLength < aDecimals) {
    for (bpInt32 vIndex = 0; vIndex < static_cast<bpInt32>(aDecimals - vLength); vIndex++) {
      if (aLeadingZeros) {
        vString = "0" + vString;
      }
      else {
        vString = vString + "0";
      }
    }
  }
  return vString;
}


bpInt32 bpImsUtils::GetHour(bpInt64 aNanosecondsOfDay)
{
  bpInt64 vHour = aNanosecondsOfDay / (60LL * 60 * 1000 * 1000 * 1000);
  return static_cast<bpInt32>(vHour);
}


bpInt32 bpImsUtils::GetMinute(bpInt64 aNanosecondsOfDay)
{
  bpInt64 vMin = (aNanosecondsOfDay / (60LL * 1000 * 1000 * 1000)) % 60;
  return static_cast<bpInt32>(vMin);
}


bpInt32 bpImsUtils::GetSecond(bpInt64 aNanosecondsOfDay)
{
  bpInt64 vSec = (aNanosecondsOfDay / (1000 * 1000 * 1000)) % 60;
  return static_cast<bpInt32>(vSec);
}


bpInt32 bpImsUtils::GetMillisecond(bpInt64 aNanosecondsOfDay)
{
  bpInt64 vMsec = (aNanosecondsOfDay / (1000 * 1000)) % 1000;
  return static_cast<bpInt32>(vMsec, 3);
}


bpString bpImsUtils::TimeToString(bpInt32 aHour, bpInt32 aMinute, bpInt32 aSecond)
{
  bpString vTime = ToString(aHour) + ":" + ToString(aMinute) + ":" + ToString(aSecond);
  return vTime;
}


bpString bpImsUtils::DateToString(bpInt32 aYear, bpInt32 aMonth, bpInt32 aDay)
{
  bpString vDate = ToString(aYear) + "-" + ToString(aMonth) + "-" + ToString(aDay);
  return vDate;
}

