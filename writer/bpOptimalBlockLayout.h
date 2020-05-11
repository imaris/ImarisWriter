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
#ifndef BPOPTIMALBLOCKLAYOUT_H
#define BPOPTIMALBLOCKLAYOUT_H


#include "../interface/bpConverterTypes.h"


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
  bpSize& aBlockSizeT);

std::vector<bpVec3> GetOptimalBlockSizes(bpSize aImageBlockSize, const std::vector<bpVec3>& aResolutionSizes, bpSize aSizeT);

std::vector<bpVec3> GetOptimalImagePyramid(bpSize aMaxImagSize, const bpVec3& aImageSize, bool aReduceZ);


#endif
