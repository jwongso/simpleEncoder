# simpleEncoder
Very simple wave to mp3 (and other formats) encoder

## Dependencies
- cmake
- g++ (C++14 and newer)
- thread
- libmp3lame or lame mpr library

## Build
- `mkdir buiild && cd build`
- `cmake -DCMAKE_BUILD_TYPE=Debug ../` or `cmake -DCMAKE_BUILD_TYPE=Release ../` for release build
- `make -j8`

## Usage
Input: folder contains .wav/.wave files
Output: all encoded files within the input folders with different encoding format/extension e.g., .mp3
- `cd build`
- `./simpleEncoder ../test_data`
