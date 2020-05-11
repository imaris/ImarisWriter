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
#ifndef __BP_IMS_LAYOUT_3D__
#define __BP_IMS_LAYOUT_3D__

#include "../interface/bpConverterTypes.h"

class bpImsLayout3D
{
public:

  bpImsLayout3D(const bpVec3& aSize, const bpVec3& aMemoryBlockSize);
  bpImsLayout3D(bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ, bpSize aBlockSizeX, bpSize aBlockSizeY, bpSize aBlockSizeZ);
  ~bpImsLayout3D();

  const bpVec3& GetMemoryBlockSize() const;
  const bpVec3& GetImageSize() const;

  bpVec3 GetNBlocks() const;

  bpSize GetBlockIndex(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ) {
    return mNBlocksX*mNBlocksY*aBlockIndexZ + mNBlocksX * aBlockIndexY + aBlockIndexX;
  }

private:
  const bpVec3 mMemoryBlockSize;
  const bpVec3 mSize;

  const bpSize mNBlocksX;
  const bpSize mNBlocksY;
  const bpSize mNBlocksZ;
};

#endif // __BP_IMS_LAYOUT_3D__
