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
#include "bpWriterHDF5.h"
#include "bpH5LZ4.h"
#include "bpCompressionAlgorithmFactory.h"

#include <iomanip>
#include <sstream>

static const bpString mDataSetDirectoryName = "DataSet";
static const bpString mDataSetTimesDirectoryName = "DataSetTimes";
static const bpString mDataSetInformationDirectoryName = "DataSetInfo";
static const bpString mThumbnailDirectoryName = "Thumbnail";


static hid_t ConvertToHDF5DataType(bpConverterTypes::tDataType aNumberType)
{
  hid_t vDataTypeId = 0;
  switch (aNumberType) {
  case bpConverterTypes::bpUInt8Type:
    vDataTypeId = H5Tcopy(H5T_NATIVE_UCHAR);
    break;
  case bpConverterTypes::bpUInt16Type:
    vDataTypeId = H5Tcopy(H5T_NATIVE_USHORT);
    break;
  case bpConverterTypes::bpUInt32Type:
    vDataTypeId = H5Tcopy(H5T_NATIVE_UINT32);
    break;
  case bpConverterTypes::bpFloatType:
    vDataTypeId = H5Tcopy(H5T_NATIVE_FLOAT);
    break;
  default:
    // uint8
    vDataTypeId = H5Tcopy(H5T_NATIVE_UCHAR);
    break;
  }
  return vDataTypeId;
}


static bpString bpReplace(const bpString& aString, const bpString& aSubStringOld, const bpString& aSubStringNew)
{
  if (aSubStringOld.empty()) {
    return aString;
  }
  bpString vString = aString;
  bpString vResult = "";
  bpSize vPos = vString.find(aSubStringOld);
  while (vPos != bpString::npos) {
    vResult += vString.substr(0, vPos);
    vResult += aSubStringNew;
    vString.erase(0, vPos + aSubStringOld.length());
    vPos = vString.find(aSubStringOld);
  }
  vResult += vString;
  return vResult;
}


static bool bpStartsWith(const bpString& aString, const bpString& aPrefix)
{
  if (aString.size() < aPrefix.size()) {
    return false;
  }
  return aString.compare(0, aPrefix.size(), aPrefix) == 0;
}



static bpString bpFloatToString(bpFloat aValue, bpInt16 aFixedPrecision = 3)
{
  std::ostringstream vOStringStream;
  if (aFixedPrecision >= 0) {
    vOStringStream.setf(std::ios_base::fixed);
    vOStringStream << std::setprecision(aFixedPrecision) << aValue;
  }
  else {
    vOStringStream << std::setprecision(std::numeric_limits<bpFloat>::digits10) << aValue;
  }
  return vOStringStream.str();
}


static void AdaptExtents(bpFloatVec3& aExtentMin, bpFloatVec3& aExtentMax, const bpVec3& aImageSize)
{
  for (bpSize vIndex = 0; vIndex < 3; vIndex++) {
    if (aExtentMax[vIndex] < aExtentMin[vIndex]) {
      float vTemp = aExtentMax[vIndex];
      aExtentMax[vIndex] = aExtentMin[vIndex];
      aExtentMin[vIndex] = vTemp;
    }
    if (aExtentMax[vIndex] == aExtentMin[vIndex]) {
      aExtentMax[vIndex] = aExtentMin[vIndex] + static_cast<bpFloat>(aImageSize[vIndex]);
    }
  }
}


bpWriterHDF5::bpWriterHDF5(
  const bpString& aFilename,
  const bpImsLayout& aImageLayout,
  bpConverterTypes::tCompressionAlgorithmType aCompressionAlgorithmType)
  : mGroupsManager(),
    mDatasetIndex(0),
    mImageLayout(aImageLayout),
    mCompressionAlgorithmType(aCompressionAlgorithmType)
{
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);

  if (IsCompressionAlgorithmLZ4()) {
    H5Zregister_lz4();
  }

  H5Eset_auto(H5E_DEFAULT, NULL, NULL);
  mGroupsManager.emplace_back(std::make_shared<H5FileIdImpl>(aFilename));
  if (GetFileId() == H5I_INVALID_HID) {
    throw bpError("bpWriterHDF5: Could not write to " + aFilename);
  }

  mDatasetIndex = WriteDatasetHeader();
}


bpWriterHDF5::~bpWriterHDF5()
{
  mGroupsManager.clear();
  mDataSetCache.clear();
}


hid_t bpWriterHDF5::GetFileId() const
{
  return mGroupsManager[0].GetMainFileId();
}


bpSize bpWriterHDF5::WriteDatasetHeader()
{
  H5GroupId vRootId(GetFileId(), "/");

  bpUInt32 vNumberOfDataSets = 1;

  bpString vIdentifier = "ImarisDataSet";
  WriteAttribute("ImarisDataSet", vIdentifier, vRootId);
  WriteAttribute("ImarisVersion", "5.5.0", vRootId);
  WriteAttribute("DataSetInfoDirectoryName", mDataSetInformationDirectoryName, vRootId);
  WriteAttribute("ThumbnailDirectoryName", mThumbnailDirectoryName, vRootId);
  WriteAttribute("DataSetDirectoryName", mDataSetDirectoryName, vRootId);
  WriteAttribute("NumberOfDataSets", &vNumberOfDataSets, 1, H5T_NATIVE_UINT32, vRootId);

  return 0;
}


bpString bpWriterHDF5::EncodeName(bpString aName)
{
  aName = bpReplace(aName, "%", "%p"); // must be first
  aName = bpReplace(aName, "/", "%s");
  return aName;
}


void bpWriterHDF5::WriteMetadata(
  const bpString& aApplicationName,
  const bpString& aApplicationVersion,
  const bpConverterTypes::cImageExtent& aImageExtent,
  const bpConverterTypes::tParameters& aParameters,
  const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
  const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel)
{
  if (GetFileId() == H5I_INVALID_HID) {
    throw bpError("bpWriterHDF5::WriteMetadata: The file was not opened properly.");
  }

  bpConverterTypes::tParameters vParameters(aParameters);
  UpdateImageMetadata(aApplicationName, aApplicationVersion, aImageExtent, aTimeInfoPerTimePoint, aColorInfoPerChannel, vParameters);

  bpString vDirectoryName = GetDirectoryName(mDataSetInformationDirectoryName, mDatasetIndex);
  hid_t vParentGroupId = GetFileId();
  H5GroupId vDataSetInfoId(vParentGroupId, vDirectoryName);
  if (vDataSetInfoId.Get() < 0) {
    throw bpError("bpWriterHDF5::RemoveAndRecreateGroup: Could not create group " + vDirectoryName);
  }

  bpConverterTypes::tParameters::const_iterator vSectionIt = vParameters.begin();
  for (; vSectionIt != vParameters.end(); ++vSectionIt) {
    H5GroupId vSectionId(vDataSetInfoId, EncodeName(vSectionIt->first));

    const tParameterMap& vSection = vSectionIt->second;
    tParameterMap::const_iterator vParameterIt = vSection.begin();
    for (; vParameterIt != vSection.end(); ++vParameterIt) {
      const bpString& vParameterName = vParameterIt->first;
      const bpString& vParameterValue = vParameterIt->second;
      if (!vParameterName.empty()) {
        WriteAttribute(vParameterName, vParameterValue, vSectionId, true);
      }
    }
  }
}


void bpWriterHDF5::WriteHistogram(const bpHistogram& aHistogram, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  bpHistogram vHistogram256 = bpResampleHistogram(aHistogram, 256);
  WriteHistogramImpl(vHistogram256, "", aIndexT, aIndexC, aIndexR);
  if (aHistogram.GetNumberOfBins() != 256) {
    WriteHistogramImpl(aHistogram, "1024", aIndexT, aIndexC, aIndexR);
  }
}


void bpWriterHDF5::AllocateGroupsManager(bpSize aIndexR)
{
  if (mGroupsManager.size() <= aIndexR) {
    mGroupsManager.resize(aIndexR + 1, mGroupsManager.front());
  }
  if (mDataSetCache.size() <= aIndexR) {
    mDataSetCache.resize(aIndexR + 1);
  }
}


hid_t bpWriterHDF5::GetChannelGroupId(bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  AllocateGroupsManager(aIndexR);
  bool vHasChanged = mGroupsManager[aIndexR].Set(mDatasetIndex, aIndexT, aIndexC, aIndexR);
  if (vHasChanged) {
    mDataSetCache[aIndexR].Close();
  }
  return mGroupsManager[aIndexR].GetChannelGroupId();
}


void bpWriterHDF5::WriteHistogramImpl(const bpHistogram& aHistogram, const bpString& aSuffix, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  if (GetFileId() == H5I_INVALID_HID) {
    throw bpError("bpWriterHDF5::WriteDataChunk: The file was not opened properly.");
  }

  hid_t vChannelGroupId = GetChannelGroupId(aIndexT, aIndexC, aIndexR);

  WriteAttribute("HistogramMin" + aSuffix, bpFloatToString(aHistogram.GetMin()), vChannelGroupId);
  WriteAttribute("HistogramMax" + aSuffix, bpFloatToString(aHistogram.GetMax()), vChannelGroupId);

  hsize_t vFileDim[1] = { aHistogram.GetNumberOfBins() };
  hid_t vFileSpaceId = H5Screate_simple(1, vFileDim, nullptr);
  hid_t vHistogramId = H5Dcreate(vChannelGroupId, ("Histogram" + aSuffix).c_str(), H5T_NATIVE_UINT64, vFileSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Sclose(vFileSpaceId);

  std::vector<bpUInt64> vBuffer(aHistogram.GetNumberOfBins());
  for (bpSize vBinIndex = 0; vBinIndex < aHistogram.GetNumberOfBins(); ++vBinIndex) {
    vBuffer[vBinIndex] = aHistogram.GetCount(vBinIndex);
  }

  if (H5Dwrite(vHistogramId, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, vBuffer.data()) < 0) {
    H5Dclose(vHistogramId);
    throw bpError("Imaris5 Writer: Could not write histogram to file!");
  }

  if (H5Dclose(vHistogramId) < 0) {
    throw bpError("Imaris5 Writer: Could not write histogram to file!");
  }
}


void bpWriterHDF5::WriteThumbnail(const bpThumbnail& aThumbnail)
{
  if (GetFileId() == H5I_INVALID_HID) {
    throw bpError("bpWriterHDF5::WriteThumbnail: The file was not opened properly.");
  }

  bpSize vSize = aThumbnail.GetSizeX() * aThumbnail.GetSizeY();
  if (vSize == 0) {
    return;
  }

  hid_t vParentGroupId = GetFileId();
  H5GroupId vThumbnailId(vParentGroupId, mThumbnailDirectoryName);
  if (vThumbnailId.Get() < 0) {
    throw bpError("bpWriterHDF5::WriteThumbnail: Could not create group");
  }

  hsize_t vThumbnailDims[2] = { aThumbnail.GetSizeY(), aThumbnail.GetSizeX() * 4 };
  hid_t vDataSpaceId = H5Screate_simple(2, vThumbnailDims, nullptr);
  hid_t vDataSetId = H5Dcreate(vThumbnailId.Get(), "Data", H5T_NATIVE_UCHAR, vDataSpaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (vDataSetId < 0) {
    vDataSetId = H5Dopen(vThumbnailId.Get(), "Data", H5P_DEFAULT);
    if (vDataSetId < 0) {
      throw bpError("bpWriterHDF5::WriteThumbnail: Could not open dataset");
    }
  }

  if (H5Dwrite(vDataSetId, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, aThumbnail.GetRGBAPointer()) < 0) {
    H5Dclose(vDataSetId);
    H5Sclose(vDataSpaceId);
    throw bpError("Imaris5 Writer: Could not write Thumbnail to file!");
  }

  H5Dclose(vDataSetId);
  H5Sclose(vDataSpaceId);
}

bool bpWriterHDF5::IsCompressionAlgorithmGzip() const
{
  switch (mCompressionAlgorithmType) {
  case bpConverterTypes::eCompressionAlgorithmNone:
  case bpConverterTypes::eCompressionAlgorithmLZ4:
  case bpConverterTypes::eCompressionAlgorithmShuffleLZ4:
    return false;
  default:
    return true;
  }
}


bool bpWriterHDF5::IsCompressionAlgorithmLZ4() const
{
  switch (mCompressionAlgorithmType) {
  case bpConverterTypes::eCompressionAlgorithmLZ4:
  case bpConverterTypes::eCompressionAlgorithmShuffleLZ4:
    return true;
  default:
    return false;
  }
}


bool bpWriterHDF5::IsCompressionAlgorithmShuffle() const
{
  switch (mCompressionAlgorithmType) {
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel1:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel2:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel3:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel4:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel5:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel6:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel7:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel8:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel9:
  case bpConverterTypes::eCompressionAlgorithmShuffleLZ4:
    return true;
  default:
    return false;
  }
}


void bpWriterHDF5::GetGzipParameters(bpInt32& aCompressionLevel) const
{
  switch (mCompressionAlgorithmType) {
  case bpConverterTypes::eCompressionAlgorithmNone:
  case bpConverterTypes::eCompressionAlgorithmLZ4:
  case bpConverterTypes::eCompressionAlgorithmShuffleLZ4:
    aCompressionLevel = 0;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel1:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel1:
    aCompressionLevel = 1;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel2:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel2:
    aCompressionLevel = 2;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel3:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel3:
    aCompressionLevel = 3;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel4:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel4:
    aCompressionLevel = 4;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel5:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel5:
    aCompressionLevel = 5;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel6:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel6:
    aCompressionLevel = 6;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel7:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel7:
    aCompressionLevel = 7;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel8:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel8:
    aCompressionLevel = 8;
    break;
  case bpConverterTypes::eCompressionAlgorithmGzipLevel9:
  case bpConverterTypes::eCompressionAlgorithmShuffleGzipLevel9:
    aCompressionLevel = 9;
    break;
  }
}


bpSize bpWriterHDF5::GetBlockSizeBytes(bpSize aIndexR) const
{
  static std::map<bpConverterTypes::tDataType, bpSize> vPixelSizeMap{
    { bpConverterTypes::bpNoType, 0 },
    { bpConverterTypes::bpInt8Type, 1 },
    { bpConverterTypes::bpUInt8Type, 1 },
    { bpConverterTypes::bpInt16Type, 2 },
    { bpConverterTypes::bpUInt16Type, 2 },
    { bpConverterTypes::bpInt32Type, 4 },
    { bpConverterTypes::bpUInt32Type, 4 },
    { bpConverterTypes::bpFloatType, 4 },
    { bpConverterTypes::bpDoubleType, 8 },
    { bpConverterTypes::bpInt64Type, 8 },
    { bpConverterTypes::bpUInt64Type, 8 }
  };

  const bpVec3 vBlockSize = mImageLayout.GetBlockSize(aIndexR);
  bpSize vPixelSize = vPixelSizeMap[mImageLayout.GetDataType()];
  return vPixelSize * vBlockSize[0] * vBlockSize[1] * vBlockSize[2];
}


void bpWriterHDF5::WriteDataBlock(
  const void* aData, bpSize aDataSize,
  bpSize aBlockIndexX, bpSize aBlockIndexY, bpSize aBlockIndexZ,
  bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  if (GetFileId() == H5I_INVALID_HID) {
    throw bpError("bpWriterHDF5::WriteDataChunk: The file was not opened properly.");
  }

  //we need to following information in order to write a block:
  // - block size
  // - number of blocks

  //ims file structure:
  //DataSet +
  //        +-ResolutionLevel 0 +
  //        |                   +- Time Index 0 +
  //        |                                   +- Channel Index 0 +
  //        |                                   |                  +- Image Data [x y z]
  //        |                                   +- Channel Index 1 +
  //        |                                                      +- Image Data [x y z]
  //        +-ResolutionLevel 1 +
  //                            +- Time Index 1 +
  //                                            +- Channel Index 0 +
  //                                            |                  +- Image Data [x y z]
  //                                            +- Channel Index 1 +
  //                                                               +- Image Data [x y z]

  // set the chunk size. NDLR: A chunk cannot be bigger than the size of data
  // the chunk size will be our block size but could be anything smaller, it does not matter and should be tested...
  hsize_t vHDF5ChunkSize[3]; // blockSize[z, y, x]
  hsize_t vNumHDF5Chunks[3]; // numberOfBlocks[z, y, x]
  hsize_t vHDF5FileSize[3]; // blockSize[z, y, x] * numberOfBlocks[z, y, x]

  const bpVec3& vBlockSize = mImageLayout.GetBlockSize(aIndexR);
  const bpVec3& vNBlocksOfR = mImageLayout.GetNBlocks(aIndexR);
  for (bpSize vIndex = 0; vIndex < 3; vIndex++) {
    vHDF5ChunkSize[vIndex] = vBlockSize[2 - vIndex];
    vNumHDF5Chunks[vIndex] = vNBlocksOfR[2 - vIndex];
    vHDF5FileSize[vIndex] = vHDF5ChunkSize[vIndex] * vNumHDF5Chunks[vIndex];
  }

  hid_t vChannelGroupId = GetChannelGroupId(aIndexT, aIndexC, aIndexR);
  bpString vDataSetName = "Data";

  hid_t vDataId = mDataSetCache[aIndexR].mDataId;
  if (vDataId == H5I_INVALID_HID) {
    if (GroupExists(vChannelGroupId, vDataSetName)) {
      vDataId = H5Dopen(vChannelGroupId, vDataSetName.c_str(), H5P_DEFAULT);
    }
    else {
      hid_t vDataTypeId = ConvertToHDF5DataType(mImageLayout.GetDataType());
      hid_t vFileSpaceId = H5Screate_simple(3, vHDF5FileSize, nullptr);

      const bpVec3 vImageSize = mImageLayout.GetImageSize(aIndexR);
      WriteAttribute("ImageSizeX", bpImsUtils::bpToString(vImageSize[0]), vChannelGroupId);
      WriteAttribute("ImageSizeY", bpImsUtils::bpToString(vImageSize[1]), vChannelGroupId);
      WriteAttribute("ImageSizeZ", bpImsUtils::bpToString(vImageSize[2]), vChannelGroupId);

      hid_t vPListId = H5Pcreate(H5P_DATASET_CREATE);
      H5Pset_chunk(vPListId, 3, vHDF5ChunkSize);
      if (IsCompressionAlgorithmShuffle() && bpCompressionAlgorithmFactory::GetNBytesShuffle(mImageLayout.GetDataType()) > 1){
        H5Pset_shuffle(vPListId);
      }

      if (IsCompressionAlgorithmGzip()) {
        bpInt32 vCompressionLevel;
        GetGzipParameters(vCompressionLevel);
        H5Pset_deflate(vPListId, vCompressionLevel);
      }

      if (IsCompressionAlgorithmLZ4()) {
        H5Pset_lz4(vPListId);
      }

      vDataId = H5Dcreate(vChannelGroupId, "Data", vDataTypeId, vFileSpaceId, H5P_DEFAULT, vPListId, H5P_DEFAULT);

      H5Pclose(vPListId);


      H5Tclose(vDataTypeId);
      H5Sclose(vFileSpaceId);
    }

    mDataSetCache[aIndexR].mDataId = vDataId;
  }

  if (vDataId < 0) {
    throw bpError("bpDataBlockReaderWriterImaris5::WriteDataBlock: Opening / Creating group failed.");
  }

  hsize_t vStart[] = {
    aBlockIndexZ * vHDF5ChunkSize[0],
    aBlockIndexY * vHDF5ChunkSize[1],
    aBlockIndexX * vHDF5ChunkSize[2]
  };

  herr_t vStatus;
  bool vUseNativeFilters = false;
  if (vUseNativeFilters) {
    // write with native API
    hid_t vDataTypeId = ConvertToHDF5DataType(mImageLayout.GetDataType());
    hid_t vMemSpaceId = H5Screate_simple(3, vHDF5ChunkSize, nullptr);
    hid_t vFileSpaceId = H5Dget_space(vDataId);
    H5Sselect_hyperslab(vFileSpaceId, H5S_SELECT_SET, vStart, nullptr, vHDF5ChunkSize, nullptr);
    vStatus = H5Dwrite(vDataId, vDataTypeId, vMemSpaceId, vFileSpaceId, H5P_DEFAULT, aData);
    H5Sclose(vMemSpaceId);
  }
  else {
    vStatus = H5Dwrite_chunk(vDataId, H5P_DEFAULT, 0, vStart, aDataSize, aData);
  }

  if (vStatus < 0) {
    throw bpError("bpDataBlockReaderWriterImaris5::WriteDataBlock: Could not write part of dataset in file!");
  }
}


void bpWriterHDF5::cDataSetCache::Close()
{
  if (mDataId != H5I_INVALID_HID) {
    H5Dclose(mDataId);
    mDataId = H5I_INVALID_HID;
  }
}


bpString bpWriterHDF5::GetDirectoryName(const bpString& aDirectoryName, bpSize aDatasetIndex)
{
  return aDirectoryName + (aDatasetIndex == 0 ? "" : bpImsUtils::bpToString(aDatasetIndex));
}


void bpWriterHDF5::UpdateImageMetadata(
  const bpString& aApplicationName,
  const bpString& aApplicationVersion,
  const bpConverterTypes::cImageExtent& aImageExtent,
  const bpConverterTypes::tTimeInfoVector& aTimeInfoPerTimePoint,
  const bpConverterTypes::tColorInfoVector& aColorInfoPerChannel,
  bpConverterTypes::tParameters& aParameters)
{

  if (!aApplicationName.empty() || !aApplicationVersion.empty()) {
    tParameterMap& vDataSetParameters = aParameters["ImarisDataSet"];

    vDataSetParameters["NumberOfImages"] = "1";
    vDataSetParameters["Creator"] = aApplicationName;
    vDataSetParameters["Version"] = aApplicationVersion;
  }

  if (bpStartsWith(aApplicationName, "Imaris")) {
    tParameterMap& vImarisParameters = aParameters["Imaris"];
    vImarisParameters["Version"] = aApplicationVersion;
  }

  tParameterMap& vLogParameters = aParameters["Log"];
  if (vLogParameters.empty()) {
    vLogParameters["Entries"] = "0";
  }

  tParameterMap& vImageParameters = aParameters["Image"];

  if (vImageParameters.find("Name") == vImageParameters.end()) {
    vImageParameters["Name"] = "(name not specified)";
  }
  if (vImageParameters.find("Description") == vImageParameters.end()) {
    vImageParameters["Description"] = "(description not specified)";
  }

  bpFloatVec3 vExtentMin{ aImageExtent.mExtentMinX, aImageExtent.mExtentMinY, aImageExtent.mExtentMinZ };
  bpFloatVec3 vExtentMax{ aImageExtent.mExtentMaxX, aImageExtent.mExtentMaxY, aImageExtent.mExtentMaxZ };
  AdaptExtents(vExtentMin, vExtentMax, mImageLayout.GetImageSize(0));

  vImageParameters["ExtMin0"] = bpFloatToString(vExtentMin[0], -1);
  vImageParameters["ExtMin1"] = bpFloatToString(vExtentMin[1], -1);
  vImageParameters["ExtMin2"] = bpFloatToString(vExtentMin[2], -1);
  vImageParameters["ExtMax0"] = bpFloatToString(vExtentMax[0], -1);
  vImageParameters["ExtMax1"] = bpFloatToString(vExtentMax[1], -1);
  vImageParameters["ExtMax2"] = bpFloatToString(vExtentMax[2], -1);

  bpVec3 vImageSize = mImageLayout.GetImageSize(0);
  vImageParameters["X"] = bpImsUtils::bpToString(vImageSize[0]);
  vImageParameters["Y"] = bpImsUtils::bpToString(vImageSize[1]);
  vImageParameters["Z"] = bpImsUtils::bpToString(vImageSize[2]);

  if (vImageParameters.find("Unit") == vImageParameters.end()) {
    vImageParameters["Unit"] = "um";
  }

  vImageParameters["ResampleDimensionX"] = "true";
  vImageParameters["ResampleDimensionY"] = "true";
  vImageParameters["ResampleDimensionZ"] = "true";

  bpString vRecordingDate = "";
  if (!aTimeInfoPerTimePoint.empty()) {
    vRecordingDate = bpImsUtils::TimeInfoToString(aTimeInfoPerTimePoint[0]);
  }
  vImageParameters["RecordingDate"] = vRecordingDate;

  tParameterMap& vTimeInfoParameters = aParameters["TimeInfo"];
  bpSize vNumberOfTimePoints = mImageLayout.GetNumberOfTimePoints();
  vTimeInfoParameters["FileTimePoints"] = bpImsUtils::bpToString(vNumberOfTimePoints);
  vTimeInfoParameters["DatasetTimePoints"] = bpImsUtils::bpToString(vNumberOfTimePoints);
  bpString vTimePointName = "";
  bpSize vTimeInfoSize = aTimeInfoPerTimePoint.size();
  for (bpSize vTimeIndex = 0; vTimeIndex < vNumberOfTimePoints; ++vTimeIndex) {
    vTimePointName = "TimePoint" + bpImsUtils::bpToString(vTimeIndex + 1);
    bpString vTimePoint = "";
    if (vTimeIndex < vTimeInfoSize) {
      vTimePoint = bpImsUtils::TimeInfoToString(aTimeInfoPerTimePoint[vTimeIndex]);
    }
    vTimeInfoParameters[vTimePointName] = vTimePoint;
  }

  bpSize vNumberOfChannels = mImageLayout.GetNumberOfChannels();
  for (bpSize vChannelIndex = 0; vChannelIndex < vNumberOfChannels; ++vChannelIndex) {
    tParameterMap& vChannelParameters = aParameters["Channel " + bpImsUtils::bpToString(vChannelIndex)];

    if (vChannelParameters.find("Name") == vChannelParameters.end()) {
      vChannelParameters["Name"] = "(name not specified)";
    }
    if (vChannelParameters.find("Description") == vChannelParameters.end()) {
      vChannelParameters["Description"] = "(description not specified)";
    }

    bpStoreColorInParameters(vChannelParameters, aColorInfoPerChannel[vChannelIndex]);
  }
}


void bpWriterHDF5::WriteAttribute(const bpString& aAttributeName, const bpString& aAttribute, const H5GroupId& aAttributeLocation, bool aLongAttributesToDataSet)
{
  WriteAttribute(aAttributeName, aAttribute, aAttributeLocation.Get(), aLongAttributesToDataSet);
}


void bpWriterHDF5::WriteAttribute(const bpString& aAttributeName, const bpString& aAttribute, const hid_t& aAttributeLocation, bool aLongAttributesToDataSet)
{
  WriteAttribute(aAttributeName, aAttribute.c_str(), aAttribute.size(), H5T_C_S1, aAttributeLocation, aLongAttributesToDataSet);
}


void bpWriterHDF5::WriteAttribute(const bpString& aAttributeName, const void* aAttributeValue, const hsize_t& aAttributeSize, const hid_t& aAttributeType, const H5GroupId& aAttributeLocation)
{
  WriteAttribute(aAttributeName, aAttributeValue, aAttributeSize, aAttributeType, aAttributeLocation.Get(), false);
}


void bpWriterHDF5::WriteAttribute(const bpString& aAttributeName, const void* aAttributeValue, const hsize_t& aAttributeSize, const hid_t& aAttributeType, const hid_t& aAttributeLocation, bool aLongAttributesToDataSet)
{
  if (aLongAttributesToDataSet && aAttributeSize > 1024 && aAttributeType == H5T_C_S1) {
    WriteAttributeT<true>(EncodeName(aAttributeName), aAttributeValue, aAttributeSize, aAttributeType, aAttributeLocation);
  }
  else {
    WriteAttributeT<false>(aAttributeName, aAttributeValue, aAttributeSize, aAttributeType, aAttributeLocation);
  }
}


class bpWriterHDF5::cAttributeWriter
{
public:
  static htri_t H5Aexists(hid_t obj_id, const char *attr_name)
  {
    return ::H5Aexists(obj_id, attr_name);
  }

  static herr_t H5Adelete(hid_t loc_id, const char *name)
  {
    return ::H5Adelete(loc_id, name);
  }

  static hid_t H5Acreate(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t acpl_id)
  {
    return ::H5Acreate(loc_id, name, type_id, space_id, acpl_id, H5P_DEFAULT);
  }

  static herr_t H5Awrite(hid_t attr_id, hid_t type_id, const void *buf)
  {
    return ::H5Awrite(attr_id, type_id, buf);
  }

  static herr_t H5Aclose(hid_t attr_id)
  {
    return ::H5Aclose(attr_id);
  }
};


class bpWriterHDF5::cDataSetWriter
{
public:
  static htri_t H5Aexists(hid_t obj_id, const char *attr_name)
  {
    return ::H5Lexists(obj_id, attr_name, H5P_DEFAULT);
  }

  static herr_t H5Adelete(hid_t loc_id, const char *name)
  {
    return ::H5Ldelete(loc_id, name, H5P_DEFAULT);
  }

  static hid_t H5Acreate(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t acpl_id)
  {
    return ::H5Dcreate(loc_id, name, type_id, space_id, H5P_DEFAULT, acpl_id, H5P_DEFAULT);
  }

  static herr_t H5Awrite(hid_t attr_id, hid_t type_id, const void *buf)
  {
    return ::H5Dwrite(attr_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
  }

  static herr_t H5Aclose(hid_t attr_id)
  {
    return ::H5Dclose(attr_id);
  }
};


template<bool TWriteAsDataSet>
void bpWriterHDF5::WriteAttributeT(const bpString& aAttributeName, const void* aAttributeValue, const hsize_t& aAttributeSize, const hid_t& aAttributeType, const hid_t& aAttributeLocation)
{
  using tWriter = std::conditional_t<TWriteAsDataSet, cDataSetWriter, cAttributeWriter>;

  hsize_t vAttributeSize = aAttributeSize;
  if (vAttributeSize == 0) {
    // sometimes attributes do not have a value... and size is 0, but the interface does not like it...
    vAttributeSize = 1;
  }
  // create a simple dataspace with rank 1
  hid_t vDataSpaceId = H5Screate_simple(1, &vAttributeSize, nullptr);
  if (vDataSpaceId < 0) {
    throw bpError("bpDataBlockReaderWriterImaris5::WriteDataBlock: Could not create attribute dataspace.");
  }


  htri_t vExists = tWriter::H5Aexists(aAttributeLocation, aAttributeName.c_str());
  hid_t vAttributeId = H5I_INVALID_HID;
  if (vExists > 0) {
    vAttributeId = tWriter::H5Adelete(aAttributeLocation, aAttributeName.c_str());
    if (vAttributeId < 0) {
      throw bpError("bpDataBlockReaderWriterImaris5::WriteDataBlock: Could not delete attribute.");
    }
  }
  vAttributeId = tWriter::H5Acreate(aAttributeLocation, aAttributeName.c_str(), aAttributeType, vDataSpaceId, H5P_DEFAULT);
  if (vAttributeId < 0) {
    throw bpError("bpDataBlockReaderWriterImaris5::WriteAttribute: Could not recreate attribute in file!");
  }
  // we can now write its value
  if (aAttributeValue != nullptr) {
    if (tWriter::H5Awrite(vAttributeId, aAttributeType, aAttributeValue) < 0) {
      throw bpError("bpDataBlockReaderWriterImaris5::WriteDataBlock: Could not write attribute " + aAttributeName);
    }
  }
  // close what we opened
  tWriter::H5Aclose(vAttributeId);
  H5Sclose(vDataSpaceId);
}


bool bpWriterHDF5::GroupExists(const hid_t& aLocationId, const bpString& aDirectoryName)
{
  hid_t vLaplId = H5Pcreate(H5P_LINK_ACCESS);
  htri_t vGroupExists = H5Lexists(aLocationId, aDirectoryName.c_str(), vLaplId);
  H5Pclose(vLaplId);
  return vGroupExists > 0;
}


bpWriterHDF5::H5GroupId::H5GroupId()
{
}


bpWriterHDF5::H5GroupId::H5GroupId(hid_t aParentGroupId, const bpString& aGroupName)
  : mImpl(new H5GroupIdImpl(aParentGroupId, aGroupName))
{
}


bpWriterHDF5::H5GroupId::H5GroupId(const H5GroupId& aParentGroupId, const bpString& aGroupName)
  : mImpl(new H5GroupIdImpl(aParentGroupId.Get(), aGroupName))
{
}


hid_t bpWriterHDF5::H5GroupId::Get() const
{
  return mImpl ? mImpl->Get() : H5I_INVALID_HID;
}


bpWriterHDF5::H5GroupId::H5GroupIdImpl::H5GroupIdImpl(hid_t aParentGroupId, const bpString& aGroupName)
  : mId(Open(aParentGroupId, aGroupName))
{
}


bpWriterHDF5::H5GroupId::H5GroupIdImpl::~H5GroupIdImpl()
{
  if (mId >= 0) {
    H5Gclose(mId);
  }
}


hid_t bpWriterHDF5::H5GroupId::H5GroupIdImpl::Open(hid_t aParentGroupId, const bpString& aGroupName) const
{
  if (aGroupName == "/") { // root group wants only H5GOpen
    return H5Gopen(aParentGroupId, "/", H5P_DEFAULT);
  }
  if (GroupExists(aParentGroupId, aGroupName)) {
    return H5Gopen(aParentGroupId, aGroupName.c_str(), H5P_DEFAULT);
  }
  else {
    return H5Gcreate(aParentGroupId, aGroupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
}


hid_t bpWriterHDF5::H5GroupId::H5GroupIdImpl::Get() const
{
  return mId;
}


bpWriterHDF5::H5FileIdImpl::H5FileIdImpl(const bpString& aFileName)
  : mId(Open(aFileName))
{
}


bpWriterHDF5::H5FileIdImpl::~H5FileIdImpl()
{
  if (mId >= 0) {
    H5Fclose(mId);
  }
}


hid_t bpWriterHDF5::H5FileIdImpl::Open(const bpString& aFileName) const
{
  hid_t vFileId = -1;
  hid_t vPropertyId = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_libver_bounds(vPropertyId, H5F_LIBVER_V18, H5F_LIBVER_V18);
  vFileId = H5Fcreate(aFileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, vPropertyId);
  H5Pclose(vPropertyId);
  if (vFileId < 0) {
    return vFileId;
  }
  // Need to close and reopen the file (with H5Fopen) to allow creating other files while aFileName is open
  // (On MAC OS X, currently not possible to use H5Fcreate if another file has been created by H5Fcreate and is not closed)
  H5Fclose(vFileId);
  vFileId = H5Fopen(aFileName.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  return vFileId;
}


hid_t bpWriterHDF5::H5FileIdImpl::Get() const
{
  return mId;
}


bpWriterHDF5::H5GroupsManager::H5GroupsManager(bpSharedPtr<H5FileIdImpl> aMainFile)
  : mMainFile(std::move(aMainFile)),
    mIndexR(std::numeric_limits<bpSize>::max()),
    mIndexT(std::numeric_limits<bpSize>::max()),
    mIndexC(std::numeric_limits<bpSize>::max()),
    mDataSetIndex(std::numeric_limits<bpSize>::max())
{
}

bpWriterHDF5::H5GroupsManager::H5GroupsManager(const H5GroupsManager& aOther)
  : mMainFile(aOther.mMainFile),
    mIndexR(std::numeric_limits<bpSize>::max()),
    mIndexT(std::numeric_limits<bpSize>::max()),
    mIndexC(std::numeric_limits<bpSize>::max()),
    mDataSetIndex(std::numeric_limits<bpSize>::max())
{
}


bool bpWriterHDF5::H5GroupsManager::Set(bpSize aDataSetIndex, bpSize aIndexT, bpSize aIndexC, bpSize aIndexR)
{
  bool vHasChanged = aDataSetIndex != mDataSetIndex || aIndexR != mIndexR || aIndexT != mIndexT || aIndexC != mIndexC;
  if (aDataSetIndex != mDataSetIndex) {
    SetDataSet(aDataSetIndex);
    SetResolutionLevel(aIndexR);
    SetTimePointGroup(aIndexT);
    SetChannelGroup(aIndexC);
  }
  if (aIndexR != mIndexR) {
    SetResolutionLevel(aIndexR);
    SetTimePointGroup(aIndexT);
    SetChannelGroup(aIndexC);
  }
  else if (aIndexT != mIndexT) {
    SetTimePointGroup(aIndexT);
    SetChannelGroup(aIndexC);
  }
  else if (aIndexC != mIndexC) {
    SetChannelGroup(aIndexC);
  }
  return vHasChanged;
}


hid_t bpWriterHDF5::H5GroupsManager::GetMainFileId() const
{
  return mMainFile->Get();
}


hid_t bpWriterHDF5::H5GroupsManager::GetChannelGroupId() const
{
  return mChannelGroupId.Get();
}


bpString bpWriterHDF5::H5GroupsManager::GetDataSetGroupName() const
{
  return GetDirectoryName(mDataSetDirectoryName, mDataSetIndex);
}


bpString bpWriterHDF5::H5GroupsManager::GetResolutionLevelGroupPathName() const
{
  return GetDataSetGroupName() + "/" + GetResolutionLevelGroupName();
}


bpString bpWriterHDF5::H5GroupsManager::GetResolutionLevelGroupName() const
{
  return bpString("ResolutionLevel ") + bpImsUtils::bpToString(mIndexR);
}


bpString bpWriterHDF5::H5GroupsManager::GetTimePointGroupPathName() const
{
  return GetResolutionLevelGroupPathName() + "/" + GetTimePointGroupName();
}


bpString bpWriterHDF5::H5GroupsManager::GetTimePointGroupName() const
{
  return bpString("TimePoint ") + bpImsUtils::bpToString(mIndexT);
}


bpString bpWriterHDF5::H5GroupsManager::GetChannelGroupPathName() const
{
  return GetTimePointGroupPathName() + "/" + GetChannelGroupName();
}


bpString bpWriterHDF5::H5GroupsManager::GetChannelGroupName() const
{
  return bpString("Channel ") + bpImsUtils::bpToString(mIndexC);
}


bpString bpWriterHDF5::H5GroupsManager::GetDataSetFileName(bpString aMainFileName, bpSize aDataSetIndex)
{
  return aMainFileName;
}


bpString bpWriterHDF5::H5GroupsManager::GetResolutionLevelFileName(bpString aMainFileName, bpSize aDataSetIndex, bpSize aIndexR)
{
  return GetDataSetFileName(aMainFileName, aDataSetIndex);
}


bpString bpWriterHDF5::H5GroupsManager::GetTimePointFileName(bpString aMainFileName, bpSize aDataSetIndex, bpSize aIndexR, bpSize aIndexT)
{
  return GetResolutionLevelFileName(aMainFileName, aDataSetIndex, aIndexR);
}


bpString bpWriterHDF5::H5GroupsManager::GetChannelFileName(bpString aMainFileName, bpSize aDataSetIndex, bpSize aIndexR, bpSize aIndexT, bpSize aIndexC)
{
  return GetTimePointFileName(aMainFileName, aDataSetIndex, aIndexR, aIndexC);
}


void bpWriterHDF5::H5GroupsManager::SetDataSet(bpSize aDataSetIndex)
{
  mDataSetIndex = aDataSetIndex;
  hid_t vParentGroupId = GetMainFileId();
  mDataSetId = H5GroupId(vParentGroupId, GetDataSetGroupName());
}


void bpWriterHDF5::H5GroupsManager::SetResolutionLevel(bpSize aIndexR)
{
  mIndexR = aIndexR;
  H5GroupId vParentGroupId = mDataSetId;
  mResolutionLevelId = H5GroupId(vParentGroupId, GetResolutionLevelGroupName());
}


void bpWriterHDF5::H5GroupsManager::SetTimePointGroup(bpSize aIndexT)
{
  mIndexT = aIndexT;
  H5GroupId vParentGroupId = mResolutionLevelId;
  mTimePointGroupId = H5GroupId(vParentGroupId, GetTimePointGroupName());
}


void bpWriterHDF5::H5GroupsManager::SetChannelGroup(bpSize aIndexC)
{
  mIndexC = aIndexC;
  H5GroupId vParentGroupId = mTimePointGroupId;
  mChannelGroupId = H5GroupId(vParentGroupId, GetChannelGroupName());
}


void bpWriterHDF5::bpStoreColorInParameters(tParameterMap& aColorParameters, const tColorInfo& aColorInfo) const
{
  if (aColorInfo.mIsBaseColorMode) {
    aColorParameters["ColorMode"] = "BaseColor";
    aColorParameters["Color"] = bpConvColorToString(aColorInfo.mBaseColor);
  }
  else {  // if color mode tableColor
    aColorParameters["ColorMode"] = "TableColor";
    aColorParameters["ColorTable"] = bpConvTableToString(aColorInfo);
    aColorParameters["ColorTableLength"] = bpImsUtils::bpToString(aColorInfo.mColorTable.size());
  }
  aColorParameters["ColorRange"] = bpFloatToString(aColorInfo.mRangeMin) + " " + bpFloatToString(aColorInfo.mRangeMax);
  aColorParameters["ColorOpacity"] = bpFloatToString(aColorInfo.mOpacity);
  aColorParameters["GammaCorrection"] = bpFloatToString(aColorInfo.mGammaCorrection);
}


bpString bpWriterHDF5::bpConvColorToString(const tColor& aColor) const
{
  return bpFloatToString(aColor.mRed) + " " +
    bpFloatToString(aColor.mGreen) + " " +
    bpFloatToString(aColor.mBlue);
}


bpString bpWriterHDF5::bpConvTableToString(const tColorInfo& aColorInfo) const
{
  std::ostringstream caty;
  bpSize vTableSize = aColorInfo.mColorTable.size();
  for (bpSize i = 0; i < vTableSize; i++) {
    caty << bpConvColorToString(aColorInfo.mColorTable[i]) << " ";
  }
  return caty.str();
}
