#!/usr/bin/env python3
import subprocess
import sys
import os
import re
import tempfile
import difflib

def run_simulator(simulator_path, input_file):
    """
    Ejecuta un simulador ARM con el archivo de entrada especificado.
    Retorna la salida capturada del simulador.
    """
    # Crear un archivo temporal para la salida del simulador
    with tempfile.NamedTemporaryFile(mode='w+', delete=False) as temp_out:
        temp_out_path = temp_out.name
    
    try:
        # Preparar el comando para ejecutar el simulador
        command = [simulator_path, input_file]
        
        # Crear un proceso para el simulador
        process = subprocess.Popen(
            command, 
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Enviar comandos al simulador: 'go' para ejecutar y 'rdump' para mostrar registros
        output, error = process.communicate("go\nrdump\nquit\n")
        
        if process.returncode != 0:
            print(f"Error al ejecutar el simulador {simulator_path}:")
            print(error)
            return None
        
        return output
    
    except Exception as e:
        print(f"Error al ejecutar {simulator_path}: {str(e)}")
        return None
    finally:
        # Limpiar archivo temporal
        if os.path.exists(temp_out_path):
            os.unlink(temp_out_path)

def parse_simulator_output(output):
    """
    Parsea la salida del simulador para extraer el estado de los registros.
    Retorna un diccionario con el estado de los registros.
    """
    state = {}
    
    # Extraer el valor de PC
    pc_match = re.search(r'PC\s+:\s+(0x[0-9a-fA-F]+)', output)
    if pc_match:
        state['PC'] = pc_match.group(1)
    
    # Extraer los valores de los registros X0-X31
    reg_matches = re.findall(r'X(\d+):\s+(0x[0-9a-fA-F]+)', output)
    for reg_num, reg_val in reg_matches:
        state[f'X{reg_num}'] = reg_val
    
    # Extraer los valores de las flags
    flag_n_match = re.search(r'FLAG_N:\s+(\d+)', output)
    flag_z_match = re.search(r'FLAG_Z:\s+(\d+)', output)
    
    if flag_n_match:
        state['FLAG_N'] = flag_n_match.group(1)
    if flag_z_match:
        state['FLAG_Z'] = flag_z_match.group(1)
    
    return state

def compare_states(state1, state2):
    """
    Compara los estados de dos simuladores y retorna las diferencias.
    """
    differences = []
    
    # Obtener todas las claves de ambos estados
    all_keys = set(state1.keys()) | set(state2.keys())
    
    for key in sorted(all_keys):
        val1 = state1.get(key, "No presente")
        val2 = state2.get(key, "No presente")
        
        if val1 != val2:
            differences.append(f"{key}: {val1} vs {val2}")
    
    return differences

def main():
    # Verificar argumentos
    if len(sys.argv) != 4:
        print("Uso: python comparador.py <simulador1> <simulador2> <archivo.x>")
        sys.exit(1)
    
    simulator1_path = sys.argv[1]
    simulator2_path = sys.argv[2]
    input_file = sys.argv[3]
    
    # Verificar que los archivos existen
    for file_path in [simulator1_path, simulator2_path, input_file]:
        if not os.path.exists(file_path):
            print(f"Error: El archivo {file_path} no existe.")
            sys.exit(1)
    
    # Ejecutar ambos simuladores
    print(f"Ejecutando {simulator1_path} con {input_file}...")
    output1 = run_simulator(simulator1_path, input_file)
    
    print(f"Ejecutando {simulator2_path} con {input_file}...")
    output2 = run_simulator(simulator2_path, input_file)
    
    if not output1 or not output2:
        print("Error: No se pudo obtener la salida de uno o ambos simuladores.")
        sys.exit(1)
    
    # Parsear las salidas
    state1 = parse_simulator_output(output1)
    state2 = parse_simulator_output(output2)
    
    # Mostrar salidas completas (opcional)
    print("\n==== Salida del Simulador 1 ====")
    print(output1)
    print("\n==== Salida del Simulador 2 ====")
    print(output2)
    
    # Comparar estados
    differences = compare_states(state1, state2)
    
    # Mostrar resultados
    print("\n==== Comparación de Estados ====")
    if differences:
        print(f"Se encontraron {len(differences)} diferencias:")
        for diff in differences:
            print(f"  - {diff}")
    else:
        print("¡Los resultados son idénticos!")
    
    # Comparación detallada usando difflib
    print("\n==== Diferencias Detalladas ====")
    output1_lines = output1.splitlines()
    output2_lines = output2.splitlines()
    
    diff = difflib.unified_diff(
        output1_lines, 
        output2_lines, 
        fromfile='Simulador 1', 
        tofile='Simulador 2',
        lineterm=''
    )
    
    for line in diff:
        print(line)

if __name__ == "__main__":
    main()