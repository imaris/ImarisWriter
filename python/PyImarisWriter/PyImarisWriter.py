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
Python wrapper of ImarisWriter Library.
"""
 
from collections import defaultdict
import os
import platform
from datetime import date, datetime
import time
from typing import Dict, List

try:
    import numpy as np
    HAS_NUMPY = True
except:
    HAS_NUMPY = False

from .ImarisWriterCtypes import *

def adjust_dll_loading_configuration():
    """Adds folder of this module to the dll directories so that the dependent dlls can be loaded"""
    dll_directory = os.path.dirname(__file__)
    if hasattr(os, 'add_dll_directory'):
        #print('Adding with add_dll_directory(): {}'.format(dll_directory))
        os.add_dll_directory(dll_directory) # type: ignore[attr-defined]
    else:
        #print('Adding to PATH variable: {}'.format(dll_directory))
        os.environ['PATH'] = dll_directory + os.pathsep + os.environ['PATH']

adjust_dll_loading_configuration()

# variant of Progress Callback that does not have a user data pointer
bpConverterTypesC_ProgressCallback_NoUserdata = CFUNCTYPE(c_void_p, c_float, c_ulonglong)

def to_julian_day(time_info : datetime):
    """"1970-01-01 is julian day 2440588"""
    datetime70 = date(1970,1,1)
    difference = (time_info.date() - datetime70).days
    return 2440588 + difference

def get_c_time_info(time_info : datetime):
    julian_day = to_julian_day(time_info)
    t = time_info.time()
    nanoseconds = ((t.hour * 3600 + t.minute * 60 + t.second) * 1000000 + t.microsecond) * 1000
    c_time_info = bpConverterTypesC_TimeInfo(julian_day, nanoseconds)
    return c_time_info

def get_valid_dimension_keys():
    return ['x', 'y', 'z', 'c', 't']
    
def get_required_dimension_keys():
    return set(get_valid_dimension_keys())

def get_required_color_keys_ordered_list():
    return ['r', 'g', 'b', 'a']

def get_required_color_keys():
    return set(get_required_color_keys_ordered_list())
    
def get_missing_keys(keys, required_keys):
    return required_keys.difference(keys)


class PyImarisWriterException(Exception):
    def __init__(self, text):
        #print(text)
        pass

class PyClassnameExceptionRaiser:
    """Class that has a method to raise PyImarisWriterException exception while class name is included in the
    exception message.
    """
    def raise_creating_clex(self, message):
        raise PyImarisWriterException('Error creating {}: {}'.format(self.__class__.__name__, message))

class ImageSize(PyClassnameExceptionRaiser):
    def __init__(self, *args, **kwargs):
        if len(args) == 0 and len(kwargs) == 0:
            self.x = 0
            self.y = 0
            self.z = 0
            self.c = 0
            self.t = 0
            return
        missing_keys = get_missing_keys(kwargs.keys(), get_required_dimension_keys())
        if missing_keys:
            self.raise_creating_clex('Missing arguments: {}'.format(','.join(missing_keys)))
        self.x = self._parse_int(kwargs, 'x')
        self.y = self._parse_int(kwargs, 'y')
        self.z = self._parse_int(kwargs, 'z')
        self.c = self._parse_int(kwargs, 'c')
        self.t = self._parse_int(kwargs, 't')
        
    def _parse_int(self, dict, key):
        try:
            return int(dict[key])
        except ValueError:
            raise PyImarisWriterException('Error parsing int from parameter {} : "{}"'.format(key, dict[key]))

    def __str__(self):
        return '{} (x: {} y: {} z: {} c: {} t: {})'.format(self.__class__.__name__, self.x, self.y, self.z, self.c, self.t)
        
    def __truediv__(self, other):
        return ImageSize(x=self.x // other.x, y=self.y // other.y, z=self.z // other.z, c=self.c // other.c, t=self.t // other.t)
 
class DimensionSequence(PyClassnameExceptionRaiser):
    def __init__(self, *args):
        if len(args) != 5:
            self.raise_creating_clex('5 arguments must be provided. You provided {}'.format(len(args)))
        arg_lower = [i.lower() for i in args]
        arg_set = set(arg_lower)
        if len(arg_set) != 5:
            self.raise_creating_clex('Duplicates detected. You provided {}'.format(','.join(args)))
        
        missing_keys = get_missing_keys(arg_set, get_required_dimension_keys())
        if missing_keys:
            self.raise_creating_clex('Missing arguments: {}. You provided {}'.format(','.join(missing_keys), ','.join(args)))
        self.mSequence : List[str] = arg_lower
        
    def get_sequence(self):
        return self.mSequence

class ImageExtents:
    def __init__(self, minX, minY, minZ, maxX, maxY, maxZ):
        self.minX : float = minX
        self.minY : float = minY
        self.minZ : float = minZ
        self.maxX : float = maxX
        self.maxY : float = maxY
        self.maxZ : float = maxZ

    def get_c_image_extents(self):
        c_image_extents = bpConverterTypesC_ImageExtent(self.minX,
                                                        self.minY,
                                                        self.minZ,
                                                        self.maxX,
                                                        self.maxY,
                                                        self.maxZ)
        return bpConverterTypesC_ImageExtentPtr(c_image_extents)


class Color(PyClassnameExceptionRaiser):
        
    def __init__(self, *args, **kwargs):
        if len(args) == 0 and len(kwargs) == 0:
            self.init_with_values_from_list([1, 0, 0, 1])
            return
            
        if len(args) == 4:
            self.init_with_values_from_list(args)
            return
            
        if len(kwargs) != 4:
            self.raise_creating_clex('4 arguments must be provided. You provided {}'.format(len(kwargs)))
        missing_keys = get_missing_keys(set(kwargs.keys()), get_required_color_keys())
        if missing_keys:
            self.raise_creating_clex('Missing arguments: {}. You provided {}'.format(','.join(missing_keys), ','.join(kwargs)))

        list = [self._get_dict_value(kwargs, x) for x in get_required_color_keys_ordered_list()]
        self.init_with_values_from_list(list)

    
    def _to_float(self, value):
        try:
            return float(value)
        except ValueError:
            raise PyImarisWriterException('Error converting {} to float'.format(value))

    def _get_dict_value(self, dict, key):
        try:
            return dict[key]
        except ValueError:
            raise PyImarisWriterException('Error parsing float from parameter {}'.format(key))

    def init_with_values_from_list(self, list):
        if len(list) != 4:
            self.raise_creating_clex('4 arguments must be provided. You provided {}'.format(len(list)))
        self.mRed = self._to_float(list[0])
        self.mGreen = self._to_float(list[1])
        self.mBlue = self._to_float(list[2])
        self.mAlpha = self._to_float(list[3])

    def get_c_color(self):
        c_color = bpConverterTypesC_Color()
        c_color.mRed = self.mRed
        c_color.mGreen = self.mGreen
        c_color.mBlue  = self.mBlue
        c_color.mAlpha = self.mAlpha
        return c_color

    def __eq__(self, other):
        if isinstance(other, Color):
            return self.mRed == other.mRed and self.mGreen == other.mGreen and self.mBlue == other.mBlue and self.mAlpha == other.mAlpha
        return False

    def __str__(self):
        return '{} (red: {} green: {} blue: {} alpha: {})'.format(self.__class__.__name__, self.mRed, self.mGreen, self.mBlue, self.mAlpha)
    
    
class ColorInfo:
    def __init__(self):
        self.mBaseColor = Color()
        self.mIsBaseColorMode = True
        self.mColorTableSize = 0
        self.mOpacity = 0
        self.mRangeMin = 0
        self.mRangeMax = 255
        self.mGammaCorrection = 1
        self.mColorTableList : List[Color] = []

    def set_base_color(self, base_color : Color):
        self.mBaseColor = base_color
        self.mIsBaseColorMode = True
        
    def set_color_table(self, color_list : List[Color]):
        self.mIsBaseColorMode = False
        self.mColorTableSize = len(color_list)
        self.mColorTableList = color_list

    def get_c_color_info(self):
        c_color_info = bpConverterTypesC_ColorInfo()
        c_color_info.mIsBaseColorMode = self.mIsBaseColorMode
        c_color_info.mBaseColor = self.mBaseColor.get_c_color()
        self._create_color_table(c_color_info)
        c_color_info.mOpacity = self.mOpacity
        c_color_info.mRangeMin = self.mRangeMin
        c_color_info.mRangeMax = self.mRangeMax
        c_color_info.mGammaCorrection = self.mGammaCorrection
        return c_color_info

    def _create_color_table(self, c_color_info):
        num_colors = len(self.mColorTableList)
        c_color_info.mColorTableSize = num_colors
        c_color_info.mColorTable = (bpConverterTypesC_Color * num_colors)()
        for i, color in enumerate(self.mColorTableList):
            c_color_info.mColorTable[i] = color.get_c_color()


    def __str__(self):
        return '{} (IsBase: {} BaseColor: {} ColorTableSize: {} Opacity: {} RangeMin: {} RangeMax: {} GammaCorrection: {})' \
            .format(self.__class__.__name__, self.mIsBaseColorMode, self.mBaseColor, self.mColorTableSize, \
                    self.mOpacity, self.mRangeMin, self.mRangeMax, self.mGammaCorrection)

class Parameters:
    def __init__(self):
        self.mSections : Dict[str, Dict[str, str]] = defaultdict(dict)
        
    def set_value(self, section, parameter_name, value):
        value_str = str(value)
        self.mSections[section][parameter_name] = value_str

    def create_c_parameter(self, name, value):
        return bpConverterTypesC_Parameter(name.encode(), value.encode())

    def create_parameter_section(self, section_name, items):
        num_parameters = len(items)
        parameter_section = bpConverterTypesC_ParameterSection()
        parameter_section.mName = section_name.encode()
        parameter_data = (bpConverterTypesC_Parameter * num_parameters)()
        for i, param_name in enumerate(items):
            param_value = items[param_name]
            parameter = self.create_c_parameter(param_name, param_value)
            parameter_data[i] = parameter
        parameter_section.mValues = parameter_data
        parameter_section.mValuesCount = num_parameters
        return parameter_section

    def set_channel_name(self, channel_index : int, name : str):
        self.set_value('Channel {}'.format(channel_index), 'Name', name)

    def _get_c_parameters(self):
        number_of_sections = len(self.mSections)
        parameter_section_data = (bpConverterTypesC_ParameterSection * number_of_sections)()
        for section_index, key in enumerate(self.mSections):
            items = self.mSections[key]
            parameter_section_data[section_index] = self.create_parameter_section(key, items)
        c_parameters = bpConverterTypesC_Parameters(parameter_section_data, number_of_sections)
        return c_parameters
    

class Options:
    def __init__(self):
        self.mThumbnailSizeXY = 256
        self.mForceFileBlockSizeZ = False
        self.mFlipDimensionX = False
        self.mFlipDimensionY = False
        self.mFlipDimensionZ = False
        self.mEnableLogProgress = False
        self.mNumberOfThreads = 8
        self.mCompressionAlgorithmType = eCompressionAlgorithmGzipLevel2


class CallbackClass:
    def RecordProgress(self, progress : float, total_bytes_written : int):
        print('Client Progress: {:.2%} bytes {}'.format(progress, total_bytes_written))


class ImageConverter(PyClassnameExceptionRaiser):
    def __init__(self,
                 datatype : str,
                 image_size : ImageSize,
                 sample_size : ImageSize,
                 dimension_sequence : DimensionSequence,
                 block_size: ImageSize,
                 output_filename : str,
                 options : Options,
                 application_name : str,
                 application_version : str,
                 progress_callback_class: CallbackClass):
        self._load_dll()
        self._store_datatype(datatype)
        self._store_copyblock_method(datatype)
        self._store_image_size(image_size)
        self._store_sample_size(sample_size)
        self._store_dimension_sequence(dimension_sequence)
        self._store_block_size(block_size)
        self._store_options(options)
        self._store_strings(output_filename, application_name, application_version)
        self._store_progress_callback(progress_callback_class)

        self._create()
        
    def _get_dll_filename(self):
        if platform.system() == 'Windows':
            return 'bpImarisWriter96.dll'
        elif platform.system() == 'Darwin':
            return 'libbpImarisWriter96.dylib'
        elif platform.system() == 'Linux':
            return 'libbpImarisWriter96.so'
        else:
            print('Platform not supported: "{}"'.format(platform.system()))
            return None

    def _load_dll(self):
        dll_filename = self._get_dll_filename()
        try:
            self.mcdll = CDLL(dll_filename)
        except OSError as error:
            text = 'Could not load library "{}" (with dependencies hdf5 and zlib)\n'.format(dll_filename)
            text += 'Please make sure the library is available and in the PATH environment variable\n{}'.format(error)
            self.raise_creating_clex(text)

    def _check_errors(self, title):
            self.mcdll.bpImageConverterC_GetLastException.restype = c_char_p
            last_exception = self.mcdll.bpImageConverterC_GetLastException(self.mImageConverterPtr)
            if last_exception:
                message = '{}: An Error occured.\nError: "{}"'.format(title, last_exception.decode())
                self.raise_creating_clex(message)

    def _progress_callback(self, progress : float, total_bytes_written : int):
        self.mProgressCallbackClass.RecordProgress(progress, total_bytes_written)

    def _store_datatype(self, datatype):
        valid_types = {'uint8'  : bpConverterTypesC_UInt8Type,
                       'uint16' : bpConverterTypesC_UInt16Type,
                       'uint32' : bpConverterTypesC_UInt32Type,
                       'float32' :bpConverterTypesC_FloatType
                       }
                       
        if datatype not in valid_types.keys():
            valid_types_string = ', '.join(['"{}"'.format(x) for x in valid_types.keys()])
            self.raise_creating_clex('Datatype "{}" is invalid, must be one of {}'.format(datatype, valid_types_string))
        self.mDataType = valid_types[datatype]
        
    def _store_copyblock_method(self, datatype):
        copy_methods = {'uint8': self.mcdll.bpImageConverterC_CopyBlockUInt8,
                        'uint16': self.mcdll.bpImageConverterC_CopyBlockUInt16,
                        'uint32': self.mcdll.bpImageConverterC_CopyBlockUInt32,
                        'float32': self.mcdll.bpImageConverterC_CopyBlockFloat
                       }
        copy_method = copy_methods[datatype]
        if not copy_method:
            self.raise_creating_clex('No valid copy method for datatype "{}"'.format(self.mDataType))
        self.mCopyBlockMethod = copy_method
        
    def _store_image_size(self, image_size):
        if not isinstance(image_size, ImageSize):
            self.raise_creating_clex('Invalid image_size: {}'.format(image_size))
        self.mImageSize = bpConverterTypesC_Size5DPtr(bpConverterTypesC_Size5D(image_size.x,
                                                                               image_size.y,
                                                                               image_size.z,
                                                                               image_size.c,
                                                                               image_size.t))
    
    def _store_sample_size(self, sample_size):
        if not isinstance(sample_size, ImageSize):
            self.raise_creating_clex('Invalid sample_size: {}'.format(sample_size))
        self.mSampleSize = bpConverterTypesC_Size5DPtr(bpConverterTypesC_Size5D(sample_size.x,
                                                                                sample_size.y,
                                                                                sample_size.z,
                                                                                sample_size.c,
                                                                                sample_size.t))
    
    def _store_dimension_sequence(self, dimension_sequence):
        dimension_dict = { 'x' : bpConverterTypesC_DimensionX,
                           'y' : bpConverterTypesC_DimensionY,
                           'z' : bpConverterTypesC_DimensionZ,
                           'c' : bpConverterTypesC_DimensionC,
                           't' : bpConverterTypesC_DimensionT
                         }
        if not isinstance(dimension_sequence, DimensionSequence):
            self.raise_creating_clex('Invalid dimension_sequence: {}'.format(dimension_sequence))
        sequence = [dimension_dict[i] for i in dimension_sequence.get_sequence()]
        self.mDimensionSequence = bpConverterTypesC_DimensionSequence5D(*sequence)
        
    def _store_block_size(self, block_size):
        if not isinstance(block_size, ImageSize):
            self.raise_creating_clex('Invalid block_size: {}'.format(block_size))
        self.mBlockSize = bpConverterTypesC_Size5DPtr(bpConverterTypesC_Size5D(block_size.x,
                                                                               block_size.y,
                                                                               block_size.z,
                                                                               block_size.c,
                                                                               block_size.t))
    
    def _store_options(self, options):
        if not options:
            self.raise_creating_clex('Invalid options: {}'.format(options))
        try:
            self.mOptions = bpConverterTypesC_OptionsPtr(bpConverterTypesC_Options(options.mThumbnailSizeXY,
                                                                                   options.mForceFileBlockSizeZ,
                                                                                   options.mFlipDimensionX,
                                                                                   options.mFlipDimensionY,
                                                                                   options.mFlipDimensionZ,
                                                                                   options.mEnableLogProgress,
                                                                                   options.mNumberOfThreads,
                                                                                   options.mCompressionAlgorithmType))
        except AttributeError as error:
             self.raise_creating_clex('Invalid options: {}'.format(error))


    def _get_c_char(self, string):
        encoded = c_char_p(str(string).encode())
        return encoded

    def _store_strings(self, output_filename, application_name, application_version):
        if not output_filename:
            self.raise_creating_clex('Invalid output filename: {}'.format(output_filename))
        self.mOutputFilename = self._get_c_char(output_filename)
        self.mApplicationName = self._get_c_char(application_name)
        self.mApplicationVersion = self._get_c_char(application_version)

    def _store_progress_callback(self, progress_callback_class):
        if not hasattr(progress_callback_class, 'RecordProgress'):
            text = 'The provided progress callback class "{}" does not have the required method RecordProgress(float, int)'.format(progress_callback_class)
            raise PyImarisWriterException(text)
        self.mProgressCallbackClass = progress_callback_class

    def _get_c_time_infos(self, time_infos):
        num_time_infos = len(time_infos)
        time_info_data = (bpConverterTypesC_TimeInfo * num_time_infos)()

        for i, tinfo in enumerate(time_infos):
            time_info = get_c_time_info(tinfo)
            time_info_data[i] = time_info
        return bpConverterTypesC_TimeInfosPtr(bpConverterTypesC_TimeInfos(time_info_data, num_time_infos))

    def _get_c_color_infos(self, color_infos):
        num_color_infos = len(color_infos)
        color_info_data = (bpConverterTypesC_ColorInfo * num_color_infos)()
        for i, ci in enumerate(color_infos):
            c_color_info = ci.get_c_color_info()
            color_info_data[i] = c_color_info
        return bpConverterTypesC_ColorInfosPtr(bpConverterTypesC_ColorInfos(color_info_data, num_color_infos))

    def _create(self):

        self.mProgressCallback = bpConverterTypesC_ProgressCallback_NoUserdata(self._progress_callback)

        self.mcdll.bpImageConverterC_Create.restype = bpImageConverterCPtr
        self.mImageConverterPtr = self.mcdll.bpImageConverterC_Create(self.mDataType,
                                                                      self.mImageSize,
                                                                      self.mSampleSize,
                                                                      self.mDimensionSequence,
                                                                      self.mBlockSize,
                                                                      self.mOutputFilename,
                                                                      self.mOptions,
                                                                      self.mApplicationName,
                                                                      self.mApplicationVersion,
                                                                      self.mProgressCallback,
                                                                      None)

    def _get_np_type(self):
        if not HAS_NUMPY:
            raise PyImarisWriterException('Cannot get numpy type, numpy module not found')

        if self.mDataType == bpConverterTypesC_UInt8Type:
            return np.uint8
        elif self.mDataType == bpConverterTypesC_UInt16Type:
            return np.uint16
        elif self.mDataType == bpConverterTypesC_UInt32Type:
            return np.uint32
        elif self.mDataType == bpConverterTypesC_FloatType:
            return np.float32
        else:
            raise PyImarisWriterException('Could not create numpy type for PyImarisWriter Type "{}"'.format(self.mDataType))

    def _get_np_as_data_type(self):
        if not HAS_NUMPY:
            raise PyImarisWriterException('Cannot get numpy/ctypes type, numpy module not found')

        if self.mDataType == bpConverterTypesC_UInt8Type:
            return c_char_p
        elif self.mDataType == bpConverterTypesC_UInt16Type:
            return POINTER(c_uint16)
        elif self.mDataType == bpConverterTypesC_UInt32Type:
            return POINTER(c_uint32)
        elif self.mDataType == bpConverterTypesC_FloatType:
            return POINTER(c_float)
        else:
            raise PyImarisWriterException(
                'Could not create numpy/ctypes type for PyImarisWriter Type "{}"'.format(self.mDataType))


    def _get_converted_voxel_data(self, voxel_data):
        """Checks if a numpy array was passed and converts the array if necessary"""
        if HAS_NUMPY and type(voxel_data) is np.ndarray:
            np_type = self._get_np_type()
            time1 = datetime.now()
            np_data = voxel_data.astype(np_type, copy=False)
            time2 = datetime.now()
            diff_seconds = (time2 - time1).total_seconds()
            #print('astype took {} seconds'.format(diff_seconds))
            as_data_type = self._get_np_as_data_type()
            voxel_data_conv = np_data.ctypes.data_as(as_data_type)
            return voxel_data_conv

        return voxel_data


    def NeedCopyBlock(self, block_index : ImageSize) -> bool:
        c_block_index = bpConverterTypesC_Index5D(block_index.x, block_index.y, block_index.z, block_index.c, block_index.t, )
        return_value = self.mcdll.bpImageConverterC_NeedCopyBlock(self.mImageConverterPtr, c_block_index)
        need_copy = return_value == 1
        #print('NeedCopyBlock {} ? {}'.format(block_index, "yes" if return_value else "no"))
        return need_copy


    def CopyBlock(self, voxel_data, block_index : ImageSize) -> None:
        c_block_index = bpConverterTypesC_Index5D(block_index.x, block_index.y, block_index.z, block_index.c, block_index.t, )
        block_index_ptr = bpConverterTypesC_Index5DPtr(c_block_index)
        voxel_data_conv = self._get_converted_voxel_data(voxel_data)
        self.mCopyBlockMethod(self.mImageConverterPtr, voxel_data_conv, block_index_ptr)
        title = 'CopyBlock {}'.format(block_index)
        self._check_errors(title)


    def Finish(self,
               image_extents : ImageExtents,
               parameters : Parameters,
               time_infos : List[datetime],
               color_infos : List[ColorInfo],
               adjust_color_range : bool):
        c_image_extents = image_extents.get_c_image_extents()
        c_parameters = parameters._get_c_parameters()
        c_time_infos = self._get_c_time_infos(time_infos)
        c_color_infos = self._get_c_color_infos(color_infos)

        self.mcdll.bpImageConverterC_Finish(self.mImageConverterPtr,
                                            c_image_extents,
                                            c_parameters,
                                            c_time_infos,
                                            c_color_infos,
                                            adjust_color_range)
        self._check_errors('bpImageConverterC_Finish')

    def Destroy(self) -> None:
        self.mcdll.bpImageConverterC_Destroy(self.mImageConverterPtr)
        self._check_errors('bpImageConverterC_Destroy')
