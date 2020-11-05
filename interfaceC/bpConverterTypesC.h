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
#ifndef __BP_CONVERTER_TYPES_C__
#define __BP_CONVERTER_TYPES_C__


typedef unsigned char bpConverterTypesC_UInt8;
typedef unsigned short int bpConverterTypesC_UInt16;
typedef unsigned int bpConverterTypesC_UInt32;
typedef unsigned long long bpConverterTypesC_UInt64;
typedef float bpConverterTypesC_Float;


typedef enum
{
  bpConverterTypesC_UInt8Type,
  bpConverterTypesC_UInt16Type,
  bpConverterTypesC_UInt32Type,
  bpConverterTypesC_FloatType
} bpConverterTypesC_DataType;


typedef struct
{
  float mExtentMinX;
  float mExtentMinY;
  float mExtentMinZ;
  float mExtentMaxX;
  float mExtentMaxY;
  float mExtentMaxZ;
} bpConverterTypesC_ImageExtent;

typedef const bpConverterTypesC_ImageExtent* bpConverterTypesC_ImageExtentPtr;


typedef enum
{
  bpConverterTypesC_DimensionX,
  bpConverterTypesC_DimensionY,
  bpConverterTypesC_DimensionZ,
  bpConverterTypesC_DimensionC,
  bpConverterTypesC_DimensionT
} bpConverterTypesC_Dimension;


// should not contain duplicates
typedef struct
{
  bpConverterTypesC_Dimension mDimension0;
  bpConverterTypesC_Dimension mDimension1;
  bpConverterTypesC_Dimension mDimension2;
  bpConverterTypesC_Dimension mDimension3;
  bpConverterTypesC_Dimension mDimension4;
} bpConverterTypesC_DimensionSequence5D;

typedef const bpConverterTypesC_DimensionSequence5D* bpConverterTypesC_DimensionSequence5DPtr;


typedef struct
{
  unsigned int mValueX;
  unsigned int mValueY;
  unsigned int mValueZ;
  unsigned int mValueC;
  unsigned int mValueT;
} bpConverterTypesC_Size5D;

typedef const bpConverterTypesC_Size5D* bpConverterTypesC_Size5DPtr;


typedef struct
{
  unsigned int mValueX;
  unsigned int mValueY;
  unsigned int mValueZ;
  unsigned int mValueC;
  unsigned int mValueT;
} bpConverterTypesC_Index5D;

typedef const bpConverterTypesC_Index5D* bpConverterTypesC_Index5DPtr;


typedef const char* bpConverterTypesC_String;


typedef enum {
  eCompressionAlgorithmNone = 0,
  eCompressionAlgorithmGzipLevel1 = 1,
  eCompressionAlgorithmGzipLevel2 = 2,
  eCompressionAlgorithmGzipLevel3 = 3,
  eCompressionAlgorithmGzipLevel4 = 4,
  eCompressionAlgorithmGzipLevel5 = 5,
  eCompressionAlgorithmGzipLevel6 = 6,
  eCompressionAlgorithmGzipLevel7 = 7,
  eCompressionAlgorithmGzipLevel8 = 8,
  eCompressionAlgorithmGzipLevel9 = 9,
  eCompressionAlgorithmShuffleGzipLevel1 = 11,
  eCompressionAlgorithmShuffleGzipLevel2 = 12,
  eCompressionAlgorithmShuffleGzipLevel3 = 13,
  eCompressionAlgorithmShuffleGzipLevel4 = 14,
  eCompressionAlgorithmShuffleGzipLevel5 = 15,
  eCompressionAlgorithmShuffleGzipLevel6 = 16,
  eCompressionAlgorithmShuffleGzipLevel7 = 17,
  eCompressionAlgorithmShuffleGzipLevel8 = 18,
  eCompressionAlgorithmShuffleGzipLevel9 = 19,
  eCompressionAlgorithmLZ4 = 21,
  eCompressionAlgorithmLShuffleLZ4 = 31
} tCompressionAlgorithmType;


typedef struct
{
  unsigned int mThumbnailSizeXY; // 256
  bool mFlipDimensionX; // false
  bool mFlipDimensionY; // false
  bool mFlipDimensionZ; // false
  bool mForceFileBlockSizeZ1; // false
  bool mEnableLogProgress; // false
  unsigned int mNumberOfThreads; // 8
  tCompressionAlgorithmType mCompressionAlgorithmType; // eCompressionAlgorithmGzipLevel2
  bool mDisablePyramid; // false
} bpConverterTypesC_Options;

typedef const bpConverterTypesC_Options* bpConverterTypesC_OptionsPtr;

typedef struct {
  bpConverterTypesC_String mName;
  bpConverterTypesC_String mValue;
} bpConverterTypesC_Parameter;


typedef struct {
  bpConverterTypesC_String mName;
  const bpConverterTypesC_Parameter* mValues;
  unsigned int mValuesCount;
} bpConverterTypesC_ParameterSection;


typedef struct {
  const bpConverterTypesC_ParameterSection* mValues;
  unsigned int mValuesCount;
} bpConverterTypesC_Parameters;

typedef const bpConverterTypesC_Parameters* bpConverterTypesC_ParametersPtr;


typedef struct {
  unsigned int mJulianDay;
  unsigned long long mNanosecondsOfDay;
} bpConverterTypesC_TimeInfo;

typedef struct {
  const bpConverterTypesC_TimeInfo* mValues;
  unsigned int mValuesCount;
} bpConverterTypesC_TimeInfos;

typedef const bpConverterTypesC_TimeInfos* bpConverterTypesC_TimeInfoVector;


typedef struct {
  float mRed;
  float mGreen;
  float mBlue;
  float mAlpha;
} bpConverterTypesC_Color;

typedef struct {
  bool mIsBaseColorMode;
  bpConverterTypesC_Color mBaseColor;
  const bpConverterTypesC_Color* mColorTable;
  unsigned int mColorTableSize;
  float mOpacity;
  float mRangeMin;
  float mRangeMax;
  float mGammaCorrection;
} bpConverterTypesC_ColorInfo;

typedef struct {
  const bpConverterTypesC_ColorInfo* mValues;
  unsigned int mValuesCount;
} bpConverterTypesC_ColorInfos;

typedef const bpConverterTypesC_ColorInfos* bpConverterTypesC_ColorInfoVector;


#endif // __BP_CONVERTER_TYPES__
