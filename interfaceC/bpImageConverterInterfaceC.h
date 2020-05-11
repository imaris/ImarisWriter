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
#ifndef __BP_IMAGE_CONVERTER_INTERFACE_C__
#define __BP_IMAGE_CONVERTER_INTERFACE_C__


#ifdef _WIN32
#define BP_IMARISWRITER_DLL_EXPORT __declspec( dllexport )
#define BP_IMARISWRITER_DLL_IMPORT __declspec( dllimport )
#else //_WIN32
#define BP_IMARISWRITER_DLL_EXPORT __attribute__((visibility("default")))
#define BP_IMARISWRITER_DLL_IMPORT __attribute__((visibility("default")))
#endif

#ifdef EXPORT_IMARISWRITER_DLL
#define BP_IMARISWRITER_DLL_API BP_IMARISWRITER_DLL_EXPORT
#else
#define BP_IMARISWRITER_DLL_API BP_IMARISWRITER_DLL_IMPORT
#endif


#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "bpConverterTypesC.h"

struct bpImageConverterC;
typedef struct bpImageConverterC* bpImageConverterCPtr;

typedef void(*bpConverterTypesC_ProgressCallback)(bpConverterTypesC_Float aProgress, bpConverterTypesC_UInt64 aTotalBytesWritten, void* aUserData);

BP_IMARISWRITER_DLL_API bpImageConverterCPtr bpImageConverterC_Create(
  bpConverterTypesC_DataType aDataType, bpConverterTypesC_Size5DPtr aImageSize, bpConverterTypesC_Size5DPtr aSample,
  bpConverterTypesC_DimensionSequence5DPtr aDimensionSequence, bpConverterTypesC_Size5DPtr aFileBlockSize,
  bpConverterTypesC_String aOutputFile, bpConverterTypesC_OptionsPtr aOptions,
  bpConverterTypesC_String aApplicationName, bpConverterTypesC_String aApplicationVersion,
  bpConverterTypesC_ProgressCallback aProgressCallback, void* aCallbackUserData);

BP_IMARISWRITER_DLL_API void bpImageConverterC_Destroy(bpImageConverterCPtr aImageConverterC);

BP_IMARISWRITER_DLL_API const char* bpImageConverterC_GetLastException(bpImageConverterCPtr aImageConverterC);

BP_IMARISWRITER_DLL_API bool bpImageConverterC_NeedCopyBlock(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_Index5DPtr aBlockIndex);

BP_IMARISWRITER_DLL_API void bpImageConverterC_CopyBlockUInt8(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_UInt8* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex);
BP_IMARISWRITER_DLL_API void bpImageConverterC_CopyBlockUInt16(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_UInt16* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex);
BP_IMARISWRITER_DLL_API void bpImageConverterC_CopyBlockUInt32(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_UInt32* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex);
BP_IMARISWRITER_DLL_API void bpImageConverterC_CopyBlockFloat(bpImageConverterCPtr aImageConverterC, bpConverterTypesC_Float* aFileDataBlock, bpConverterTypesC_Index5DPtr aBlockIndex);

BP_IMARISWRITER_DLL_API void bpImageConverterC_Finish(
  bpImageConverterCPtr aImageConverterC,
  bpConverterTypesC_ImageExtentPtr aImageExtent,
  bpConverterTypesC_ParametersPtr aParameters,
  bpConverterTypesC_TimeInfoVector aTimeInfoPerTimePoint,
  bpConverterTypesC_ColorInfoVector aColorInfoPerChannel,
  bool aAutoAdjustColorRange);

#ifdef __cplusplus
}
#endif

#endif // __BP_IMAGE_CONVERTER_INTERFACE__
