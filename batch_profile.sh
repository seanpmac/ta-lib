#!/bin/bash
# Batch profiling script for multiple forex data files
# Runs ta_perf on all CSV files in a directory and aggregates results

set -e

# Configuration
DATA_DIR="/Users/Sean/GitHub/QuantConnect/Lean/Data/forex/oanda/second/eurusd"
OUTPUT_DIR="./profiling_results"
PERF_BIN="./build/bin/ta_perf"
THREADS=8

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== TA-Lib Multi-File Performance Profiler ===${NC}"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Find all CSV files
CSV_FILES=($DATA_DIR/*.csv)
FILE_COUNT=${#CSV_FILES[@]}

if [ $FILE_COUNT -eq 0 ]; then
    echo "No CSV files found in $DATA_DIR"
    exit 1
fi

echo -e "${GREEN}Found $FILE_COUNT CSV files${NC}"
echo ""

# Create summary CSV header
SUMMARY_FILE="$OUTPUT_DIR/batch_summary.csv"
echo "file,bars,serial_time_ms,parallel_time_ms,threads,speedup,efficiency,indicators" > "$SUMMARY_FILE"

# Process each file
for ((i=0; i<$FILE_COUNT; i++)); do
    CSV_FILE="${CSV_FILES[$i]}"
    BASENAME=$(basename "$CSV_FILE" .csv)
    JSON_OUT="$OUTPUT_DIR/${BASENAME}.json"
    
    PROGRESS=$((i + 1))
    echo -e "${YELLOW}[$PROGRESS/$FILE_COUNT]${NC} Processing $BASENAME..."
    
    # Run profiler in both modes
    $PERF_BIN --input "$CSV_FILE" --mode both --threads $THREADS --output "$JSON_OUT" > /dev/null 2>&1
    
    # Extract metrics using grep/awk (more robust than jq for control characters)
    BARS=$(grep '"bar_count"' "$JSON_OUT" | head -1 | awk -F': ' '{print $2}' | tr -d ',')
    
    # Get serial metrics (in "serial" section, first total_time_us after it)
    SERIAL_TIME=$(grep -A50 '"serial"' "$JSON_OUT" | grep '"total_time_us"' | head -1 | awk -F': ' '{print $2}' | tr -d ',')
    
    # Get parallel metrics (in "parallel" section)
    PARALLEL_TIME=$(grep -A5 '"parallel"' "$JSON_OUT" | grep '"total_time_us"' | head -1 | awk -F': ' '{print $2}' | tr -d ',')
    THREADS_USED=$(grep -A5 '"parallel"' "$JSON_OUT" | grep '"thread_count"' | head -1 | awk -F': ' '{print $2}' | tr -d ',')
    SPEEDUP=$(grep -A5 '"parallel"' "$JSON_OUT" | grep '"speedup"' | head -1 | awk -F': ' '{print $2}' | tr -d ',')
    EFFICIENCY=$(grep -A5 '"parallel"' "$JSON_OUT" | grep '"efficiency"' | head -1 | awk -F': ' '{print $2}' | tr -d ',')
    
    # Count indicators
    INDICATORS=$(grep -c '"name"' "$JSON_OUT")
    if [ "$INDICATORS" -gt 0 ]; then
        INDICATORS=$((INDICATORS / 2))  # Serial + parallel entries
    fi
    
    # Convert microseconds to milliseconds
    SERIAL_MS=$(echo "scale=3; $SERIAL_TIME / 1000" | bc)
    PARALLEL_MS=$(echo "scale=3; $PARALLEL_TIME / 1000" | bc)
    
    # Append to CSV
    echo "$BASENAME,$BARS,$SERIAL_MS,$PARALLEL_MS,$THREADS_USED,$SPEEDUP,$EFFICIENCY,$INDICATORS" >> "$SUMMARY_FILE"
    
    echo "  ✓ $BARS bars, Serial: ${SERIAL_MS}ms, Parallel: ${PARALLEL_MS}ms, Speedup: ${SPEEDUP}x"
done

echo ""
echo -e "${GREEN}=== Batch Profiling Complete ===${NC}"
echo ""
echo "Results saved to:"
echo "  • Summary CSV: $SUMMARY_FILE"
echo "  • Individual:  $OUTPUT_DIR/*.json"
echo ""

# Calculate aggregate statistics
echo -e "${BLUE}=== Aggregate Statistics ===${NC}"
if command -v awk &> /dev/null; then
    TOTAL_BARS=$(awk -F, 'NR>1 {sum+=$2} END {print sum}' "$SUMMARY_FILE")
    AVG_SPEEDUP=$(awk -F, 'NR>1 {sum+=$6; count++} END {printf "%.2f", sum/count}' "$SUMMARY_FILE")
    AVG_EFFICIENCY=$(awk -F, 'NR>1 {sum+=$7; count++} END {printf "%.3f", sum/count}' "$SUMMARY_FILE")
    TOTAL_SERIAL=$(awk -F, 'NR>1 {sum+=$3} END {printf "%.2f", sum}' "$SUMMARY_FILE")
    TOTAL_PARALLEL=$(awk -F, 'NR>1 {sum+=$4} END {printf "%.2f", sum}' "$SUMMARY_FILE")
    
    echo "Files processed:    $FILE_COUNT"
    echo "Total bars:         $TOTAL_BARS"
    echo "Total indicators:   $(head -2 "$SUMMARY_FILE" | tail -1 | cut -d',' -f8)"
    echo "Average speedup:    ${AVG_SPEEDUP}x"
    echo "Average efficiency: $(echo "$AVG_EFFICIENCY * 100" | bc)%"
    echo ""
    echo "Cumulative time (all files):"
    echo "  Serial:   ${TOTAL_SERIAL}ms"
    echo "  Parallel: ${TOTAL_PARALLEL}ms"
    echo "  Time saved: $(echo "$TOTAL_SERIAL - $TOTAL_PARALLEL" | bc)ms"
fi

echo ""
echo "View summary: cat $SUMMARY_FILE"
