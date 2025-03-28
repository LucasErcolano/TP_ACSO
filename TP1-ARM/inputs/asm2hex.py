#!/usr/bin/env python3

import os
import argparse
import subprocess

parser = argparse.ArgumentParser(description="Convert ARM assembly files to hex bytecode")
parser.add_argument("fasm", metavar="input.s", nargs="+", help="One or more ARM assembly files (ASCII)")
args = parser.parse_args()

curpwd = os.getcwd()

# Path to ARM assembler and objdump
armas = os.path.join(os.path.dirname(__file__), '..', 'aarch64-linux-android-4.9', 'bin', 'aarch64-linux-android-as')
armobjdump = os.path.join(os.path.dirname(__file__), '..', 'aarch64-linux-android-4.9', 'bin', 'aarch64-linux-android-objdump')

# Process each input file
for fasm in args.fasm:
    # Temporary output file
    ftmp = f"tmp_{os.path.basename(fasm)}.out"
    
    # Hex output file (replace .s with .x)
    fhex = os.path.splitext(fasm)[0] + ".x"

    try:
        # Assemble the file
        subprocess.run([armas, fasm, "-o", ftmp], check=True)

        # Dump object file to hex
        with open(fhex, "w", encoding="utf-8") as fstdout:
            subprocess.run([armobjdump, "-d", ftmp], stdout=fstdout, check=True)

        # Extract only bytecode lines
        bytecode_lines = []
        with open(fhex, encoding="utf-8") as f:
            for line in f:
                parts = line.split('\t')  # Split on tab characters
                if len(parts) > 1 and parts[0].strip().endswith(":"):
                    bytecode = parts[1].strip()
                    bytecode_lines.append(bytecode)

        # Write only bytecode to the hex file
        with open(fhex, "w", encoding="utf-8") as f:
            f.write("\n".join(bytecode_lines))

        print(f"Converted {fasm} to {fhex}")

    except subprocess.CalledProcessError as e:
        print(f"Error processing {fasm}: {e}")
    
    finally:
        # Clean up temporary file
        if os.path.exists(ftmp):
            os.unlink(ftmp)