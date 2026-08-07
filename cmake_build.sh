#!/bin/bash

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DQT5_INSTALL_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5

# Build the project
make -j$(nproc)

# Install the application
sudo make install

echo "Build complete! You can now run 'wojakcoin-qt' from your terminal."
