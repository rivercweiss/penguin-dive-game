#!/bin/bash

# Test Runner Script for Diving Penguin Game
# Runs all tests according to the Testing Framework Plan

set -e  # Exit on any error

echo "========================================="
echo "Diving Penguin Game - Test Suite Runner"
echo "========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if ESP-IDF environment is set up
check_esp_idf_env() {
    print_status "Checking ESP-IDF environment..."
    if [ -z "$IDF_PATH" ]; then
        print_warning "ESP-IDF environment not detected. Setting up..."
        if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
            source "$HOME/esp/esp-idf/export.sh"
            print_success "ESP-IDF environment loaded"
        else
            print_error "ESP-IDF not found. Please install ESP-IDF first."
            exit 1
        fi
    else
        print_success "ESP-IDF environment detected"
    fi
}

# Run desktop simulator tests
run_simulator_tests() {
    print_status "Running Desktop Simulator Tests..."
    
    cd simulator
    
    # Check if SDL2 is available
    if ! pkg-config --exists sdl2; then
        print_warning "SDL2 not found. Simulator tests require SDL2."
        print_warning "Install SDL2: brew install sdl2 (macOS) or apt-get install libsdl2-dev (Ubuntu)"
        cd ..
        return 1
    fi
    
    # Build and run simulator tests
    if [ -d "build" ]; then
        rm -rf build
    fi
    
    mkdir build
    cd build
    
    if cmake .. && make; then
        print_status "Running simulator integration tests..."
        if ./penguin_simulator_tests; then
            print_success "Desktop simulator tests PASSED"
        else
            print_error "Desktop simulator tests FAILED"
            cd ../..
            return 1
        fi
    else
        print_error "Failed to build simulator tests"
        cd ../..
        return 1
    fi
    
    cd ../..
    return 0
}

# Run component unit tests
run_component_tests() {
    local component=$1
    print_status "Running $component component tests..."
    
    # Create test project for this component
    local test_project_dir="build_tests/$component"
    mkdir -p "$test_project_dir"
    
    # Create CMakeLists.txt for component test
    cat > "$test_project_dir/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS ../../components/$component)
project(test_$component)
EOF
    
    # Create main directory and test main
    mkdir -p "$test_project_dir/main"
    cat > "$test_project_dir/main/CMakeLists.txt" << EOF
idf_component_register(SRCS "test_main.c"
                       REQUIRES unity $component
                       INCLUDE_DIRS "")
EOF
    
    # Copy test files
    cp "components/$component/test/test_$component.c" "$test_project_dir/main/test_main.c"
    
    cd "$test_project_dir"
    
    if idf.py build; then
        print_success "$component component tests compiled successfully"
        cd ../..
        return 0
    else
        print_error "$component component tests compilation FAILED"
        cd ../..
        return 1
    fi
}

# Run all component tests
run_all_component_tests() {
    print_status "Running All Component Unit Tests..."
    
    local components=("game_engine" "penguin_physics" "ice_pillars" "display_driver")
    local failed_tests=()
    
    for component in "${components[@]}"; do
        if ! run_component_tests "$component"; then
            failed_tests+=("$component")
        fi
    done
    
    if [ ${#failed_tests[@]} -eq 0 ]; then
        print_success "All component tests PASSED"
        return 0
    else
        print_error "Component tests FAILED: ${failed_tests[*]}"
        return 1
    fi
}

# Run integration tests
run_integration_tests() {
    print_status "Running Integration Tests..."
    
    # Create integration test project
    local test_project_dir="build_tests/integration"
    mkdir -p "$test_project_dir"
    
    cat > "$test_project_dir/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS 
    ../../components/game_engine
    ../../components/penguin_physics
    ../../components/ice_pillars
    ../../components/display_driver
)
project(test_integration)
EOF
    
    mkdir -p "$test_project_dir/main"
    cat > "$test_project_dir/main/CMakeLists.txt" << EOF
idf_component_register(SRCS "test_main.c"
                       REQUIRES unity game_engine penguin_physics ice_pillars display_driver
                       INCLUDE_DIRS "")
EOF
    
    cp "tests/integration/test_complete_gameplay.c" "$test_project_dir/main/test_main.c"
    
    cd "$test_project_dir"
    
    if idf.py build; then
        print_success "Integration tests compiled successfully"
        cd ../..
        return 0
    else
        print_error "Integration tests compilation FAILED"
        cd ../..
        return 1
    fi
}

# Run hardware tests (compile only, since hardware may not be connected)
run_hardware_tests() {
    print_status "Running Hardware Tests (compilation check)..."
    
    local test_project_dir="build_tests/hardware"
    mkdir -p "$test_project_dir"
    
    cat > "$test_project_dir/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_hardware)
EOF
    
    mkdir -p "$test_project_dir/main"
    cat > "$test_project_dir/main/CMakeLists.txt" << EOF
idf_component_register(SRCS "test_main.c"
                       REQUIRES unity driver
                       INCLUDE_DIRS "")
EOF
    
    cp "tests/hardware/test_m5stickc_plus.c" "$test_project_dir/main/test_main.c"
    
    cd "$test_project_dir"
    
    if idf.py build; then
        print_success "Hardware tests compiled successfully"
        cd ../..
        return 0
    else
        print_error "Hardware tests compilation FAILED"
        cd ../..
        return 1
    fi
}

# Run requirements tests
run_requirements_tests() {
    print_status "Running Requirements-Based Tests..."
    
    local test_project_dir="build_tests/requirements"
    mkdir -p "$test_project_dir"
    
    cat > "$test_project_dir/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS 
    ../../components/game_engine
    ../../components/penguin_physics
    ../../components/ice_pillars
    ../../components/display_driver
)
project(test_requirements)
EOF
    
    mkdir -p "$test_project_dir/main"
    cat > "$test_project_dir/main/CMakeLists.txt" << EOF
idf_component_register(SRCS "test_main.c"
                       REQUIRES unity game_engine penguin_physics ice_pillars display_driver
                       INCLUDE_DIRS "")
EOF
    
    cp "tests/requirements/test_game_requirements.c" "$test_project_dir/main/test_main.c"
    
    cd "$test_project_dir"
    
    if idf.py build; then
        print_success "Requirements tests compiled successfully"
        cd ../..
        return 0
    else
        print_error "Requirements tests compilation FAILED"
        cd ../..
        return 1
    fi
}

# Main test execution
main() {
    print_status "Starting comprehensive test suite..."
    
    # Clean previous test builds
    if [ -d "build_tests" ]; then
        rm -rf build_tests
    fi
    mkdir -p build_tests
    
    local failed_suites=()
    
    # Check ESP-IDF environment
    check_esp_idf_env
    
    # Run desktop simulator tests (if SDL2 available)
    print_status "=== Desktop Simulator Tests ==="
    if ! run_simulator_tests; then
        failed_suites+=("simulator")
    fi
    echo ""
    
    # Run component tests
    print_status "=== Component Unit Tests ==="
    if ! run_all_component_tests; then
        failed_suites+=("components")
    fi
    echo ""
    
    # Run integration tests
    print_status "=== Integration Tests ==="
    if ! run_integration_tests; then
        failed_suites+=("integration")
    fi
    echo ""
    
    # Run hardware tests
    print_status "=== Hardware Tests ==="
    if ! run_hardware_tests; then
        failed_suites+=("hardware")
    fi
    echo ""
    
    # Run requirements tests
    print_status "=== Requirements Tests ==="
    if ! run_requirements_tests; then
        failed_suites+=("requirements")
    fi
    echo ""
    
    # Summary
    echo "========================================="
    echo "Test Suite Summary"
    echo "========================================="
    
    if [ ${#failed_suites[@]} -eq 0 ]; then
        print_success "🎉 ALL TEST SUITES PASSED! 🎉"
        print_success "The Diving Penguin game components are ready for development!"
        echo ""
        print_status "Next steps:"
        print_status "1. Run 'cd simulator && mkdir build && cd build && cmake .. && make' to build simulator"
        print_status "2. Run './penguin_simulator' to test the game interactively"
        print_status "3. Use 'idf.py build flash monitor' to deploy to hardware"
        exit 0
    else
        print_error "❌ TEST FAILURES DETECTED ❌"
        print_error "Failed test suites: ${failed_suites[*]}"
        echo ""
        print_status "Check the error messages above to fix the failing tests."
        exit 1
    fi
}

# Run main function
main "$@"