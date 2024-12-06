#!/bin/sh

# Configuration file path
CONFIG_FILE="src/filesystem/filesystem.config"

# Extract the directory path from the MOUNT_DIR line
MOUNT_DIR=$(grep '^MOUNT_DIR=' "$CONFIG_FILE" | cut -d'=' -f2 | sed 's?/*$??')

# Check if the directory was found
if [ -z "$MOUNT_DIR" ]; then
  printf "$CONFIG_FILE: el archivo de configuracion no contiene la clave MOUNT_DIR\n" >&2
  exit 1
fi
printf "MOUNT_DIR=$MOUNT_DIR\n"

set -x

# Remove specified files
rm -f "$MOUNT_DIR/bloques.dat"
rm -f "$MOUNT_DIR/bitmap.dat"
rm -f "$MOUNT_DIR/files/"*.dmp
rmdir "$MOUNT_DIR/files" 2>/dev/null || true
rmdir "$MOUNT_DIR" 2>/dev/null || true

{ set +x ; } 2>/dev/null

# Confirmation message
printf "$MOUNT_DIR: Archivos eliminados\n"