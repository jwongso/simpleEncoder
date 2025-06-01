#!/bin/bash

# -------------------------------------------------------------------------------------------------
#
# Copyright (C) all of the contributors. All rights reserved.
#
# This software, including documentation, is protected by copyright controlled by
# contributors. All rights are reserved. Copying, including reproducing, storing,
# adapting or translating, any or all of this material requires the prior written
# consent of all contributors.
#
# -------------------------------------------------------------------------------------------------

# validate_encoded_files.sh
# Comprehensive script for automating the validation of encoded audio files
# This script demonstrates best practices for validating encoder output

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_WAV_DIR="$PROJECT_ROOT/test_wav"
OUTPUT_DIR="$PROJECT_ROOT/encoded_output"
VALIDATION_REPORT="$PROJECT_ROOT/validation_report.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if required tools are available
check_dependencies() {
    log_info "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for required audio analysis tools (optional but recommended)
    if command -v ffprobe >/dev/null 2>&1; then
        log_success "ffprobe found - advanced validation available"
        FFPROBE_AVAILABLE=true
    else
        log_warning "ffprobe not found - basic validation only"
        FFPROBE_AVAILABLE=false
    fi
    
    if command -v mediainfo >/dev/null 2>&1; then
        log_success "mediainfo found - enhanced metadata analysis available"
        MEDIAINFO_AVAILABLE=true
    else
        log_warning "mediainfo not found - limited metadata analysis"
        MEDIAINFO_AVAILABLE=false
    fi
    
    # Check for file command
    if ! command -v file >/dev/null 2>&1; then
        missing_deps+=("file")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        log_error "Missing required dependencies: ${missing_deps[*]}"
        log_info "Please install missing dependencies and try again"
        exit 1
    fi
    
    log_success "All required dependencies found"
}

# Function to validate file format using file command
validate_with_file_command() {
    local file_path="$1"
    local expected_format="$2"
    
    local file_output
    file_output=$(file "$file_path" 2>/dev/null || echo "unknown")
    
    case "$expected_format" in
        "mp3")
            if echo "$file_output" | grep -qi "mpeg\|mp3"; then
                return 0
            fi
            ;;
        "aac")
            if echo "$file_output" | grep -qi "aac\|adts"; then
                return 0
            fi
            ;;
        "flac")
            if echo "$file_output" | grep -qi "flac"; then
                return 0
            fi
            ;;
        "opus")
            if echo "$file_output" | grep -qi "opus\|ogg"; then
                return 0
            fi
            ;;
        "vorbis")
            if echo "$file_output" | grep -qi "vorbis\|ogg"; then
                return 0
            fi
            ;;
    esac
    
    return 1
}

# Function to validate file with ffprobe (if available)
validate_with_ffprobe() {
    local file_path="$1"
    local format_info
    
    if [ "$FFPROBE_AVAILABLE" = false ]; then
        return 1
    fi
    
    # Try to get format information
    if format_info=$(ffprobe -v quiet -print_format json -show_format "$file_path" 2>/dev/null); then
        # Check if we got valid JSON output
        if echo "$format_info" | jq -e '.format' >/dev/null 2>&1; then
            return 0
        fi
    fi
    
    return 1
}

# Function to get detailed file information
get_file_info() {
    local file_path="$1"
    local file_size
    local file_format
    
    file_size=$(stat -f%z "$file_path" 2>/dev/null || stat -c%s "$file_path" 2>/dev/null || echo "unknown")
    file_format=$(file "$file_path" 2>/dev/null || echo "unknown")
    
    echo "Size: $file_size bytes"
    echo "Format: $file_format"
    
    # Add ffprobe info if available
    if [ "$FFPROBE_AVAILABLE" = true ]; then
        local probe_info
        if probe_info=$(ffprobe -v quiet -print_format json -show_format -show_streams "$file_path" 2>/dev/null); then
            echo "FFprobe analysis:"
            echo "$probe_info" | jq -r '
                if .format then
                    "  Duration: " + (.format.duration // "unknown") + " seconds",
                    "  Bitrate: " + (.format.bit_rate // "unknown") + " bps",
                    "  Format: " + (.format.format_name // "unknown")
                else empty end,
                if .streams then
                    .streams[] | select(.codec_type == "audio") |
                    "  Codec: " + (.codec_name // "unknown"),
                    "  Sample Rate: " + (.sample_rate // "unknown") + " Hz",
                    "  Channels: " + (.channels // "unknown" | tostring)
                else empty end
            ' 2>/dev/null || echo "  Analysis failed"
        fi
    fi
    
    # Add mediainfo if available
    if [ "$MEDIAINFO_AVAILABLE" = true ]; then
        echo "MediaInfo analysis:"
        mediainfo --Output=JSON "$file_path" 2>/dev/null | jq -r '
            .media.track[] | select(.["@type"] == "Audio") |
            "  Duration: " + (.Duration // "unknown") + " ms",
            "  Bitrate: " + (.BitRate // "unknown") + " bps",
            "  Sample Rate: " + (.SamplingRate // "unknown") + " Hz",
            "  Channels: " + (.Channels // "unknown")
        ' 2>/dev/null || echo "  Analysis failed"
    fi
}

# Function to validate a single encoded file
validate_encoded_file() {
    local file_path="$1"
    local original_wav="$2"
    local expected_format="$3"
    local validation_passed=true
    
    echo "----------------------------------------"
    echo "Validating: $(basename "$file_path")"
    echo "Expected format: $expected_format"
    echo "Original WAV: $(basename "$original_wav")"
    
    # Check if file exists
    if [ ! -f "$file_path" ]; then
        log_error "File does not exist: $file_path"
        return 1
    fi
    
    # Check if file is not empty
    if [ ! -s "$file_path" ]; then
        log_error "File is empty: $file_path"
        return 1
    fi
    
    # Validate format using file command
    if validate_with_file_command "$file_path" "$expected_format"; then
        log_success "Format validation passed (file command)"
    else
        log_warning "Format validation failed (file command)"
        validation_passed=false
    fi
    
    # Validate with ffprobe if available
    if validate_with_ffprobe "$file_path"; then
        log_success "Format validation passed (ffprobe)"
    else
        if [ "$FFPROBE_AVAILABLE" = true ]; then
            log_warning "Format validation failed (ffprobe)"
            validation_passed=false
        fi
    fi
    
    # Get detailed file information
    echo "File information:"
    get_file_info "$file_path"
    
    # Compare file sizes (basic compression check)
    if [ -f "$original_wav" ]; then
        local original_size encoded_size compression_ratio
        original_size=$(stat -f%z "$original_wav" 2>/dev/null || stat -c%s "$original_wav" 2>/dev/null || echo "0")
        encoded_size=$(stat -f%z "$file_path" 2>/dev/null || stat -c%s "$file_path" 2>/dev/null || echo "0")
        
        if [ "$original_size" -gt 0 ] && [ "$encoded_size" -gt 0 ]; then
            compression_ratio=$(echo "scale=2; $encoded_size * 100 / $original_size" | bc -l 2>/dev/null || echo "unknown")
            echo "Compression ratio: $compression_ratio% of original size"
            
            # Check if compression is reasonable (encoded file should be smaller for lossy formats)
            case "$expected_format" in
                "mp3"|"aac"|"opus"|"vorbis")
                    if [ "$encoded_size" -ge "$original_size" ]; then
                        log_warning "Encoded file is not smaller than original (unexpected for lossy format)"
                    else
                        log_success "Good compression achieved"
                    fi
                    ;;
                "flac")
                    if [ "$encoded_size" -ge "$original_size" ]; then
                        log_warning "FLAC file is not smaller than original WAV"
                    else
                        log_success "Lossless compression achieved"
                    fi
                    ;;
            esac
        fi
    fi
    
    if [ "$validation_passed" = true ]; then
        log_success "Overall validation: PASSED"
        return 0
    else
        log_error "Overall validation: FAILED"
        return 1
    fi
}

# Function to run encoder and validate output
test_encoder_format() {
    local format="$1"
    local encoder_executable="$BUILD_DIR/simpleEncoder"
    
    log_info "Testing $format encoder..."
    
    # Create format-specific output directory
    local format_output_dir="$OUTPUT_DIR/$format"
    mkdir -p "$format_output_dir"
    
    # Run encoder
    if [ -f "$encoder_executable" ]; then
        log_info "Running encoder for $format format..."
        if "$encoder_executable" --input "$TEST_WAV_DIR" --output "$format_output_dir" --format "$format" --threads 2 >/dev/null 2>&1; then
            log_success "Encoder completed successfully for $format"
        else
            log_warning "Encoder may have encountered issues for $format (check manually)"
        fi
    else
        log_warning "Encoder executable not found, skipping encoding step"
    fi
    
    # Validate output files
    local validation_results=()
    local total_files=0
    local passed_files=0
    
    # Get expected file extension
    local extension
    case "$format" in
        "mp3") extension="mp3" ;;
        "aac") extension="aac" ;;
        "flac") extension="flac" ;;
        "opus") extension="opus" ;;
        "vorbis") extension="ogg" ;;
        *) extension="unknown" ;;
    esac
    
    # Find and validate encoded files
    if [ -d "$format_output_dir" ]; then
        while IFS= read -r -d '' encoded_file; do
            total_files=$((total_files + 1))
            
            # Find corresponding original WAV file
            local base_name
            base_name=$(basename "$encoded_file" ".$extension")
            local original_wav="$TEST_WAV_DIR/$base_name.wav"
            
            if validate_encoded_file "$encoded_file" "$original_wav" "$format"; then
                passed_files=$((passed_files + 1))
            fi
            
        done < <(find "$format_output_dir" -name "*.$extension" -print0 2>/dev/null)
    fi
    
    # Report results for this format
    echo "========================================"
    echo "$format Validation Summary:"
    echo "Total files: $total_files"
    echo "Passed validation: $passed_files"
    echo "Failed validation: $((total_files - passed_files))"
    
    if [ "$total_files" -eq 0 ]; then
        log_warning "No $format files found for validation"
        return 1
    elif [ "$passed_files" -eq "$total_files" ]; then
        log_success "All $format files passed validation"
        return 0
    else
        log_error "Some $format files failed validation"
        return 1
    fi
}

# Function to generate comprehensive validation report
generate_validation_report() {
    log_info "Generating comprehensive validation report..."
    
    {
        echo "=========================================="
        echo "Audio Encoder Validation Report"
        echo "Generated: $(date)"
        echo "=========================================="
        echo
        
        echo "Test Configuration:"
        echo "- Project Root: $PROJECT_ROOT"
        echo "- Test WAV Directory: $TEST_WAV_DIR"
        echo "- Output Directory: $OUTPUT_DIR"
        echo "- FFprobe Available: $FFPROBE_AVAILABLE"
        echo "- MediaInfo Available: $MEDIAINFO_AVAILABLE"
        echo
        
        echo "Input Files:"
        if [ -d "$TEST_WAV_DIR" ]; then
            find "$TEST_WAV_DIR" -name "*.wav" -exec basename {} \; | sort
        else
            echo "No test WAV directory found"
        fi
        echo
        
        echo "Validation Results by Format:"
        echo "=============================="
        
        # Validate each format
        for format in mp3 aac flac opus vorbis; do
            echo
            echo "--- $format ---"
            local format_output_dir="$OUTPUT_DIR/$format"
            
            if [ -d "$format_output_dir" ]; then
                local file_count
                file_count=$(find "$format_output_dir" -type f | wc -l)
                echo "Files found: $file_count"
                
                if [ "$file_count" -gt 0 ]; then
                    echo "File list:"
                    find "$format_output_dir" -type f -exec basename {} \; | sort
                    
                    echo "Detailed analysis:"
                    find "$format_output_dir" -type f | while read -r file; do
                        echo "  File: $(basename "$file")"
                        get_file_info "$file" | sed 's/^/    /'
                        echo
                    done
                fi
            else
                echo "No output directory found for $format"
            fi
        done
        
        echo
        echo "Recommendations:"
        echo "================"
        echo "1. Verify all expected output files are present"
        echo "2. Check file sizes are reasonable for each format"
        echo "3. Ensure lossy formats achieve good compression ratios"
        echo "4. Validate audio quality manually for critical applications"
        echo "5. Test with various input file types and sample rates"
        
    } > "$VALIDATION_REPORT"
    
    log_success "Validation report generated: $VALIDATION_REPORT"
}

# Main function
main() {
    echo "=========================================="
    echo "Audio Encoder Validation Script"
    echo "=========================================="
    
    # Check dependencies
    check_dependencies
    
    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    
    # Check if test WAV files exist
    if [ ! -d "$TEST_WAV_DIR" ]; then
        log_error "Test WAV directory not found: $TEST_WAV_DIR"
        log_info "Please ensure test WAV files are available in the test_wav directory"
        exit 1
    fi
    
    local wav_count
    wav_count=$(find "$TEST_WAV_DIR" -name "*.wav" | wc -l)
    if [ "$wav_count" -eq 0 ]; then
        log_error "No WAV files found in $TEST_WAV_DIR"
        exit 1
    fi
    
    log_info "Found $wav_count WAV files for testing"
    
    # Test each encoder format
    local overall_success=true
    local formats=("mp3" "aac" "flac" "opus" "vorbis")
    
    for format in "${formats[@]}"; do
        echo
        if ! test_encoder_format "$format"; then
            overall_success=false
        fi
    done
    
    # Generate comprehensive report
    generate_validation_report
    
    # Final summary
    echo
    echo "=========================================="
    echo "Final Summary"
    echo "=========================================="
    
    if [ "$overall_success" = true ]; then
        log_success "All encoder formats passed validation"
        echo "Validation report: $VALIDATION_REPORT"
        exit 0
    else
        log_error "Some encoder formats failed validation"
        echo "Check the validation report for details: $VALIDATION_REPORT"
        exit 1
    fi
}

# Script usage information
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -v, --verbose  Enable verbose output"
    echo
    echo "This script validates encoded audio files by:"
    echo "1. Running encoders on test WAV files"
    echo "2. Validating output file formats and integrity"
    echo "3. Analyzing compression ratios and metadata"
    echo "4. Generating a comprehensive validation report"
    echo
    echo "Prerequisites:"
    echo "- Built simpleEncoder executable in build/ directory"
    echo "- Test WAV files in test_wav/ directory"
    echo "- Optional: ffprobe and mediainfo for enhanced validation"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -v|--verbose)
            set -x
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Run main function
main "$@"
