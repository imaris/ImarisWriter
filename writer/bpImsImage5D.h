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
#ifndef __BP_IMS_IMAGE_5D__
#define __BP_IMS_IMAGE_5D__

#include "bpImsImage3D.h"
#include "bpMemoryManager.h"


/**
* Ims image representing the dimensions X,Y,Z,C,T on one resolution level.
*/
template<typename TDataType>
class bpImsImage5D
{
public:

  bpImsImage5D(bpSize aSizeX, bpSize aSizeY, bpSize aSizeZ, bpSize aSizeC, bpSize aSizeT,
    bpSize aBlockSizeX, bpSize aBlockSizeY, bpSize aBlockSizeZ, bpSharedPtr<bpMemoryManager<TDataType> > aManager);

  bpImsImage5D(const bpImsImage5D&) = delete;
  //bpImsImage5D& operator=(const bpImsImage5D&) = delete;
  bpImsImage5D(bpImsImage5D&&) = default;

  ~bpImsImage5D();

  bpSize GetSizeT() const;
  bpSize GetSizeC() const;

  void CopyData(bpSize aIndexT, bpSize aIndexC, bpSize aIndexZ, const bpVec2& aBeginXY, const bpVec2& aEndXY, const TDataType* aDataBlockXY);

  bpImsImage3D<TDataType>& GetImage3D(bpSize aIndexT, bpSize aIndexC);

  const bpImsImage3D<TDataType>& GetImage3D(bpSize aIndexT, bpSize aIndexC) const;

  bpImsImageBlock<TDataType>& GetBlock(bpSize aIndexT, bpSize aIndexC, bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ);

  bool PadBorderBlockWithZeros(bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ, bpSize aIndexC, bpSize aIndexT);
private:

  // 3D images (for dimensions X,Y,Z) for each timepoint and channel, e.g. mImages[timeIndex][channelIndex]
  std::vector<std::vector<bpImsImage3D<TDataType> > > mImages;
};

#endif // __BP_IMS_IMAGE_5D__
