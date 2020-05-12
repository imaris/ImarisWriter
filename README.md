# ImarisWriter

High-performance image writer library.

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
  make
  ```

- Debug
  
  ```bash
  mkdir debug
  cd debug
  cmake -DHDF5_ROOT:PATH="<libs>/hdf5" -DZLIB_ROOT:PATH="<libs>/zlib" -DLZ4_ROOT:PATH="<libs>/lz4" -DCMAKE_BUILD_TYPE=Debug ..
  make
  ```
  
