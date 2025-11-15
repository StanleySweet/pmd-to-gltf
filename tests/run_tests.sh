#!/bin/bash

# Test Runner Script for PMD to glTF Converter
# Runs all unit tests and integration tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "=== PMD to glTF Converter Test Suite ==="
echo

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Build directory not found. Building project..."
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    echo
fi

# Run all tests
echo "Running all tests..."
cd build
ctest --output-on-failure --verbose

# Check test results
if [ $? -eq 0 ]; then
    echo
    echo "✅ All tests passed!"
    echo
    
    # Run a quick integration test manually
    echo "Running manual integration test..."
    if [ -f "../input/horse.pmd" ]; then
        ./converter ../input/horse > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "✅ Manual integration test passed!"
            ls -lh ../output/horse.gltf
        else
            echo "❌ Manual integration test failed!"
            exit 1
        fi
    else
        echo "⚠️  No test input files found, skipping manual integration test"
    fi
else
    echo
    echo "❌ Some tests failed!"
    exit 1
fi

echo
echo "🎉 Test suite completed successfully!"