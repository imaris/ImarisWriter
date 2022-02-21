# ImarisWriter

ImarisWriter is a high performance file writer for microscopy images. It creates image files suitable for high performance visualization and analysis in the [Imaris5 File Format](https://github.com/imaris/ImarisWriter/blob/master/doc/Imaris5FileFormat.pdf). The library facilitates writing of very large image data that exceed a computer’s RAM by “streaming” the data to the library in small blocks. It is capable of writing data with high speed. And it takes care of all the details of multi-resolution resampling, chunking, compression, multi-threading, etc., and delivers its functionality to the user in a simple to use way.

For details refer to our publication: Beati, Igor, Eliana Andreica, and Peter Majer. "ImarisWriter: open source software for storage of large images in blockwise multi-resolution format." arXiv preprint [arXiv:2008.10311](https://arxiv.org/abs/2008.10311) (2020).

If ImarisWriter is useful for your work and if you want to help us continue our open source efforts please cite our publication in your work.

### Usage
The ImarisWriter library has a C++ API, a C API, and a Python API. Using the C++ API pseudocode for writing a file mainly consists of a loop to copy all blocks to the library:

```C++
bpImageConverter<bpUInt16> vImageConverter(...);
  
for ( all blocks ) { 
 vImageConverter::CopyBlock(vBlockData, vBlockPosition);
}

vImageConverter::Finish(vParameters, ...);
```
A full usage example in C++ as well as a full example in C can be found here: https://github.com/imaris/ImarisWriterTest

### Dependencies

1. hdf5 version >= 1.10.4: https://www.hdfgroup.org/downloads/hdf5/ (compile with default options, only base C module is required)
1. zlib: https://www.zlib.net/ (compile with default options)
1. lz4: https://github.com/lz4/lz4 (compile with default options)

### Build

- Release
  
  ```bash
  mkdir release
  cd release
  cmake -DHDF5_ROOT:PATH="<libs>/hdf5" -DZLIB_ROOT:PATH="<libs>/zlib" -DLZ4_ROOT:PATH="<libs>/lz4" ..
  ```

- Debug
  
  ```bash
  mkdir debug
  cd debug
  cmake -DHDF5_ROOT:PATH="<libs>/hdf5" -DZLIB_ROOT:PATH="<libs>/zlib" -DLZ4_ROOT:PATH="<libs>/lz4" -DCMAKE_BUILD_TYPE=Debug ..
  ```
  
On Windows, the generated solution files can be opened and compiled with Visual Studio, while on Linux and Mac the generated Makefile can be compiled with ```make```. The Visual Studio version should be specified according to the setup of the other libraries, e.g. adding ```-G "Visual Studio 14 Win64"```.
