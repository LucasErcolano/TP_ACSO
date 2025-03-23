#!/usr/bin/env python3

import sys
import os
import subprocess
import threading
import time
import signal

def usage():
    print(f"Usage: {sys.argv[0]} <input_file.x>")
    sys.exit(1)

def check_executable(path):
    if not os.path.isfile(path) or not os.access(path, os.X_OK):
        print(f"Error: {path} not found or not executable")
        sys.exit(1)

def read_output(process, name, lock):
    """Read output from a process and print it with a header"""
    while True:
        output = process.stdout.readline()
        if output == '' and process.poll() is not None:
            break
        if output:
            with lock:
                print(f"\n=== {name} OUTPUT ===")
                print(output.strip())
                print("=====================")
                print("> ", end="", flush=True)  # Re-print prompt

def main():
    if len(sys.argv) < 2:
        usage()
    
    input_file = sys.argv[1]
    sim_path = "./sim"
    ref_sim_path = "./ref_sim"
    
    check_executable(sim_path)
    check_executable(ref_sim_path)
    
    # Start both simulators
    sim = subprocess.Popen([sim_path, input_file], 
                          stdin=subprocess.PIPE, 
                          stdout=subprocess.PIPE, 
                          stderr=subprocess.STDOUT,
                          text=True, 
                          bufsize=1)
    
    ref_sim = subprocess.Popen([ref_sim_path, input_file], 
                              stdin=subprocess.PIPE, 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.STDOUT,
                              text=True, 
                              bufsize=1)
    
    # Print initial output from both simulators
    print("\n=== YOUR SIMULATOR INITIAL OUTPUT ===")
    time.sleep(0.5)  # Give some time for initial output
    while sim.stdout.readable() and not sim.stdout.closed:
        line = sim.stdout.readline()
        if not line:
            break
        print(line.strip())
        if "ARM-SIM>" in line:
            break
    
    print("\n=== REFERENCE SIMULATOR INITIAL OUTPUT ===")
    while ref_sim.stdout.readable() and not ref_sim.stdout.closed:
        line = ref_sim.stdout.readline()
        if not line:
            break
        print(line.strip())
        if "ARM-SIM>" in line:
            break
    
    # Setup output readers
    output_lock = threading.Lock()
    sim_thread = threading.Thread(target=read_output, args=(sim, "YOUR SIMULATOR", output_lock))
    ref_sim_thread = threading.Thread(target=read_output, args=(ref_sim, "REFERENCE SIMULATOR", output_lock))
    
    sim_thread.daemon = True
    ref_sim_thread.daemon = True
    
    sim_thread.start()
    ref_sim_thread.start()
    
    print("\n=== SIMULATOR COMPARISON TOOL ===")
    print("Enter commands below. Both simulators will receive the same input.")
    print("Press Ctrl+C to exit.")
    print("===============================")
    
    try:
        while True:
            cmd = input("> ")
            if cmd.strip().lower() == "exit" or cmd.strip().lower() == "quit":
                break
                
            # Send command to both simulators
            sim.stdin.write(cmd + "\n")
            sim.stdin.flush()
            
            ref_sim.stdin.write(cmd + "\n")
            ref_sim.stdin.flush()
            
            # Give some time for the commands to process
            time.sleep(0.2)
    
    except KeyboardInterrupt:
        print("\nExiting...")
    
    finally:
        # Clean up
        sim.terminate()
        ref_sim.terminate()
        sim.wait(timeout=1)
        ref_sim.wait(timeout=1)

if __name__ == "__main__":
    main()