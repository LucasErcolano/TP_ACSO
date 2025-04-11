#!/usr/bin/env python3
import subprocess
import sys
import os
import time
import itertools # No estrictamente necesario ahora, pero lo dejamos por si acaso
from multiprocessing import Pool, cpu_count, current_process # Para paralelismo

# --- Configuración ---
bomb_executable = './bomb'

# --- Respuestas Fijas Fases 1-4 ---
phase1_answer = "He conocido, aunque tarde, sin haberme arrepentido, que es pecado cometido el decir ciertas verdades"
phase2_answer = "35110 -1"  # Espacio como separador, según tu respuesta
phase3_answer = "joropear 8 abrete_sesamo"
phase4_answer = "NGMDHH"
# --- ------------- ---

# --- Rango para Fase 5 ---
# Probar números del 1 al 1000 (inclusive)
phase5_range = range(1, 1001)
# --- ------------- ---

# --- Configuración de Paralelismo ---
try:
    NUM_WORKERS = max(1, cpu_count() - 1)
except NotImplementedError:
    NUM_WORKERS = 1
# NUM_WORKERS = cpu_count() # Descomentar para usar todos los cores
CHUNK_SIZE = 50 # Ajustado, ya que las tareas podrían ser un poco más largas
COMMUNICATE_TIMEOUT = 5 # Aumentado ligeramente por más fases
# --- ------------- ---

# --- Función Worker (Prueba un número para Fase 5) ---
def run_single_attempt(p5_num):
    """
    Función ejecutada por cada worker del pool.
    Recibe el número a probar para la Fase 5 (p5_num).
    Envía las respuestas de Fases 1-4 + el número de Fase 5.
    Retorna (True, p5_num) en éxito (sin BOOM), False en BOOM, None en error/timeout.
    """
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

        # Construir la entrada completa para Fases 1-5
        phase1_input = phase1_answer + '\n'
        phase2_input = phase2_answer + '\n'
        phase3_input = phase3_answer + '\n'
        phase4_input = phase4_answer + '\n'
        phase5_input = f"{p5_num}\n" # El número actual a probar

        full_input = phase1_input + phase2_input + phase3_input + phase4_input + phase5_input

        try:
            # Enviar todo, esperar resultado
            stdout_data, stderr_data = process.communicate(input=full_input, timeout=COMMUNICATE_TIMEOUT)

        except subprocess.TimeoutExpired:
            # print(f"Worker {current_process().pid}: Timeout para Fase 5 = {p5_num}") # Debug
            process.kill()
            try: process.communicate(timeout=0.5)
            except: pass
            return None # Indica Timeout

        except Exception as e:
            # print(f"Worker {current_process().pid}: Error communicate() para Fase 5 = {p5_num}: {e}") # Debug
            if process.poll() is None: process.kill()
            try: process.communicate(timeout=0.5)
            except: pass
            return None # Indica otro error

        # Evaluar resultado: Éxito si NO explotó después de recibir las 5 entradas
        # No buscamos un mensaje específico de Fase 5, solo la ausencia de BOOM.
        # (Asume que las fases anteriores son correctas y BOOM indicaría fallo en Fase 5)
        if "BOOM!!!" not in stdout_data:
             # Podríamos verificar si la salida contiene los mensajes de éxito de fases anteriores,
             # pero la ausencia de BOOM es generalmente suficiente aquí.
            return (True, p5_num) # Devolver éxito y el número probado
        else:
            # print(f"Worker {current_process().pid}: BOOM para Fase 5 = {p5_num}") # Debug
            # print(stdout_data) # Debug
            return False # Fallo (BOOM)

    except FileNotFoundError:
        print(f"ERROR FATAL Worker {current_process().pid}: Ejecutable '{bomb_executable}' no encontrado.")
        return None
    except Exception as e:
        # print(f"Worker {current_process().pid}: Error Popen/Otro para Fase 5 = {p5_num}: {e}") # Debug
        return None
    finally:
        if process and process.poll() is None:
            process.kill()
            try: process.communicate(timeout=0.5)
            except: pass

# --- Ejecución Principal (Manejo del Pool para Fase 5) ---
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
    print(f"Iniciando búsqueda para Fase 5 (probando números del {phase5_range.start} al {phase5_range.stop - 1})...")
    start_time = time.time()

    # Rango de números a probar para Fase 5
    combinations = phase5_range
    total_combinations = len(combinations)
    print(f"Espacio de búsqueda total: {total_combinations:,} combinaciones.")
    if total_combinations == 0:
         print("Advertencia: Rango de Fase 5 está vacío.")
         sys.exit(0)

    solution_p5 = None # Variable para guardar la solución de Fase 5
    tested_count = 0
    error_count = 0

    # Crear y gestionar el pool de procesos
    pool = Pool(processes=NUM_WORKERS)
    try:
        # Usar imap_unordered para obtener resultados tan pronto como estén listos
        # El iterable ahora es solo el rango de números para Fase 5
        results_iterator = pool.imap_unordered(run_single_attempt, combinations, chunksize=CHUNK_SIZE)

        print(f"Buscando... Presiona Ctrl+C para detener.")
        for result in results_iterator:
            tested_count += 1

            # Imprimir progreso menos frecuentemente
            if tested_count % (CHUNK_SIZE * NUM_WORKERS * 2) == 0 or tested_count == 1:
                 elapsed = time.time() - start_time
                 rate = tested_count / elapsed if elapsed > 0 else 0
                 progress = f"{tested_count:,}/{total_combinations:,}"
                 print(f"  Probadas {progress} combinaciones... ({rate:.1f} comb/s)")

            if isinstance(result, tuple) and result[0] is True:
                # ¡Éxito! result es (True, p5_num)
                solution_p5 = result[1] # Guardar el número de Fase 5
                print(f"\n>>> ¡ÉXITO! Solución para Fase 5 encontrada: {solution_p5} <<<")
                print("Terminando workers restantes...")
                pool.terminate()
                break # Salir del bucle

            elif result is None:
                 error_count += 1
                 # Opcional: Imprimir advertencia si hay muchos errores
                 # if error_count % 20 == 0:
                 #      print(f"  Advertencia: {error_count} errores/timeouts encontrados...")

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

    if solution_p5 is not None:
        print(f"\nSolución Final Encontrada para Fase 5: {solution_p5}")
    else:
        print("\nNo se encontró la solución para la Fase 5 dentro del rango especificado.")
        if tested_count < total_combinations:
             print("(La búsqueda puede haberse detenido prematuramente).")
    print("-" * 40)