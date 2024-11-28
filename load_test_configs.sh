#!/bin/bash

# Funcion para desplegar menu y manejar input
select_config_folder() {
    PS3="Elija uno de los siguientes tests de config: " # NOTA: Si metemos mas select, habria que ver de actualizar el PS3
    options=($(ls -d configs/*/))
    select opt in "Salir" "${options[@]}"; do
        case $opt in
            "Salir")
                echo "Saliendo..."
                exit 0
                ;;
            *)
                if [[ -d $opt ]]; then
                    echo "Elegiste $opt"
                    load_configs "$opt"
                    break
                else
                    echo "Opcion invalida. Reintentar de nuevo."
                fi
                ;;
        esac
    done
}

# Funcion para cargar el config seleecionado
load_configs() {
    config_folder=$1
    cp "${config_folder}cpu.config" "src/cpu/cpu.config"
    cp "${config_folder}filesystem.config" "src/filesystem/filesystem.config"
    cp "${config_folder}kernel.config" "src/kernel/kernel.config"
    cp "${config_folder}memoria.config" "src/memoria/memoria.config"
    echo "Se cargaron las configs de $config_folder"
}

# Ejecutar la funcion "Main"
select_config_folder
