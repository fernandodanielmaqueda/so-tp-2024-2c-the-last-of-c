#!/bin/sh

printf "Eliminando archivos de log\n"

set -x

find . -type f -name '*.log' -exec rm -f {} +
# find . -type f -name '*.log' -exec rm -i {} +

{ set +x ; } 2>/dev/null

printf "Archivos de log eliminados\n"