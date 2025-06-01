# simpleEncoder

A simple command-line tool for encoding WAV files to various audio formats like MP3, FLAC, AAC, Vorbis, and Opus.

## Features
- Supports multiple audio formats: MP3, FLAC, AAC, Vorbis, Opus
- Multi-threaded encoding for faster processing
- Simple CLI interface
- Verbose mode for detailed output

## Dependencies
- CMake (for building)
- g++ (C++14 or newer)
- libmp3lame (for MP3 encoding)
- libfdk_aac (for AAC; note: requires FFmpeg with --enable-libfdk-aac, nonfree license)
- libvorbis and libogg (for Vorbis)
- libopus (for Opus)
- libFLAC (for FLAC)

**Note:** For GPL compliance, use FFmpeg's native AAC encoder instead of libfdk_aac if needed.

## Build Instructions
1. Create a build directory: `mkdir build && cd build`
2. Configure with CMake: `cmake -DCMAKE_BUILD_TYPE=Release ..` (or `Debug` for debugging)
3. Build: `make -j8` (adjust `-j` based on your CPU cores)

## Usage
Convert WAV files in a directory to the specified format.

```
Audio Encoder - Convert WAV files to various formats
Usage: ./simpleEncoder [OPTIONS]

Options:
  -h, --help                   Print this help message and exit
  -i, --input TEXT:DIR REQUIRED
                               Input directory containing WAV files
  -o, --output TEXT REQUIRED   Output directory for encoded files
  -f, --format TEXT:{flac,mp3,opus,aac,vorbis} [flac]
                               Output format
  -j, --threads UINT:INT in [1 - 24] [12] 
                               Number of encoding threads
  -v, --verbose [0]            Enable verbose output
```

### Examples
- Encode to FLAC with 4 threads: `./simpleEncoder -i ../test -o output -f flac -j 4`
- Encode to MP3: `./simpleEncoder -i ../test -o output -f mp3`
- Encode to AAC with verbose output: `./simpleEncoder -i ../test -o output -f aac -v`

## License
This project is licensed under [insert license, e.g., MIT]. See the LICENSE file for details. (Note: If no LICENSE file exists, consider adding one.)
