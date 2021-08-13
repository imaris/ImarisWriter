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
#ifndef __BP_IMAGE_CONVERTER_INTERFACE__
#define __BP_IMAGE_CONVERTER_INTERFACE__


#include "bpConverterTypes.h"

class bpHistogram;

template<typename TDataType>
class bpImageConverterInterface
{
public:
  virtual ~bpImageConverterInterface() = default;

  /**
   * CopyBlock makes a copy of data at aData and uses this copy for writing to file.
   *
   * A call to CopyBlock automatically advances the internal data position of the writer as specified by
   * FileBlockSize and DimensionSequence in the constructor.
   */
  virtual void CopyBlock(const TDataType* aData, const bpConverterTypes::tIndex5D& aBlockIndex) = 0;

  virtual void Finish(
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
    bool aAutoAdjustColorRange) = 0;

  /**
  * When ImageConverter sampling in the constructor is set up to subsample the image it is possible that some input
  * blocks are not needed. NeedCopyBlock allows to query whether the "current block" is needed.
  * The code if (NeedCopyBlock()) {CopyBlock(aData);}; correctly advances the internal data position in the same way as
  * a call to CopyBlock(aData) does.
  */
  virtual bool NeedCopyBlock(const bpConverterTypes::tIndex5D& aBlockIndex) const = 0;


};

#endif // __BP_IMAGE_CONVERTER_INTERFACE__
