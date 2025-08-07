#!/bin/bash

# Build and run the Penguin Dive Game simulator

set -e  # Exit on any error

echo "Building simulator..."
cd simulator
mkdir -p build
cd build
cmake ..
make

echo "Running simulator..."
./penguin_simulator