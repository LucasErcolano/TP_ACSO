#!/usr/bin/env python3
import subprocess
import sys
import os
import time
import itertools # Para generar combinaciones
from multiprocessing import Pool, cpu_count, current_process # Para paralelismo

# --- Configuración ---
bomb_executable = './bomb'
phase1_password = "He conocido, aunque tarde, sin haberme arrepentido, que es pecado cometido el decir ciertas verdades"

# --- Rangos (¡¡¡CRÍTICO PARA EL RENDIMIENTO!!!) ---
# --- ¡¡¡AJUSTA ESTOS RANGOS BASADO EN TU ANÁLISIS DE phase_2!!! ---
# ¡Rangos más pequeños = Búsqueda MUCHO más rápida!
n1_range = range(-500000, 500000)  # Ejemplo: ¡AJUSTAR!
n2_range = [-1] # Ejemplo: ¡AJUSTAR!
# --- ------------- ---

# --- Configuración de Paralelismo ---
try:
    # Usar todos los cores menos uno (o ajustar según preferencia)
    NUM_WORKERS = max(1, cpu_count() - 1)
except NotImplementedError:
    NUM_WORKERS = 1
# NUM_WORKERS = cpu_count() # Descomentar para usar todos los cores
CHUNK_SIZE = 100 # Tamaño del lote de tareas para cada worker
COMMUNICATE_TIMEOUT = 3 # Timeout para cada intento individual en segundos
# --- ------------- ---

# --- Función Worker (Ejecuta un intento, salida mínima) ---
def run_single_attempt(params):
    """
    Función ejecutada por cada worker del pool.
    Recibe una tupla (n1, n2).
    Retorna (True, n1, n2) en éxito, False en BOOM/Fallo Fase1, None en error/timeout.
    """
    p2_n1, p2_n2 = params # Desempaqueta solo n1, n2

    process = None
    try:
        process = subprocess.Popen(
            [bomb_executable],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            universal_newlines=True
        )

        # Formato de entrada Fase 2: "n1, n2"
        p2_input_str = f"{p2_n1}, {p2_n2}"
        full_input = phase1_password + '\n' + p2_input_str + '\n'

        try:
            stdout_data, stderr_data = process.communicate(input=full_input, timeout=COMMUNICATE_TIMEOUT)

        except subprocess.TimeoutExpired:
            process.kill()
            try: process.communicate(timeout=0.5)
            except: pass
            return None # Indica Timeout

        except Exception as e:
            if process.poll() is None: process.kill()
            try: process.communicate(timeout=0.5)
            except: pass
            return None # Indica otro error

        # Evaluar resultado
        phase1_ok = "Etapa 1 desactivada" in stdout_data
        boom = "BOOM!!!" in stdout_data

        # Éxito si Fase 1 OK y no BOOM
        if phase1_ok and not boom:
            return (True, p2_n1, p2_n2) # Devolver éxito y combinación n1, n2
        else:
            return False # Fallo

    except FileNotFoundError:
        print(f"ERROR FATAL Worker {current_process().pid}: Ejecutable '{bomb_executable}' no encontrado.")
        return None
    except Exception as e:
        return None
    finally:
        if process and process.poll() is None:
            process.kill()
            try: process.communicate(timeout=0.5)
            except: pass

# --- Ejecución Principal (Manejo del Pool) ---
if __name__ == "__main__":
    # --- Verificaciones iniciales ---
    if not os.path.isfile(bomb_executable):
        print(f"Error: El ejecutable '{bomb_executable}' no se encontró.")
        sys.exit(1)
    if not os.access(bomb_executable, os.X_OK):
        print(f"Error: El ejecutable '{bomb_executable}' no es ejecutable. Intenta con 'chmod +x {bomb_executable}'.")
        sys.exit(1)

    # Fecha y hora actuales
    current_date = time.strftime("%Y-%m-%d %H:%M:%S %Z")
    print(f"Fecha y hora actuales: {current_date}")

    print(f"Optimizando con {NUM_WORKERS} workers (cores: {cpu_count()})")
    print(f"Iniciando búsqueda para Fase 2 (n1, n2) con formato 'n1, n2'...")
    start_time = time.time()

    # Generar combinaciones (n1, n2) como un iterador
    combinations = itertools.product(n1_range, n2_range)
    try:
        total_combinations = len(n1_range) * len(n2_range)
        print(f"Espacio de búsqueda total: {total_combinations:,} combinaciones.")
        if total_combinations == 0:
             print("Advertencia: Rangos definidos resultan en 0 combinaciones.")
             sys.exit(0)
    except OverflowError:
        total_combinations = float('inf')
        print("Advertencia: Espacio de búsqueda demasiado grande para calcular el total exacto.")

    solution_found = None
    tested_count = 0
    error_count = 0

    # Crear y gestionar el pool de procesos
    pool = Pool(processes=NUM_WORKERS)
    try:
        results_iterator = pool.imap_unordered(run_single_attempt, combinations, chunksize=CHUNK_SIZE)

        print(f"Buscando... Presiona Ctrl+C para detener.")
        for result in results_iterator:
            tested_count += 1

            if tested_count % (CHUNK_SIZE * NUM_WORKERS * 5) == 0 or tested_count == 1:
                 elapsed = time.time() - start_time
                 rate = tested_count / elapsed if elapsed > 0 else 0
                 progress = f"{tested_count:,}"
                 if total_combinations != float('inf'):
                     progress += f"/{total_combinations:,}"
                 print(f"  Probadas {progress} combinaciones... ({rate:.1f} comb/s)")

            if isinstance(result, tuple) and result[0] is True:
                # ¡Éxito! result es (True, n1, n2)
                solution_found = result[1:] # Guardar (n1, n2)
                print(f"\n>>> ¡ÉXITO! Combinación encontrada: n1={solution_found[0]}, n2={solution_found[1]} <<<")
                print("Terminando workers restantes...")
                pool.terminate()
                break

            elif result is None:
                 error_count += 1
                 if error_count % 50 == 0:
                      print(f"  Advertencia: {error_count} errores/timeouts encontrados...")

    except KeyboardInterrupt:
        print("\nInterrupción por teclado detectada. Terminando workers...")
        pool.terminate()
    except Exception as e:
        print(f"\nOcurrió un error inesperado durante el procesamiento del pool: {e}")
        pool.terminate()
    finally:
        pool.close()
        pool.join()

    # --- Reporte Final ---
    end_time = time.time()
    total_time = end_time - start_time
    final_rate = tested_count / total_time if total_time > 0 else 0

    print("-" * 40)
    print(f"Búsqueda completada en {total_time:.2f} segundos.")
    print(f"Total de combinaciones probadas: {tested_count:,}")
    if final_rate > 0:
         print(f"Velocidad promedio: {final_rate:.1f} combinaciones/segundo.")
    if error_count > 0:
        print(f"Número de errores/timeouts: {error_count}")

    if solution_found:
        print(f"\nSolución Final Encontrada: n1={solution_found[0]}, n2={solution_found[1]}")
    else:
        print("\nNo se encontró la solución para la Fase 2 dentro de los rangos especificados.")
        if not total_combinations or (total_combinations != float('inf') and tested_count < total_combinations) :
            print("(La búsqueda puede haberse detenido prematuramente o los rangos eran incorrectos).")
        print("\n¡RECUERDA! La optimización más importante es reducir los RANGOS")
        print("analizando la lógica de 'phase_2' en el código fuente o ensamblador.")
    print("-" * 40)