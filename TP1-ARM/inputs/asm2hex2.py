#!/usr/bin/env python2

import os, argparse, subprocess

# parse arguments
parser = argparse.ArgumentParser()
parser.add_argument("input_dir", help="directorio con archivos de ensamblador ARM (.s)")
parser.add_argument("output_dir", help="directorio para archivos de salida (.x)")
args = parser.parse_args()

# Asegurar que el directorio de salida exista
if not os.path.exists(args.output_dir):
    os.makedirs(args.output_dir)

# Ruta a las herramientas ARM
armas = os.path.join(os.path.dirname(__file__),
                     '..', 'aarch64-linux-android-4.9', 'bin',
                     'aarch64-linux-android-as')
armobjdump = os.path.join(os.path.dirname(__file__),
                          '..', 'aarch64-linux-android-4.9', 'bin',
                          'aarch64-linux-android-objdump')

# Procesar cada archivo .s en el directorio de entrada
for filename in os.listdir(args.input_dir):
    if filename.endswith('.s'):
        # Rutas completas
        fasm = os.path.join(args.input_dir, filename)
        fbase = os.path.splitext(filename)[0]
        fhex = os.path.join(args.output_dir, fbase + '.x')
        ftmp = os.path.join(args.output_dir, "tmp_" + fbase + ".out")
        
        # Ejecutar el ensamblador ARM
        cmd = [armas, fasm, "-o", ftmp]
        subprocess.call(cmd)
        
        # Ejecutar objdump para obtener el código hexadecimal
        cmd = [armobjdump, "-d", ftmp]
        fstdout = open(fhex, "w")
        subprocess.call(cmd, stdout=fstdout)
        fstdout.close()
        
        # Procesar el archivo de salida
        lines = open(fhex).readlines()
        data = ''
        for x in lines:
            if '\t' in x:
                data = data + x
        open(fhex, 'w').write(data)
        
        lines = open(fhex).readlines()
        lines = map(lambda x: x.split('\t')[1], lines)
        data = str.join('\n', lines)
        open(fhex, 'w').write(data)
        
        # Eliminar archivo temporal
        cmd = ["rm", ftmp]
        cmd = str.join(' ', cmd)
        subprocess.call(cmd, shell=True)

print("Procesamiento completado. Los archivos de salida están en: " + args.output_dir)