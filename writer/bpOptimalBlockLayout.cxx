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
#include "bpOptimalBlockLayout.h"


#include <cmath>
#include <vector>
#include <algorithm>


typedef float bpFloat;
typedef unsigned long long bpUInt64;


class bpCBlockLayoutCost
{
public:
  bpSize mSizeX;
  bpSize mSizeY;
  bpSize mSizeZ;
  bpSize mSizeT;
  bpFloat mCostSlice;
  bpFloat mCostGeometry;
  bpFloat mCostMemory;
};


class bpCBlockLayoutCostLess
{
public:
  bool operator()(const bpCBlockLayoutCost& aBlockLayoutA, const bpCBlockLayoutCost& aBlockLayoutB) {
    if (aBlockLayoutA.mCostSlice != aBlockLayoutB.mCostSlice) {
      return aBlockLayoutA.mCostSlice < aBlockLayoutB.mCostSlice;
    }
    else if (aBlockLayoutA.mCostGeometry != aBlockLayoutB.mCostGeometry) {
      return aBlockLayoutA.mCostGeometry < aBlockLayoutB.mCostGeometry;
    }
    else {
      return aBlockLayoutA.mCostMemory < aBlockLayoutB.mCostMemory;
    }
  }
};


void GetOptimalBlockSize(
  bpSize aImageBlockSize,
  bpSize aImageSizeX,
  bpSize aImageSizeY,
  bpSize aImageSizeZ,
  bpSize aImageSizeT,
  bpSize aMinBlockSizeX,
  bpSize aMinBlockSizeY,
  bpSize aMinBlockSizeZ,
  bpSize aMinBlockSizeT,
  bpSize& aBlockSizeX,
  bpSize& aBlockSizeY,
  bpSize& aBlockSizeZ,
  bpSize& aBlockSizeT)
{
  bpUInt64 vBlockSize = static_cast<bpUInt64>(powf(2.0f, floorf(logf(static_cast<bpFloat>(aImageBlockSize)) / logf(2.0f))));

  bpUInt64 vImageSizeX = aImageSizeX;
  bpUInt64 vImageSizeY = aImageSizeY;
  bpUInt64 vImageSizeZ = aImageSizeZ;
  bpUInt64 vImageSizeT = aImageSizeT;

  bpUInt64 vImageSizeXYZT = vImageSizeX * vImageSizeY * vImageSizeZ * vImageSizeT;

  // compile a list of all possible layouts
  typedef std::vector<bpCBlockLayoutCost> tBlockLayoutCosts;
  tBlockLayoutCosts vBlockLayoutCosts;

  // layouts must have power of two sizes
  for (bpSize vBlockSizeX = 1; vBlockSizeX <= vBlockSize; vBlockSizeX *= 2) {
    for (bpSize vBlockSizeY = 1; vBlockSizeY * vBlockSizeX <= vBlockSize; vBlockSizeY *= 2) {
      for (bpSize vBlockSizeZ = 1; vBlockSizeZ * vBlockSizeY * vBlockSizeX <= vBlockSize; vBlockSizeZ *= 2) {
        bpSize vBlockSizeT = (bpSize)(vBlockSize / (vBlockSizeX * vBlockSizeY * vBlockSizeZ));
        //        if (vBlockSizeT == 1) {
        if ((aImageSizeZ > 1 && vBlockSizeX == vBlockSizeY &&  vBlockSizeZ > 2) ||   // some graphics boards want square textures and z > 2 for 3D
          (aImageSizeZ == 1 && ((vBlockSizeX <= 4 * vBlockSizeY) && (vBlockSizeY <= 4 * vBlockSizeX)) && vBlockSizeZ == 1)) { // or 2D
          if (vBlockSizeX * vBlockSizeY * vBlockSizeZ * vBlockSizeT == vBlockSize) {
            if (vBlockSizeX >= aMinBlockSizeX && vBlockSizeY >= aMinBlockSizeY && vBlockSizeZ >= aMinBlockSizeZ && vBlockSizeT >= aMinBlockSizeT) {
              bpUInt64 vNumberOfBlocksX = 1 + (vImageSizeX - 1) / vBlockSizeX;
              bpUInt64 vNumberOfBlocksY = 1 + (vImageSizeY - 1) / vBlockSizeY;
              bpUInt64 vNumberOfBlocksZ = 1 + (vImageSizeZ - 1) / vBlockSizeZ;
              bpUInt64 vNumberOfBlocksT = 1 + (vImageSizeT - 1) / vBlockSizeT;

              bpCBlockLayoutCost vBlockLayoutCost;

              vBlockLayoutCost.mSizeX = vBlockSizeX;
              vBlockLayoutCost.mSizeY = vBlockSizeY;
              vBlockLayoutCost.mSizeZ = vBlockSizeZ;
              vBlockLayoutCost.mSizeT = vBlockSizeT;

              // the costs for rendering the 3 main orthogonal slices together
              vBlockLayoutCost.mCostSlice = 1.0f
                + vNumberOfBlocksX * vNumberOfBlocksY
                + vNumberOfBlocksX * vNumberOfBlocksZ
                + vNumberOfBlocksY * vNumberOfBlocksZ
                - vNumberOfBlocksX
                - vNumberOfBlocksY
                - vNumberOfBlocksZ;

              // surface area (minimum is a cube), important for clipping
              vBlockLayoutCost.mCostGeometry = (bpFloat)(vBlockSizeX * vBlockSizeY +
                vBlockSizeX * vBlockSizeZ +
                vBlockSizeY * vBlockSizeZ);

              // memory usage: 1 means that no memory is wasted / larger values indicate blocks with unused voxels
              vBlockLayoutCost.mCostMemory = (bpFloat)(vNumberOfBlocksX * vBlockSizeX *
                vNumberOfBlocksY * vBlockSizeY *
                vNumberOfBlocksZ * vBlockSizeZ *
                vNumberOfBlocksT * vBlockSizeT) / (bpFloat)vImageSizeXYZT;

              // add layout to the list
              vBlockLayoutCosts.push_back(vBlockLayoutCost);
            }
          }
        }
        //        }
      }
    }
  }
  if (!vBlockLayoutCosts.empty()) {
    // sort the list (first by slice cost, then by geometry cost, then by memory cost)
    std::sort(vBlockLayoutCosts.begin(), vBlockLayoutCosts.end(), bpCBlockLayoutCostLess());

    // the first is the best ...
    bpCBlockLayoutCost vBlockLayoutCostOptimum = *vBlockLayoutCosts.begin();

    // ... if it doesn't waste to much memory
    const bpFloat vCostMemoryMax = 2.0f;
    if (vBlockLayoutCostOptimum.mCostMemory > vCostMemoryMax) {
      tBlockLayoutCosts::const_iterator vBlockLayoutPos = vBlockLayoutCosts.begin();
      while (vBlockLayoutPos != vBlockLayoutCosts.end()) {
        if ((*vBlockLayoutPos).mCostMemory <= vCostMemoryMax) {
          vBlockLayoutCostOptimum = *vBlockLayoutPos;
          break;
        }
        ++vBlockLayoutPos;
      }
    }

    // return result
    aBlockSizeX = vBlockLayoutCostOptimum.mSizeX;
    aBlockSizeY = vBlockLayoutCostOptimum.mSizeY;
    aBlockSizeZ = vBlockLayoutCostOptimum.mSizeZ;
    aBlockSizeT = vBlockLayoutCostOptimum.mSizeT;
  }
  else {
    // paranoid:  divide equally if no optimum available
    bpSize vLog2BlockSize = 0;
    while ((((bpSize)1) << vLog2BlockSize) < aImageBlockSize) {
      ++vLog2BlockSize;
    }
    bpSize vLog2BlockSizeZ = vLog2BlockSize / 3;
    bpSize vLog2BlockSizeY = (vLog2BlockSize - vLog2BlockSizeZ) / 2;
    bpSize vLog2BlockSizeX = (vLog2BlockSize - vLog2BlockSizeZ - vLog2BlockSizeY);
    aBlockSizeZ = (bpSize)1 << (vLog2BlockSizeZ);
    aBlockSizeY = (bpSize)1 << (vLog2BlockSizeY);
    aBlockSizeX = (bpSize)1 << (vLog2BlockSizeX);
    aBlockSizeT = 1;
  }
}


std::vector<bpVec3> GetOptimalBlockSizes(
  bpSize aImageBlockSize,
  const std::vector<bpVec3>& aResolutionSizes,
  bpSize aSizeT)
{
  std::vector<bpVec3> vResolutionBlockSizes;
  bpSize vResolutionLevels = aResolutionSizes.size();

  for (bpSize vIndex = 0; vIndex < vResolutionLevels; vIndex++) {

    bpSize vImageSizeX = aResolutionSizes[vIndex][0];
    bpSize vImageSizeY = aResolutionSizes[vIndex][1];
    bpSize vImageSizeZ = aResolutionSizes[vIndex][2];
    bpSize vImageSizeT = aSizeT;  // Need to be sizeT, not 1

    bpSize vMinBlockSizeX = 1;
    bpSize vMinBlockSizeY = 1;
    bpSize vMinBlockSizeZ = 1;
    bpSize vMinBlockSizeT = 1;

    if (vIndex > 0) {
      bpSize vLargeSizeX = aResolutionSizes[vIndex - 1][0];
      bpSize vLargeSizeY = aResolutionSizes[vIndex - 1][1];
      bpSize vLargeSizeZ = aResolutionSizes[vIndex - 1][2];
      bool vReduceX = (vLargeSizeX / 2) == aResolutionSizes[vIndex][0];
      bool vReduceY = (vLargeSizeY / 2) == aResolutionSizes[vIndex][1];
      bool vReduceZ = (vLargeSizeZ / 2) == aResolutionSizes[vIndex][2];

      vMinBlockSizeX = vReduceX ? vResolutionBlockSizes[vIndex - 1][0] / 2 : vResolutionBlockSizes[vIndex - 1][0];
      vMinBlockSizeY = vReduceY ? vResolutionBlockSizes[vIndex - 1][1] / 2 : vResolutionBlockSizes[vIndex - 1][1];
      vMinBlockSizeZ = vReduceZ ? vResolutionBlockSizes[vIndex - 1][2] / 2 : vResolutionBlockSizes[vIndex - 1][2];
    }

    bpSize vMemBlockSizeX;
    bpSize vMemBlockSizeY;
    bpSize vMemBlockSizeZ;
    bpSize vMemBlockSizeT;

    GetOptimalBlockSize(aImageBlockSize, vImageSizeX, vImageSizeY, vImageSizeZ, vImageSizeT,
      vMinBlockSizeX, vMinBlockSizeY, vMinBlockSizeZ, vMinBlockSizeT, vMemBlockSizeX, vMemBlockSizeY, vMemBlockSizeZ, vMemBlockSizeT);
    bpVec3 vMemoryBlockSize = { vMemBlockSizeX, vMemBlockSizeY, vMemBlockSizeZ };
    vResolutionBlockSizes.push_back(vMemoryBlockSize);
  }

  return vResolutionBlockSizes;
}


std::vector<bpVec3> GetOptimalImagePyramid(bpSize aMaxImagSize, const bpVec3& aImageSize, bool aReduceZ)
{
  std::vector<bpVec3> vResult{ aImageSize };

  bpVec3 vSize = aImageSize;
  while (static_cast<bpUInt64>(vSize[0]) * vSize[1] * vSize[2] > aMaxImagSize) {
    bpUInt64 vLargeSizeX = vSize[0];
    bpUInt64 vLargeSizeY = vSize[1];
    bpUInt64 vLargeSizeZ = aReduceZ ? vSize[2] : 1;
    bool vReduceX = vLargeSizeX > 1 && (10 * vLargeSizeX) * (10 * vLargeSizeX) > vLargeSizeY * vLargeSizeZ;
    bool vReduceY = vLargeSizeY > 1 && (10 * vLargeSizeY) * (10 * vLargeSizeY) > vLargeSizeX * vLargeSizeZ;
    bool vReduceZ = vLargeSizeZ > 1 && (10 * vLargeSizeZ) * (10 * vLargeSizeZ) > vLargeSizeX * vLargeSizeY;
    if (!vReduceX && !vReduceY && !vReduceZ) {
      break;
    }
    if (vReduceX) {
      vSize[0] /= 2;
    }
    if (vReduceY) {
      vSize[1] /= 2;
    }
    if (vReduceZ) {
      vSize[2] /= 2;
    }
    vResult.push_back(vSize);
  }

  return vResult;
}
