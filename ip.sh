#!/bin/bash

# Funcion "Main", para desplegar el menu de select y manejar el input
select_module_ip() {
    while true; do
        # Buscamos la primer carpeta de Test adentro de Configs
        configs_folder=configs/
        first_folder=$(ls -d "$configs_folder"*/ | head -n 1)
        folder_name=$(basename "$first_folder")

        printf "\n+-----------------------------------+\nModificador de IPs de configs en $configs_folder\n"
        printf "Se muestran las IPs encontradas en la primera carpeta de configs: $folder_name \n\n"


        # Texto default para cada IP
        cpu_ip="IP NO ENCONTRADA EN LA CONFIG"
        filesystem_ip="IP NO ENCONTRADA EN LA CONFIG"
        memoria_ip="IP NO ENCONTRADA EN LA CONFIG"

        # Guardamos la IP para cada Modulo, checkeando en todas las configs de la primer carpeta y quedandonos con el ultimo match para ese Modulo
        for config_file in "${first_folder}"*.config; do
            if grep -q "^IP_CPU=" "$config_file"; then
                cpu_ip=$(grep -E "^IP_CPU=" "$config_file" | cut -d'=' -f2)
            fi
            if grep -q "^IP_FILESYSTEM=" "$config_file"; then
                filesystem_ip=$(grep -E "^IP_FILESYSTEM=" "$config_file" | cut -d'=' -f2)
            fi
            if grep -q "^IP_MEMORIA=" "$config_file"; then
                memoria_ip=$(grep -E "^IP_MEMORIA=" "$config_file" | cut -d'=' -f2)
            fi
        done

        PS3="Ingrese una de las siguientes opciones para actualizar las IPs (en todas los configs): "
        options=("Salir" "+ CPU [$cpu_ip]" "+ FileSystem [$filesystem_ip]" "+ Memoria [$memoria_ip]" "Resetear todas las IPs a \"127.0.0.1\" (localhost)" "Aplicar las IPs mostradas en pantalla a todos los configs.")
        select opt in "${options[@]}"; do
            case $REPLY in
                1)
                    printf "Saliendo...\n"
                    exit 0
                    ;;
                2)
                    update_ip "CPU" "IP_CPU"
                    break
                    ;;
                3)
                    update_ip "FILESYSTEM" "IP_FILESYSTEM"
                    break
                    ;;
                4)
                    update_ip "MEMORIA" "IP_MEMORIA"
                    break
                    ;;
                5)
                    apply_all_ips "$cpu_ip" "$filesystem_ip" "$memoria_ip"
                    break
                    ;;
                6)
                    apply_all_ips "127.0.0.1" "127.0.0.1" "127.0.0.1" "127.0.0.1"
                    break
                    ;;
                *)
                    echo "Opcion invalida. Intentar nuevamente."
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

    for config_folder in "$configs_folder"*/; do
        for config_file in "${config_folder}"*.config; do
            if grep -q "^$ip_key=" "$config_file"; then

                # Actualizar la IP
                sed -i -E "s/^($ip_key=).*/\1$new_ip/" "$config_file"

                printf "Actualizado IP de $ip_key en $config_file\n"
            fi
        done
    done

    printf "\nIP de $module_name sobreescrita a $new_ip en todas las configs.\n"
}

# Funcion para aplicar todas las IPs mostradas a todos los archivos de configuracion adentro de la carpeta Configs
apply_all_ips() {
    cpu_ip=$1
    filesystem_ip=$2
    memoria_ip=$3

    for config_folder in "$configs_folder"/*/; do
        for config_file in "${config_folder}"*.config; do

            # Actualizar las IPs
            [[ -n "$cpu_ip" ]] && sed -i -E "s/(IP_CPU=).*/\1$cpu_ip/" "$config_file"
            [[ -n "$filesystem_ip" ]] && sed -i -E "s/(IP_FILESYSTEM=).*/\1$filesystem_ip/" "$config_file"
            [[ -n "$memoria_ip" ]] && sed -i -E "s/(IP_MEMORIA=).*/\1$memoria_ip/" "$config_file"

        done
    done

    printf "\nTodas las IPs mostradas en pantalla se splicaron a todos los archivos de configuración de los Tests.\n"
}

# Ejecutar la funcion "Main"
select_module_ip
