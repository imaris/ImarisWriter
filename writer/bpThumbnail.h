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
#ifndef __BP_THUMBNAIL__
#define __BP_THUMBNAIL__


#include "../interface/bpConverterTypes.h"


class bpThumbnail
{
public:
  bpThumbnail(bpSize aSizeX, bpSize aSizeY, std::vector<bpUInt8> aRGBA)
    : mSizeX(aSizeX),
      mRGBA(std::move(aRGBA))
  {
    if (mRGBA.size() != aSizeX * aSizeY * 4) {
      throw "Invalid data size";
    }
  }

  bpSize GetSizeX() const
  {
    return mSizeX;
  }

  bpSize GetSizeY() const
  {
    return mRGBA.size() / (mSizeX * 4);
  }

  bpUInt8 GetR(bpSize aIndexX, bpSize aIndexY) const
  {
    return mRGBA[(aIndexX + aIndexY * mSizeX) * 4 + 0];
  }

  bpUInt8 GetG(bpSize aIndexX, bpSize aIndexY) const
  {
    return mRGBA[(aIndexX + aIndexY * mSizeX) * 4 + 1];
  }

  bpUInt8 GetB(bpSize aIndexX, bpSize aIndexY) const
  {
    return mRGBA[(aIndexX + aIndexY * mSizeX) * 4 + 2];
  }

  bpUInt8 GetA(bpSize aIndexX, bpSize aIndexY) const
  {
    return mRGBA[(aIndexX + aIndexY * mSizeX) * 4 + 3];
  }

  const bpUInt8* GetRGBAPointer() const
  {
    return mRGBA.data();
  }

private:
  bpSize mSizeX;
  std::vector<bpUInt8> mRGBA;
};

#endif
