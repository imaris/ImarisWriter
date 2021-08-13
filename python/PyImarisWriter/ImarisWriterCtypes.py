#/***************************************************************************
# *   Copyright (c) 2020-present Bitplane AG Zuerich                        *
# *                                                                         *
# *   Licensed under the Apache License, Version 2.0 (the "License");       *
# *   you may not use this file except in compliance with the License.      *
# *   You may obtain a copy of the License at                               *
# *                                                                         *
# *       http://www.apache.org/licenses/LICENSE-2.0                        *
# *                                                                         *
# *   Unless required by applicable law or agreed to in writing, software   *
# *   distributed under the License is distributed on an "AS IS" BASIS,     *
# *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imp   *
# *   See the License for the specific language governing permissions and   *
# *   limitations under the License.                                        *
# ***************************************************************************/

"""
Low level wrapper of ImarisWriter C Interface, implemented using ctypes.
"""
 
from ctypes import *


# bpCallbackData

class bpCallbackData(Structure):
    _fields_ = [('mImageIndex', c_uint),
                ('mProgress', c_int)]


bpCallbackDataPtr = POINTER(bpCallbackData)


# bpImageConverterCPtr

class bpImageConverterCPtr(Structure):
    _fields_ = [('raw', c_void_p)]


# bpConverterTypesC_DataType

bpConverterTypesC_DataType = c_int

bpConverterTypesC_UInt8Type = 0
bpConverterTypesC_UInt16Type = (bpConverterTypesC_UInt8Type + 1)
bpConverterTypesC_UInt32Type = (bpConverterTypesC_UInt16Type + 1)
bpConverterTypesC_FloatType = (bpConverterTypesC_UInt32Type + 1)


# bpConverterTypesC_ImageExtent

class bpConverterTypesC_ImageExtent(Structure):
    _fields_ = [('mExtentMinX', c_float),
                ('mExtentMinY', c_float),
                ('mExtentMinZ', c_float),
                ('mExtentMaxX', c_float),
                ('mExtentMaxY', c_float),
                ('mExtentMaxZ', c_float)]


bpConverterTypesC_ImageExtentPtr = POINTER(bpConverterTypesC_ImageExtent)

# bpConverterTypesC_Dimension

bpConverterTypesC_Dimension = c_int

bpConverterTypesC_DimensionX = 0
bpConverterTypesC_DimensionY = (bpConverterTypesC_DimensionX + 1)
bpConverterTypesC_DimensionZ = (bpConverterTypesC_DimensionY + 1)
bpConverterTypesC_DimensionC = (bpConverterTypesC_DimensionZ + 1)
bpConverterTypesC_DimensionT = (bpConverterTypesC_DimensionC + 1)


# bpConverterTypesC_DimensionSequence5D

class bpConverterTypesC_DimensionSequence5D(Structure):
    _fields_ = [('mDimension0', bpConverterTypesC_Dimension),
                ('mDimension1', bpConverterTypesC_Dimension),
                ('mDimension2', bpConverterTypesC_Dimension),
                ('mDimension3', bpConverterTypesC_Dimension),
                ('mDimension4', bpConverterTypesC_Dimension)]


bpConverterTypesC_DimensionSequence5DPtr = POINTER(bpConverterTypesC_DimensionSequence5D)


# bpConverterTypesC_Size5D

class bpConverterTypesC_Size5D(Structure):
    _fields_ = [('mValueX', c_uint),
                ('mValueY', c_uint),
                ('mValueZ', c_uint),
                ('mValueC', c_uint),
                ('mValueT', c_uint)]


bpConverterTypesC_Size5DPtr = POINTER(bpConverterTypesC_Size5D)


# bpConverterTypesC_Index5D

class bpConverterTypesC_Index5D(Structure):
    _fields_ = [('mValueX', c_uint),
                ('mValueY', c_uint),
                ('mValueZ', c_uint),
                ('mValueC', c_uint),
                ('mValueT', c_uint)]


bpConverterTypesC_Index5DPtr = POINTER(bpConverterTypesC_Index5D)

# tCompressionAlgorithmType

tCompressionAlgorithmType = c_int

eCompressionAlgorithmNone = 0
eCompressionAlgorithmGzipLevel1 = 1
eCompressionAlgorithmGzipLevel2 = 2
eCompressionAlgorithmGzipLevel3 = 3
eCompressionAlgorithmGzipLevel4 = 4
eCompressionAlgorithmGzipLevel5 = 5
eCompressionAlgorithmGzipLevel6 = 6
eCompressionAlgorithmGzipLevel7 = 7
eCompressionAlgorithmGzipLevel8 = 8
eCompressionAlgorithmGzipLevel9 = 9
eCompressionAlgorithmShuffleGzipLevel1 = 11
eCompressionAlgorithmShuffleGzipLevel2 = 12
eCompressionAlgorithmShuffleGzipLevel3 = 13
eCompressionAlgorithmShuffleGzipLevel4 = 14
eCompressionAlgorithmShuffleGzipLevel5 = 15
eCompressionAlgorithmShuffleGzipLevel6 = 16
eCompressionAlgorithmShuffleGzipLevel7 = 17
eCompressionAlgorithmShuffleGzipLevel8 = 18
eCompressionAlgorithmShuffleGzipLevel9 = 19
eCompressionAlgorithmLZ4 = 21
eCompressionAlgorithmShuffleLZ4 = 31


# bpConverterTypesC_Options

class bpConverterTypesC_Options(Structure):
    _fields_ = [('mThumbnailSizeXY', c_uint),
                ('mFlipDimensionX', c_bool),
                ('mFlipDimensionY', c_bool),
                ('mFlipDimensionZ', c_bool),
                ('mForceFileBlockSizeZ1', c_bool),
                ('mEnableLogProgress', c_bool),
                ('mNumberOfThreads', c_uint),
                ('mCompressionAlgorithmType', tCompressionAlgorithmType)]


bpConverterTypesC_OptionsPtr = POINTER(bpConverterTypesC_Options)

bpConverterTypesC_String = c_char_p


# bpConverterTypesC_Parameters

class bpConverterTypesC_Parameter(Structure):
    _fields_ = [('mName', bpConverterTypesC_String),
                ('mValue', bpConverterTypesC_String)]


bpConverterTypesC_ParameterPtr = POINTER(bpConverterTypesC_Parameter)


class bpConverterTypesC_ParameterSection(Structure):
    _fields_ = [('mName', bpConverterTypesC_String),
                ('mValues', bpConverterTypesC_ParameterPtr),
                ('mValuesCount', c_uint)]


bpConverterTypesC_ParameterSectionPtr = POINTER(bpConverterTypesC_ParameterSection)


class bpConverterTypesC_Parameters(Structure):
    _fields_ = [('mValues', bpConverterTypesC_ParameterSectionPtr),
                ('mValuesCount', c_uint)]


bpConverterTypesC_ParametersPtr = POINTER(bpConverterTypesC_Parameters)


# bpConverterTypesC_TimeInfos

class bpConverterTypesC_TimeInfo(Structure):
    _fields_ = [('mJulianDay', c_uint),
                ('mNanosecondsOfDay', c_ulonglong)]


bpConverterTypesC_TimeInfoPtr = POINTER(bpConverterTypesC_TimeInfo)


class bpConverterTypesC_TimeInfos(Structure):
    _fields_ = [('mValues', bpConverterTypesC_TimeInfoPtr),
                ('mValuesCount', c_uint)]


bpConverterTypesC_TimeInfosPtr = POINTER(bpConverterTypesC_TimeInfos)


# bpConverterTypesC_ColorInfos

class bpConverterTypesC_Color(Structure):
    _fields_ = [('mRed', c_float),
                ('mGreen', c_float),
                ('mBlue', c_float),
                ('mAlpha', c_float)]


bpConverterTypesC_ColorPtr = POINTER(bpConverterTypesC_Color)


class bpConverterTypesC_ColorInfo(Structure):
    _fields_ = [('mIsBaseColorMode', c_bool),
                ('mBaseColor', bpConverterTypesC_Color),
                ('mColorTable', bpConverterTypesC_ColorPtr),
                ('mColorTableSize', c_uint),
                ('mOpacity', c_float),
                ('mRangeMin', c_float),
                ('mRangeMax', c_float),
                ('mGammaCorrection', c_float)]


bpConverterTypesC_ColorInfoPtr = POINTER(bpConverterTypesC_ColorInfo)


class bpConverterTypesC_ColorInfos(Structure):
    _fields_ = [('mValues', bpConverterTypesC_ColorInfoPtr),
                ('mValuesCount', c_uint)]


bpConverterTypesC_ColorInfosPtr = POINTER(bpConverterTypesC_ColorInfos)

bpConverterTypesC_ProgressCallback = CFUNCTYPE(c_void_p, c_float, c_ulonglong, c_void_p)
