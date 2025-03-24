#!/bin/bash

# Check if directory argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <directory_with_x_files>"
    exit 1
fi

DIR="$1"

# Check if directory exists
if [ ! -d "$DIR" ]; then
    echo "Directory $DIR does not exist"
    exit 1
fi

# Check if simulators exist
if [ ! -f "./ref_sim_x86" ]; then
    echo "Error: ref_sim_x86 not found"
    exit 1
fi

if [ ! -f "./src/sim" ]; then
    echo "Error: src/sim not found"
    exit 1
fi

# Function to run commands for a simulator and capture output
run_simulator() {
    local sim_type=$1
    local input_file=$2
    local output_file=$3
    
    # Create a command file
    local cmd_file=$(mktemp)
    echo "rdump" > $cmd_file
    echo "run 1" >> $cmd_file
    
    # Keep adding "run 1" commands until halted
    for i in {1..1000}; do
        echo "run 1" >> $cmd_file
    done
    echo "quit" >> $cmd_file
    
    # Start simulator with the command file
    if [ "$sim_type" = "ref" ]; then
        cat $cmd_file | ./ref_sim_x86 "$input_file" > "$output_file" 2>&1
    else
        cat $cmd_file | ./src/sim "$input_file" > "$output_file" 2>&1
    fi
    
    # Clean up
    rm -f $cmd_file
    
    # Check if the simulator ran successfully (look for rdump output)
    if ! grep -q "X0" "$output_file"; then
        echo "Error: Simulator failed to run correctly"
        return 1
    fi
    
    return 0
}

# Process each .x file in the directory
for x_file in "$DIR"/*.x; do
    if [ ! -f "$x_file" ]; then
        continue
    fi
    
    echo "Processing $x_file..."
    
    # Create temporary files for output
    ref_output="ref_output_$$.txt"
    sim_output="sim_output_$$.txt"
    
    # Run reference simulator
    echo "Running reference simulator..."
    if ! run_simulator "ref" "$x_file" "$ref_output"; then
        echo "Failed to run reference simulator for $x_file"
        rm -f "$ref_output" "$sim_output"
        continue
    fi
    
    # Run our simulator
    echo "Running our simulator..."
    if ! run_simulator "sim" "$x_file" "$sim_output"; then
        echo "Failed to run our simulator for $x_file"
        rm -f "$ref_output" "$sim_output"
        continue
    fi
    
    # Filter and normalize outputs for comparison
    cat "$ref_output" | grep -E "X[0-9]|PC|flags" | sort > "ref_filtered_$$.txt"
    cat "$sim_output" | grep -E "X[0-9]|PC|flags" | sort > "sim_filtered_$$.txt"
    
    # Compare filtered outputs
    if diff "ref_filtered_$$.txt" "sim_filtered_$$.txt" > /dev/null; then
        echo "✓ Outputs match for $x_file"
    else
        echo "✗ Differences found in $x_file"
        echo "Differences:"
        diff "ref_filtered_$$.txt" "sim_filtered_$$.txt"
        # Clean up temporary files
        rm -f "$ref_output" "$sim_output" "ref_filtered_$$.txt" "sim_filtered_$$.txt"
        exit 1
    fi
    
    # Clean up temporary files
    rm -f "$ref_output" "$sim_output" "ref_filtered_$$.txt" "sim_filtered_$$.txt"
done

echo "All files processed successfully!"