#!/usr/bin/env python3
import subprocess
import sys
import os
import select

def run_interactive_simulation(sim1_path, sim2_path, input_file):
    # Iniciar ambos simuladores
    sim1 = subprocess.Popen(
        [sim1_path, input_file],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )
    
    sim2 = subprocess.Popen(
        [sim2_path, input_file],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    # Buffer para capturar salidas
    outputs = {sim1: {"buffer": "", "label": "SIM1"}, sim2: {"buffer": "", "label": "SIM2"}}

    while True:
        # Leer entrada del usuario
        print("\nARM-SIM> ", end="", flush=True)
        cmd = sys.stdin.readline().strip()
        if cmd.lower() in ["quit", "q"]:
            break

        # Enviar comando a ambos simuladores
        sim1.stdin.write(cmd + "\n")
        sim2.stdin.write(cmd + "\n")
        sim1.stdin.flush()
        sim2.stdin.flush()

        # Leer salidas usando select para evitar bloqueos
        while True:
            ready, _, _ = select.select([sim1.stdout, sim2.stdout], [], [], 0.1)
            if not ready:
                break
            
            for proc in ready:
                line = proc.stdout.readline()
                if not line:
                    continue
                
                outputs[proc]["buffer"] += line
                
                # Mostrar salida cuando se detecte un prompt o fin de comando
                if "ARM-SIM>" in line or "Bye." in line:
                    print(f"\n==== {outputs[proc]['label']} ====")
                    print(outputs[proc]["buffer"].strip())
                    outputs[proc]["buffer"] = ""

    # Finalizar procesos
    sim1.terminate()
    sim2.terminate()

def main():
    if len(sys.argv) != 4:
        print("Uso: python simulator.py <sim1> <sim2> <program.x>")
        sys.exit(1)

    sim1_path = sys.argv[1]
    sim2_path = sys.argv[2]
    input_file = sys.argv[3]

    run_interactive_simulation(sim1_path, sim2_path, input_file)

if __name__ == "__main__":
    main()