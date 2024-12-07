#!/bin/bash


select_log_level() {
    PS3="Seleccione el nivel de log para actualizar en todas las configs: "
    options=("Salir" "ERROR" "WARNING" "INFO" "DEBUG" "TRACE")
    select opt in "${options[@]}"; do
        case $REPLY in
            1)
                printf "Saliendo...\n"
                exit 0
                ;;
            2|3|4|5|6)
                update_log_level "$opt"
                break
                ;;
            *)
                echo "Opcion invalida. Intentar nuevamente."
                ;;
        esac
    done
}

update_log_level() {
    new_log_level=$1
    modules=("kernel" "cpu" "memoria" "filesystem")

    for module in "${modules[@]}"; do
        config_file="src/$module/${module}.config"
        if [ -f "$config_file" ]; then
            if grep -q "^LOG_LEVEL=" "$config_file"; then
                # Update del log level
                sed -i -E "s/^LOG_LEVEL=.*/LOG_LEVEL=$new_log_level/" "$config_file"
                printf "Sobreescrito LOG_LEVEL en $config_file\n"
            fi
        else
            printf "Archivo de configuracion no encontrado para el modulo $module\n"
        fi
    done

    printf "\nLOG_LEVEL actualizado a $new_log_level en todas las configs.\n"
}

# Ejecutar la funcion "Main"
select_log_level
