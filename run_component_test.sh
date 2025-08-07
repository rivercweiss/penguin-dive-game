#!/bin/bash

# Individual Component Test Runner
# Usage: ./run_component_test.sh <component_name>
# Example: ./run_component_test.sh game_engine

set -e

if [ $# -eq 0 ]; then
    echo "Usage: $0 <component_name>"
    echo "Available components: game_engine, penguin_physics, ice_pillars, display_driver"
    exit 1
fi

COMPONENT=$1

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if component exists
if [ ! -d "components/$COMPONENT" ]; then
    print_error "Component '$COMPONENT' not found!"
    print_status "Available components:"
    ls -1 components/
    exit 1
fi

# Check ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    print_status "Setting up ESP-IDF environment..."
    source "$HOME/esp/esp-idf/export.sh"
fi

print_status "Running tests for component: $COMPONENT"

# Create test project
TEST_DIR="build_test_$COMPONENT"
if [ -d "$TEST_DIR" ]; then
    rm -rf "$TEST_DIR"
fi
mkdir "$TEST_DIR"

# Create CMakeLists.txt
cat > "$TEST_DIR/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS ../components/$COMPONENT)
project(test_$COMPONENT)
EOF

# Create main directory
mkdir "$TEST_DIR/main"
cat > "$TEST_DIR/main/CMakeLists.txt" << EOF
idf_component_register(SRCS "test_main.c"
                       REQUIRES unity $COMPONENT
                       INCLUDE_DIRS "")
EOF

# Copy test file
cp "components/$COMPONENT/test/test_$COMPONENT.c" "$TEST_DIR/main/test_main.c"

# Build and run
cd "$TEST_DIR"

print_status "Building $COMPONENT tests..."
if idf.py build; then
    print_success "✅ $COMPONENT tests compiled successfully!"
    
    # Check if device is connected for actual testing
    if idf.py device-monitor 2>/dev/null | grep -q "serial"; then
        print_status "Device detected. Flashing and running tests..."
        idf.py flash monitor
    else
        print_status "No device detected. Tests compiled successfully but not executed on hardware."
        print_status "To run on hardware: idf.py -p /dev/cu.usbserial-* flash monitor"
    fi
else
    print_error "❌ $COMPONENT tests compilation FAILED!"
    cd ..
    exit 1
fi

cd ..
print_success "Component test completed for: $COMPONENT"