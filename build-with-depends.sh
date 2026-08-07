#!/bin/bash
set -e

# Determine the host architecture
HOST="x86_64-pc-linux-gnu"  # Adjust this based on your target platform

# Build dependencies
cd "$(dirname "$0")"

# Check if we're in the correct directory
if [ ! -f "depends/README.usage" ]; then
    echo "Error: This script must be run from the root of the repository"
    exit 1
fi

# Build dependencies
cd depends
make HOST=$HOST -j$(nproc)
cd ..

# Create build directory
mkdir -p build
cd build

# Configure with CMake using depends
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../depends/$HOST/share/toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_DEPENDS=ON \
    -DHOST=$HOST \
    -DCMAKE_INSTALL_PREFIX=/ \
    -DCMAKE_FIND_ROOT_PATH="$(pwd)/../depends/$HOST"

# Build the project
make -j$(nproc)

echo "Build complete! You can find the binaries in the 'build/src' directory."
