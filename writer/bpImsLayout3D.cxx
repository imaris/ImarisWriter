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
#include "bpImsLayout3D.h"


bpImsLayout3D::bpImsLayout3D(const bpVec3& aSize, const bpVec3& aMemoryBlockSize)
  : mSize(aSize),
  mMemoryBlockSize(aMemoryBlockSize),
  mNBlocksX((mSize[0] + mMemoryBlockSize[0] - 1) / mMemoryBlockSize[0]),
  mNBlocksY((mSize[1] + mMemoryBlockSize[1] - 1) / mMemoryBlockSize[1]),
  mNBlocksZ((mSize[2] + mMemoryBlockSize[2] - 1) / mMemoryBlockSize[2])
{
}

bpImsLayout3D::bpImsLayout3D(bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ, bpSize aBlockSizeX, bpSize aBlockSizeY, bpSize aBlockSizeZ)
  : mSize{aSizeX, aSizeY, aSizeZ},
  mMemoryBlockSize{aBlockSizeX, aBlockSizeY, aBlockSizeZ},
  mNBlocksX((mSize[0] + mMemoryBlockSize[0] - 1) / mMemoryBlockSize[0]),
  mNBlocksY((mSize[1] + mMemoryBlockSize[1] - 1) / mMemoryBlockSize[1]),
  mNBlocksZ((mSize[2] + mMemoryBlockSize[2] - 1) / mMemoryBlockSize[2])
{
}


bpImsLayout3D::~bpImsLayout3D()
{
}


const bpVec3& bpImsLayout3D::GetMemoryBlockSize() const
{
  return mMemoryBlockSize;
}


const bpVec3& bpImsLayout3D::GetImageSize() const
{
  return mSize;
}


bpVec3 bpImsLayout3D::GetNBlocks() const
{
  bpVec3 vNBlocks;
  vNBlocks[0] = mNBlocksX;
  vNBlocks[1] = mNBlocksY;
  vNBlocks[2] = mNBlocksZ;

  return vNBlocks;
}
