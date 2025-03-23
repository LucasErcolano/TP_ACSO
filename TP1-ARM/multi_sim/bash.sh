#!/bin/bash

# Check if an input file was provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <input_file.x>"
    exit 1
fi

INPUT_FILE=$1
SIM="./sim"
REF_SIM="./ref_sim"

# Check if both simulators exist
if [ ! -x "$SIM" ]; then
    echo "Error: $SIM not found or not executable"
    exit 1
fi

if [ ! -x "$REF_SIM" ]; then
    echo "Error: $REF_SIM not found or not executable"
    exit 1
fi

# Create temporary files for the simulators' inputs and outputs
SIM_IN=$(mktemp)
SIM_OUT=$(mktemp)
REF_SIM_OUT=$(mktemp)

# Function to clean up temporary files
cleanup() {
    rm -f "$SIM_IN" "$SIM_OUT" "$REF_SIM_OUT"
    exit 0
}

# Set up trap to clean up on exit
trap cleanup EXIT INT TERM

# Start both simulators in background
"$SIM" "$INPUT_FILE" < "$SIM_IN" > "$SIM_OUT" 2>&1 &
SIM_PID=$!

"$REF_SIM" "$INPUT_FILE" < "$SIM_IN" > "$REF_SIM_OUT" 2>&1 &
REF_SIM_PID=$!

# Main loop
echo "=== SIMULATOR COMPARISON ==="
echo "Your simulator on the left, reference simulator on the right"
echo "Type your commands below (^D to exit):"
echo "=========================================================="

while IFS= read -r line; do
    # Write the command to the input file
    echo "$line" > "$SIM_IN"
    
    # Send the command to both simulators
    kill -PIPE $SIM_PID 2>/dev/null
    kill -PIPE $REF_SIM_PID 2>/dev/null
    
    # Wait a moment for the simulators to process
    sleep 0.1
    
    # Display outputs side by side
    echo "=== YOUR SIMULATOR ==="
    cat "$SIM_OUT"
    echo ""
    echo "=== REFERENCE SIMULATOR ==="
    cat "$REF_SIM_OUT"
    echo "=========================================================="
    
    # Clean output files for next command
    > "$SIM_OUT"
    > "$REF_SIM_OUT"
done

echo "Exiting simulator comparison..."