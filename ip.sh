#!/bin/bash

# Function to display the select menu and handle input
select_module_ip() {
    while true; do
        # Buscamos la primer carpeta de Test adentro de Configs
        first_folder=$(ls -d configs/*/ | head -n 1)
        folder_name=$(basename "$first_folder")

        printf "\n+-----------------------------------+\nMostrando las IPs encontradas en la primer carpeta de Test dentro de Configs. Nombre Carpeta: $folder_name \n\n"

        # Texto default para cada IP
        kernel_ip="IP NOT FOUND IN CONFIGS"
        cpu_ip="IP NOT FOUND IN CONFIGS"
        filesystem_ip="IP NOT FOUND IN CONFIGS"
        memoria_ip="IP NOT FOUND IN CONFIGS"

        # Guardamos la IP para cada Modulo, checkeando en todas las configs de la primer carpeta y quedandonos con el ultimo match para ese Modulo
        for config_file in "${first_folder}"*.config; do
            if grep -q "^IP_KERNEL=" "$config_file"; then
                kernel_ip=$(grep -E "^IP_KERNEL=" "$config_file" | cut -d'=' -f2 | tr -d '\r')
            fi
            if grep -q "^IP_CPU=" "$config_file"; then
                cpu_ip=$(grep -E "^IP_CPU=" "$config_file" | cut -d'=' -f2 | tr -d '\r')
            fi
            if grep -q "^IP_FILESYSTEM=" "$config_file"; then
                filesystem_ip=$(grep -E "^IP_FILESYSTEM=" "$config_file" | cut -d'=' -f2 | tr -d '\r')
            fi
            if grep -q "^IP_MEMORIA=" "$config_file"; then
                memoria_ip=$(grep -E "^IP_MEMORIA=" "$config_file" | cut -d'=' -f2 | tr -d '\r')
            fi
        done

        PS3="Elija uno de los siguientes módulos para actualizar la IP (en todos los configs y Tests): "
        options=("Salir" "Aplicar las IPs mostradas en pantalla a todos los configs por las dudas." "* KERNEL [$kernel_ip]" "* CPU [$cpu_ip]" "* FILESYSTEM [$filesystem_ip]" "* MEMORIA [$memoria_ip]")
        select opt in "${options[@]}"; do
            case $REPLY in
                1)
                    echo "Saliendo..."
                    exit 0
                    ;;
                2)
                    apply_all_ips "$kernel_ip" "$cpu_ip" "$filesystem_ip" "$memoria_ip"
                    break
                    ;;
                3)
                    update_ip "KERNEL" "IP_KERNEL"
                    break
                    ;;
                4)
                    update_ip "CPU" "IP_CPU"
                    break
                    ;;
                5)
                    update_ip "FILESYSTEM" "IP_FILESYSTEM"
                    break
                    ;;
                6)
                    update_ip "MEMORIA" "IP_MEMORIA"
                    break
                    ;;
                *)
                    echo "Opcion invalida. Reintentar de nuevo."
                    ;;
            esac
        done
    done
}

# Funcion para actualizar la IP del modulo seleccionado en todas las configs de todos los Tests
# dentro de carpeta Configs
update_ip() {
    module_name=$1
    ip_key=$2

    read -p "$module_name IP: " new_ip

    for config_folder in configs/*/; do
        for config_file in "${config_folder}"*.config; do
            if grep -q "^$ip_key=" "$config_file"; then
                # Sacar los Windows line endings por las dudas
                sed -i 's/\r$//' "$config_file"
                # Actualizar la IP
                sed -i -E "s/^($ip_key=).*/\1$new_ip/" "$config_file"
                # Agregar los Windows line endings de nuevo
                sed -i 's/$/\r/' "$config_file"
                printf "Actualizado IP de $ip_key en $config_file\n"
            fi
        done
    done

    printf "\n$module_name IP updated to $new_ip in all config files.\n"
}

# Funcion para aplicar todas las IPs mostradas a todos los archivos de configuracion adentro de la carpeta Configs
apply_all_ips() {
    kernel_ip=$1
    cpu_ip=$2
    filesystem_ip=$3
    memoria_ip=$4

    for config_folder in configs/*/; do
        for config_file in "${config_folder}"*.config; do
            # Sacar los Windows line endings por las dudas
            sed -i 's/\r$//' "$config_file"
            # Actualizar las IPs
            [[ -n "$kernel_ip" ]] && sed -i -E "s/(IP_KERNEL=).*/\1$kernel_ip/" "$config_file"
            [[ -n "$cpu_ip" ]] && sed -i -E "s/(IP_CPU=).*/\1$cpu_ip/" "$config_file"
            [[ -n "$filesystem_ip" ]] && sed -i -E "s/(IP_FILESYSTEM=).*/\1$filesystem_ip/" "$config_file"
            [[ -n "$memoria_ip" ]] && sed -i -E "s/(IP_MEMORIA=).*/\1$memoria_ip/" "$config_file"
            # Agregar los Windows line endings de nuevo
            sed -i 's/$/\r/' "$config_file"
            printf "Actualizado IPs en $config_file\n"
        done
    done

    printf "\nTodas las IPs mostradas en pantalla se splicaron a todos los archivos de configuración de los Tests.\n"
}

# Ejecutar la funcion "Main"
select_module_ip
