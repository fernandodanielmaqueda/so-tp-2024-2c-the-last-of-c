#!/bin/bash

# Verificar si se proporcionó un parámetro
if [ -z "$1" ]; then
  echo "Uso: $0 \"kernel=valor1,memoria=valor2,cpu=valor3,...\""
  exit 1
fi

# Cadena de entrada
input_string="$1"

# Separar por comas para obtener pares clave=valor
IFS=',' read -r -a pairs <<< "$input_string"

# Crear variables globales basadas en los pares clave=valor
for pair in "${pairs[@]}"; do
  IFS='=' read -r key value <<< "$pair"
  declare -g "$key"="$value"
  printf "Clave: %s, Valor: %s\n" "$key" "$value"
done

# Ruta base
BASE_DIR="/home/utnso/tp-2024-2c-so/configs"

# Navegar al directorio base
cd "$BASE_DIR" || { echo "No se pudo acceder a $BASE_DIR"; exit 1; }

# Iterar sobre cada subdirectorio
for dir in */; do
  printf "\nProcesando directorio: %s\n" "$dir"
  
  for file in "$dir"*.config; do
    printf "Revisando archivo: %s\n" "$file"
    
    # Reemplazar valores según las variables globales
    [[ -n "${kernel+x}" ]] && sed -i -E "s/(IP_KERNEL=).*/\1${kernel}/" "$file"
    [[ -n "${memoria+x}" ]] && sed -i -E "s/(IP_MEMORIA=).*/\1${memoria}/" "$file"
    [[ -n "${cpu+x}" ]] && sed -i -E "s/(IP_CPU=).*/\1${cpu}/" "$file"
    [[ -n "${filesystem+x}" ]] && sed -i -E "s/(IP_FILESYSTEM=).*/\1${filesystem}/" "$file"

    # Mostrar líneas modificadas
    grep -E "IP_(KERNEL|MEMORIA|CPU|FILESYSTEM)=" "$file"
  done
done
