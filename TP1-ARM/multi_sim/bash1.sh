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

# Create directory for temporary files
TEMP_DIR=$(mktemp -d)
SIM_IN="$TEMP_DIR/sim_in"
REF_SIM_IN="$TEMP_DIR/ref_sim_in"
SIM_OUT="$TEMP_DIR/sim_out"
REF_SIM_OUT="$TEMP_DIR/ref_sim_out"

# Create named pipes
mkfifo "$SIM_IN"
mkfifo "$REF_SIM_IN"

# Function to clean up temporary files
cleanup() {
    rm -rf "$TEMP_DIR"
    kill $SIM_PID $REF_SIM_PID 2>/dev/null
    exit 0
}

# Set up trap to clean up on exit
trap cleanup EXIT INT TERM

# Start simulators in background with named pipes
"$SIM" "$INPUT_FILE" < "$SIM_IN" > "$SIM_OUT" 2>&1 &
SIM_PID=$!

"$REF_SIM" "$INPUT_FILE" < "$REF_SIM_IN" > "$REF_SIM_OUT" 2>&1 &
REF_SIM_PID=$!

# Function to handle user input and send to both simulators
process_input() {
    local cmd="$1"
    
    # Send to your simulator
    echo "$cmd" > "$SIM_IN" &
    
    # Send to reference simulator
    echo "$cmd" > "$REF_SIM_IN" &
    
    # Wait for output
    sleep 0.2
    
    # Display outputs
    echo "=== YOUR SIMULATOR ==="
    if [ -f "$SIM_OUT" ]; then
        cat "$SIM_OUT"
        > "$SIM_OUT"
    else
        echo "No output"
    fi
    
    echo ""
    echo "=== REFERENCE SIMULATOR ==="
    if [ -f "$REF_SIM_OUT" ]; then
        cat "$REF_SIM_OUT"
        > "$REF_SIM_OUT"
    else
        echo "No output"
    fi
    echo "=========================================================="
}

# Main loop
echo "=== SIMULATOR COMPARISON ==="
echo "Your simulator on the left, reference simulator on the right"
echo "Type your commands below (Ctrl+C to exit):"
echo "=========================================================="

while true; do
    read -p "> " cmd
    if [ $? -ne 0 ]; then
        break
    fi
    process_input "$cmd"
done

echo "Exiting simulator comparison..."