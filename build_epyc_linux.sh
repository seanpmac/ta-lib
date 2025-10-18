#!/bin/bash
# TA-Lib Build Script for AMD EPYC (Linux)
# Optimized for Zen 2/3/4 architecture with full multi-core + SIMD

echo "=== TA-Lib Build for AMD EPYC ==="
echo ""

# Detect CPU
if grep -q "AMD EPYC" /proc/cpuinfo; then
    echo "✓ AMD EPYC detected"
    CORES=$(nproc)
    echo "  Cores: $CORES"
else
    echo "⚠ Warning: Not running on AMD EPYC"
    CORES=$(nproc)
fi

# Detect Zen generation
MODEL=$(grep "model name" /proc/cpuinfo | head -1)
if echo "$MODEL" | grep -q "7003"; then
    ARCH="znver3"
    echo "  Architecture: Zen 3 (Milan)"
elif echo "$MODEL" | grep -q "9004"; then
    ARCH="znver4"
    echo "  Architecture: Zen 4 (Genoa)"
    AVX512="-mavx512f -mavx512dq"
elif echo "$MODEL" | grep -q "7002"; then
    ARCH="znver2"
    echo "  Architecture: Zen 2 (Rome)"
else
    ARCH="native"
    echo "  Architecture: Auto-detect (native)"
fi

echo ""
echo "Building with optimizations:"
echo "  - Multi-threading: $CORES cores"
echo "  - SIMD: AVX2 + FMA"
echo "  - OpenMP: Full support"
echo "  - Phase 1 optimizations: Active"
echo ""

# Clean
rm -rf build-epyc
mkdir -p build-epyc

# Configure with AMD-optimized flags
cd build-epyc
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-march=$ARCH -mavx2 -mfma $AVX512 -fopenmp -O3 -ffast-math" \
    -DCMAKE_CXX_FLAGS="-march=$ARCH -mavx2 -mfma $AVX512 -fopenmp -O3 -ffast-math"

if [ $? -ne 0 ]; then
    echo "Configuration failed!"
    exit 1
fi

# Build using all cores
echo ""
echo "Building with $CORES parallel jobs..."
cmake --build . -j $CORES

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "=== Build Complete ==="
echo ""

# Run tests
if [ -f "bin/ta_regtest" ]; then
    echo "Running regression tests..."
    echo ""
    ./bin/ta_regtest
    echo ""
fi

# Run performance profiling
if [ -f "bin/ta_perf" ]; then
    echo "Running performance profiling with $CORES threads..."
    echo ""
    ./bin/ta_perf --threads $CORES
fi

echo ""
echo "=== Done ==="
echo ""
echo "Executables in: $(pwd)/bin/"
echo "  - ta_regtest : Regression tests"
echo "  - ta_perf    : Performance profiling"
