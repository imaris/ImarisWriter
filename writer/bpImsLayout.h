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
#ifndef __BP_IMS_LAYOUT__
#define __BP_IMS_LAYOUT__

#include "bpImsLayout3D.h"
#include "../interface/bpConverterTypes.h"


class bpImsLayout
{

public:

  bpImsLayout(
    const std::vector<bpVec3>& aResolutionSizes, bpSize aSizeT, bpSize aSizeC,
    const std::vector<bpVec3>& aResolutionBlockSizes, bpConverterTypes::tDataType aDataType);

  ~bpImsLayout();

  bpSize GetNumberOfTimePoints() const;
  bpSize GetNumberOfChannels() const;
  bpSize GetNumberOfResolutionLevels() const;

  const bpVec3& GetBlockSize(bpSize aIndexR) const;
  const bpVec3& GetImageSize(bpSize aIndexR) const;

  bpVec3 GetNBlocks(bpSize aIndexR) const;

  bpConverterTypes::tDataType GetDataType() const;

private:
  bpSize mSizeR;  // number of resolution levels
  bpSize mSizeT;  // number of timepoints
  bpSize mSizeC;  // number of channels

  bpConverterTypes::tDataType mDataType;

  // a layout 3D for each resolution level
  std::vector<bpImsLayout3D> mLayout3D;
};

#endif // __BP_IMS_LAYOUT__
