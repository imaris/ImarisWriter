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
#ifndef __BP_WRITER_HDF5__
#define __BP_WRITER_HDF5__

#include "bpWriter.h"
#include "bpImsLayout.h"
#include "bpImsUtils.h"

#include <hdf5.h>

#include <deque>


class bpWriterHDF5 : public bpWriter
{
public:
  bpWriterHDF5(
    const bpString& aFilename,
    const bpImsLayout& aImageLayout,
    bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType);

  virtual ~bpWriterHDF5();

  virtual void WriteDataBlock(
    const void* aData, bpSize aDataSize,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  virtual void WriteHistogram(const bpHistogram& aHistogram, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  virtual void WriteMetadata(
    const bpString& aApplicationName,
    const bpString& aApplicationVersion,
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tParameters& aParameters,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel);

  virtual void WriteThumbnail(const bpThumbnail& aThumbnail);

private:
  using tColor = bpConverterTypes::cColor;
  using tColorInfo = bpConverterTypes::cColorInfo;
  using tParameterMap = std::map<std::string, std::string>;

  hid_t GetFileId() const;

  class H5GroupId
  {
  public:
    H5GroupId();
    H5GroupId(hid_t aParentGroupId, const bpString& aGroupName);
    H5GroupId(const H5GroupId& aParentGroupId, const bpString& aGroupName);

    hid_t Get() const;

  private:
    class H5GroupIdImpl
    {
    public:
      H5GroupIdImpl(hid_t aParentGroupId, const bpString& aGroupName);
      ~H5GroupIdImpl();

      hid_t Get() const;

    private:
      //H5GroupIdImpl(const H5GroupIdImpl&); // not implemented

      hid_t Open(hid_t aParentGroupId, const bpString& aGroupName) const;

      hid_t mId;
    };

    bpSharedPtr<H5GroupIdImpl> mImpl;
  };

  class H5FileIdImpl
  {
  public:
    H5FileIdImpl(const bpString& aFileName);
    ~H5FileIdImpl();

    hid_t Get() const;

  private:
    //H5FileIdImpl(const H5FileIdImpl&); // not implemented

    hid_t Open(const bpString& aFileName) const;

    hid_t mId;
  };

  /**
  * Manage creation / providing of HDF5 Groups for IMS Files writing.
  * If multi-files writing disabled:
  *    - Create new main file / Get access to existing main file
  *    - Define Groups Structure in main file
  *    - Provide Channels' GroupIds from main File
  * If multi-files writing enabled:
  *    - Create new main file / Get access to existing main file
  *    - Create new external files / Get access to existing external files
  *    - Define Groups Structure in main file and in external files
  *    - Create external-links between files
  *    - Provide Channels' GroupIds from External File
  *
  * The following elements can be split and written into external files:
  *    - DataSet (DataSet folder, DataSetTimes folder, Thumbnail folder, ...)
  *    - ResolutionLevel folders
  *    - TimePoint folders
  *    - Channels folders
  * There is one boolean member for each this split type.
  * They are by default all enabled. They can be disabled by setting the member to false in the class constructor
  *
  * The split is done in function of the enabled parents folders split. e.g.:
  * If only mWriteTimePointInExternalFile is enabled, we would have the following structure:
  *    - file_t0.ims contains:
  *        - DataSet0/ResLevel0/TimePoint0
  *        - DataSet1/ResLevel0/TimePoint0
  *        - ...
  *    - file_t1.ims contains:
  *        - DataSet0/ResLevel0/TimePoint1
  *        - DataSet1/ResLevel0/TimePoint1
  *        - ...
  * If mWriteDataSetInExternalFile and mWriteTimePointInExternalFile are enabled, we would have the following structure:
  *    - file_dset0_t0.ims contains:
  *        - DataSet0/ResLevel0/TimePoint0
  *    - file_dset1_t0.ims contains:
  *        - DataSet1/ResLevel0/TimePoint0
  *    - file_dset0_t1.ims contains:
  *        - DataSet0/ResLevel0/TimePoint1
  *    - file_dset1_t1.ims contains:
  *        - DataSet1/ResLevel0/TimePoint1
  *    - ...
  *
  */
  class H5GroupsManager
  {
  public:
    H5GroupsManager(bpSharedPtr<H5FileIdImpl> aMainFile);
    H5GroupsManager(const H5GroupsManager& aOther);

    bool Set(bpSize aDataSetIndex, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

    hid_t GetMainFileId() const;
    hid_t GetChannelGroupId() const;

    static bpString GetDataSetFileName(bpString aMainFileName, bpSize aDataSetIndex);
    static bpString GetResolutionLevelFileName(bpString aMainFileName, bpSize aDataSetIndex, bpSize aIndexR);
    static bpString GetTimePointFileName(bpString aMainFileName, bpSize aDataSetIndex, bpSize aIndexR, bpSize aIndexT);
    static bpString GetChannelFileName(bpString aMainFileName, bpSize aDataSetIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC);

  private:

    bpString GetDataSetGroupName() const;
    bpString GetResolutionLevelGroupPathName() const;
    bpString GetResolutionLevelGroupName() const;
    bpString GetTimePointGroupPathName() const;
    bpString GetTimePointGroupName() const;
    bpString GetChannelGroupPathName() const;
    bpString GetChannelGroupName() const;

    void SetDataSet(bpSize aDataSetIndex);
    void SetResolutionLevel(bpSize aIndexR);
    void SetTimePointGroup(bpSize aIndexT);
    void SetChannelGroup(bpSize aIndexC);

    bpSharedPtr<H5FileIdImpl> mMainFile;

    bpSize mDataSetIndex;
    bpSize mIndexT;
    bpSize mIndexC;
    bpSize mIndexR;

    H5GroupId mDataSetId;
    H5GroupId mResolutionLevelId;
    H5GroupId mTimePointGroupId;
    H5GroupId mChannelGroupId;
  };

  struct cDataSetCache
  {
    ~cDataSetCache()
    {
      Close();
    }

    hid_t mDataId = H5I_INVALID_HID;

    void Close();
  };

  void AllocateGroupsManager(bpSize aIndexR);
  hid_t GetChannelGroupId(bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  bpSize WriteDatasetHeader();

  void WriteHistogramImpl(const bpHistogram& aHistogram, const bpString& aSuffix, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  //void WriteTimes(const hid_t& aGroupId, const bpTimes& aTimes);

  class cAttributeWriter;
  class cDataSetWriter;

  static void WriteAttribute(const bpString& aAttributeName, const bpString& aAttribute, const H5GroupId& aAttributeLocation, bool aLongAttributesToDataSet = false);
  static void WriteAttribute(const bpString& aAttributeName, const bpString& aAttribute, const hid_t& aAttributeLocation, bool aLongAttributesToDataSet = false);
  static void WriteAttribute(const bpString& aAttributeName, const void* aAttributeValue, const hsize_t& aAttributeSize, const hid_t& aAttributeType, const H5GroupId& aAttributeLocation);
  static void WriteAttribute(const bpString& aAttributeName, const void* aAttributeValue, const hsize_t& aAttributeSize, const hid_t& aAttributeType, const hid_t& aAttributeLocation, bool aLongAttributesToDataSet = false);
  template<bool TWriteAsDataSet>
  static void WriteAttributeT(const bpString& aAttributeName, const void* aAttributeValue, const hsize_t& aAttributeSize, const hid_t& aAttributeType, const hid_t& aAttributeLocation);

  static bool GroupExists(const hid_t& aLocationId, const bpString& aDirectoryName);

  static bpString EncodeName(bpString aName);

  void WriteDataChunk(
    const void* aData, bpSize aDataSize,
    bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
    bpSize aIndexT, bpSize aIndexC, bpSize aIndexR);

  bool IsCompressionAlgorithmGzip() const;
  bool IsCompressionAlgorithmLZ4() const;
  bool IsCompressionAlgorithmShuffle() const;
  void GetGzipParameters(bpInt32& aCompressionLevel) const;
  bpSize GetBlockSizeBytes(bpSize aIndexR) const;

  static bpString GetDirectoryName(const bpString& aDirectoryName, bpSize aImageIndex);

  void UpdateImageMetadata(
    const bpString& aApplicationName,
    const bpString& aApplicationVersion,
    const bpConverterTypes::cImageExtent& aImageExtent,
    const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
    const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
    bpConverterTypes::tParameters& aParameters);

  bpString bpConvColorToString(const tColor& aColor) const;
  bpString bpConvTableToString(const tColorInfo& aColorInfo) const;
  void bpStoreColorInParameters(tParameterMap& aColorParameters, const tColorInfo& aColorInfo) const;

  std::deque<H5GroupsManager> mGroupsManager;
  std::deque<cDataSetCache> mDataSetCache;

  bpConverterTypes::tCompressionAlgorithmType mCompressionAlgorithmType;
  bpImsLayout mImageLayout;

  bpSize mDatasetIndex;
};

#endif // __BP_WRITER_HDF5__
