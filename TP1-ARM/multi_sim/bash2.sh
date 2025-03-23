#!/bin/bash

# Check if an input file was provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <input_file.x>"
    exit 1
fi

INPUT_FILE=$1

# Kill any existing session with the same name
SESSION_NAME="sim_compare"
tmux kill-session -t "$SESSION_NAME" 2>/dev/null

# Find simulators
find_sim() {
    for path in "./$1" "./src/$1" "../$1" "../src/$1"; do
        if [ -x "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    
    # Try Intel version for ref_sim
    if [ "$1" = "ref_sim" ]; then
        for path in "./ref_sim_x86" "../ref_sim_x86"; do
            if [ -x "$path" ]; then
                echo "$path"
                return 0
            fi
        done
    fi
    
    echo "Error: Could not find executable '$1'" >&2
    exit 1
}

SIM=$(find_sim "sim")
REF_SIM=$(find_sim "ref_sim")

echo "Using simulators:"
echo "  Your simulator: $SIM"
echo "  Reference simulator: $REF_SIM"
echo "Input file: $INPUT_FILE"

# Check if tmux is installed
if ! command -v tmux &> /dev/null; then
    echo "Error: This script requires tmux. Please install it and try again."
    exit 1
fi

# Create a new tmux session
tmux new-session -d -s "$SESSION_NAME"

# Split the window horizontally
tmux split-window -h -t "$SESSION_NAME"

# Add a title to each pane and start simulators
tmux send-keys -t "${SESSION_NAME}.0" "clear && echo '=== YOUR SIMULATOR ===' && echo && $SIM $INPUT_FILE" C-m
tmux send-keys -t "${SESSION_NAME}.1" "clear && echo '=== REFERENCE SIMULATOR ===' && echo && $REF_SIM $INPUT_FILE" C-m

# Wait a moment to let the simulators initialize
sleep 1

# Set synchronized input
tmux set-window-option -t "$SESSION_NAME" synchronize-panes on

# Attach to the session
echo "Starting simulators in tmux session. Both panes will receive the same input."
echo "Press Ctrl+B followed by D to detach from the session without closing it."
echo "Press Ctrl+B followed by X to close the current pane."
echo "==============================================================="
tmux attach-session -t "$SESSION_NAME"

# Clean up on exit
tmux kill-session -t "$SESSION_NAME" 2>/dev/null