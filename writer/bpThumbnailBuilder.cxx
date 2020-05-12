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
#include "bpThumbnailBuilder.h"

#include <algorithm>


bpSize bpComputeThumbnailIndexR(bpSize aThumbnailSizeXY, const std::vector<bpVec3>& aImageResolutionSizes)
{
  bpSize vResolution = aImageResolutionSizes.size() - 1;
  while ((aImageResolutionSizes[vResolution][0] < aThumbnailSizeXY || aImageResolutionSizes[vResolution][1] < aThumbnailSizeXY) && vResolution > 0) {
    --vResolution;
  }
  return vResolution;
}

std::pair<bpSize, bpSize> bpComputeThumbnailSizeXY(bpSize aThumbnailSizeXY, const bpVec3& aImageSize, const bpFloatVec3& aImageExtents)
{
  // find size with isotropic pixels
  bpFloat vVoxelSizeX = aImageExtents[0] / aImageSize[0];
  bpFloat vVoxelSizeY = aImageExtents[1] / aImageSize[1];
  bpSize vIsotropicSizeX = vVoxelSizeX > vVoxelSizeY ? (bpSize)(aImageExtents[0] / vVoxelSizeY) : aImageSize[0];
  bpSize vIsotropicSizeY = vVoxelSizeY > vVoxelSizeX ? (bpSize)(aImageExtents[1] / vVoxelSizeX) : aImageSize[1];

  // compute new size (maintain ratio from x and y)
  bpFloat vResampleFactorX = static_cast<bpFloat>(aThumbnailSizeXY) / static_cast<bpFloat>(vIsotropicSizeX);
  bpFloat vResampleFactorY = static_cast<bpFloat>(aThumbnailSizeXY) / static_cast<bpFloat>(vIsotropicSizeY);
  bpFloat vResampleFactor = std::min(vResampleFactorX, vResampleFactorY);
  bpSize vNewSizeX = static_cast<bpSize>(vIsotropicSizeX * vResampleFactor + 0.5f);
  bpSize vNewSizeY = static_cast<bpSize>(vIsotropicSizeY * vResampleFactor + 0.5f);
  if (vNewSizeX == 0) vNewSizeX = 1;
  if (vNewSizeY == 0) vNewSizeY = 1;

  return{ vNewSizeX, vNewSizeY };
}

bpFloat bpComputeThumbnailQuality(const bpThumbnail& aThumbnail)
{
  bpSize vSizeX = aThumbnail.GetSizeX();
  bpSize vSizeY = aThumbnail.GetSizeY();
  bpSize vSize = vSizeX * vSizeY;
  std::vector<bpFloat> vLuminance(vSize);
  bpDouble vSum = 0;
  bpDouble vSquaresSum = 0;
  for (bpSize vIndexY = 0; vIndexY < vSizeY; ++vIndexY) {
    for (bpSize vIndexX = 0; vIndexX < vSizeX; ++vIndexX) {
      bpFloat vValue =
        0.299f * aThumbnail.GetR(vIndexX, vIndexY) +
        0.587f * aThumbnail.GetG(vIndexX, vIndexY) +
        0.114f * aThumbnail.GetB(vIndexX, vIndexY);
      vLuminance[vIndexX + vIndexY * vSizeX] = vValue;
      vSum += vValue;
      vSquaresSum += vValue * vValue;
    }
  }
  std::sort(vLuminance.begin(), vLuminance.end());

  // check luminance distribution
  bpFloat vQualityLuminance = 0.001f;
  bpFloat vDelta = vLuminance.back() - vLuminance.front();
  if (vDelta > 0.0f) {
    // quality is high, if bright and darke areas in image are equaliy sized
    // quality is low, if very image large areas of image are either bright or dark
    bpFloat vHalf = (vLuminance.front() + vLuminance.back()) / 2.0f;
    vQualityLuminance = 1.0f - 2.0f * fabs(vLuminance[vSize / 2] - vHalf) / vDelta;
    vQualityLuminance = std::min(std::max(vQualityLuminance, 0.001f), 1.0f);
  }

  // check if values are well distributed
  bpFloat vMean = vSum / vSize;
  bpFloat vQualityLuminanceDistribution = static_cast<bpFloat>(sqrt(std::max(vSquaresSum / vSize - vMean * vMean, 0.0)) / 255);
  vQualityLuminanceDistribution = std::min(std::max(vQualityLuminanceDistribution, 0.001f), 1.0f);

  // look at gradients, if the image has some structures, a histogram with gradient values
  // would certainly show such charakteristics. todo ...
  bpFloat vQualityGradients = 1.0f;

  // mix the quality values
  bpFloat vQuality = pow(vQualityLuminance * vQualityLuminanceDistribution * vQualityGradients, 1.0f / 3.0f);
  return vQuality;
}
