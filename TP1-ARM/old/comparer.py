#!/usr/bin/env python3

import sys
import os
import subprocess
import time

def usage():
    print(f"Usage: {sys.argv[0]} <input_file.x>")
    sys.exit(1)

def check_executable(path):
    if not os.path.isfile(path) or not os.access(path, os.X_OK):
        print(f"Error: {path} not found or not executable")
        sys.exit(1)
    return path

def find_executable(name):
    """Find the executable in current or neighboring directories"""
    paths = [
        f"./{name}",
        f"./src/{name}",
        f"../{name}",
        f"../src/{name}"
    ]
    
    # Try Intel version for ref_sim if on Linux
    if name == "ref_sim":
        paths.append("./ref_sim_x86")
        paths.append("../ref_sim_x86")
    
    for path in paths:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path
    
    print(f"Error: Could not find executable '{name}'")
    sys.exit(1)

def main():
    if len(sys.argv) < 2:
        usage()
    
    input_file = sys.argv[1]
    
    # Find executables
    sim_path = find_executable("sim")
    ref_sim_path = find_executable("ref_sim")
    
    print(f"Using simulators:\n  Your simulator: {sim_path}\n  Reference simulator: {ref_sim_path}")
    print(f"Input file: {input_file}")
    
    print("\n=== SIMULATOR COMPARISON TOOL ===")
    print("Enter commands below. Both simulators will receive the same input.")
    print("Commands will be sent to both simulators and their outputs will be displayed.")
    print("Type 'exit' or 'quit' to exit the tool.")
    print("===============================")
    
    try:
        while True:
            cmd = input("> ")
            if cmd.strip().lower() in ["exit", "quit"]:
                break
            
            # Run your simulator with the command
            print("\n=== YOUR SIMULATOR OUTPUT ===")
            try:
                sim_proc = subprocess.run(
                    f"echo '{cmd}' | {sim_path} {input_file}",
                    shell=True,
                    text=True,
                    timeout=5,
                    capture_output=True
                )
                print(sim_proc.stdout)
                if sim_proc.stderr:
                    print("STDERR:", sim_proc.stderr)
            except subprocess.TimeoutExpired:
                print("Your simulator timed out after 5 seconds")
            
            # Run reference simulator with the command
            print("\n=== REFERENCE SIMULATOR OUTPUT ===")
            try:
                ref_proc = subprocess.run(
                    f"echo '{cmd}' | {ref_sim_path} {input_file}",
                    shell=True,
                    text=True,
                    timeout=5,
                    capture_output=True
                )
                print(ref_proc.stdout)
                if ref_proc.stderr:
                    print("STDERR:", ref_proc.stderr)
            except subprocess.TimeoutExpired:
                print("Reference simulator timed out after 5 seconds")
            
            print("===============================")
    
    except KeyboardInterrupt:
        print("\nExiting...")

if __name__ == "__main__":
    main()