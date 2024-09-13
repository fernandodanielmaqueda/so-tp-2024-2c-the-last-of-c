/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "filesystem.h"

char *MODULE_NAME = "filesystem";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "filesystem.log";

t_config *MODULE_CONFIG;
char *MODULE_CONFIG_PATHNAME = "filesystem.config";

char *MOUNT_DIR;
size_t BLOCK_SIZE;
size_t BLOCK_COUNT;
int BLOCK_ACCESS_DELAY;

FILE *FILE_BLOCKS;
FILE *FILE_METADATA;
char *PTRO_BITMAP;
size_t BITMAP_SIZE;

t_Bitmap *BITMAP;
//t_bitarray *BITMAP;
char *PTRO_BLOCKS;
size_t BLOCKS_TOTAL_SIZE;

int module(int argc, char *argv[]) {

	initialize_configs(MODULE_CONFIG_PATHNAME);
	initialize_loggers();
	initialize_global_variables();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	BITMAP = initialize_bitmap(); 

	initialize_sockets();// bucle infinito

	free_bitmap_blocks();
	//finish_threads();
	finish_sockets();
	//finish_configs();
	finish_loggers();
	finish_global_variables();
   
    return EXIT_SUCCESS;
}

void initialize_global_variables(void) {

}

void finish_global_variables(void) {

}

void read_module_config(t_config* MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "PUERTO_ESCUCHA", "MOUNT_DIR", "BLOCK_SIZE", "BLOCK_COUNT", "RETARDO_ACCESO_BLOQUE", "LOG_LEVEL", NULL)) {
        //fprintf(stderr, "%s: El archivo de configuración no tiene la propiedad/key/clave %s", MODULE_CONFIG_PATHNAME, "LOG_LEVEL");
        exit(EXIT_FAILURE);
    }

	SERVER_FILESYSTEM = (t_Server) {.server_type = FILESYSTEM_PORT_TYPE, .clients_type = MEMORY_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA")};
	MOUNT_DIR = config_get_string_value(MODULE_CONFIG, "MOUNT_DIR");
	BLOCK_SIZE = config_get_int_value(MODULE_CONFIG, "BLOCK_SIZE");
	BLOCK_COUNT = config_get_int_value(MODULE_CONFIG, "BLOCK_COUNT");
	BLOCK_ACCESS_DELAY = config_get_int_value(MODULE_CONFIG, "RETARDO_ACCESO_BLOQUE");
	LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));
}

// Función para los hilos atendededor de un boludo (Uno por cada solicitud de Memoria)
void *filesystem_client_handler_for_memory(t_Client *new_client) {
    char *filename;
    void *memory_dump;
    size_t dump_size;
	t_list* list_bit_index = list_create(void);

    receive_memory_dump(&filename, &memory_dump, &dump_size, new_client->fd_client);//bloqueante

    pthread_mutex_lock(&MUTEX_BITMAP);

		if(!exist_free_bits_bitmap(BITMAP, necessary_bits(dump_size))){
			pthread_mutex_unlock(&MUTEX_BITMAP);
            send_return_value_with_header(MEMORY_DUMP_HEADER, 1, new_client->fd_client);
            close(new_client->fd_client);
            return NULL;
        }

		set_bits_bitmap(BITMAP, list_bit_index);

    pthread_mutex_unlock(&MUTEX_BITMAP);

    // ESCRIBIR EN BLOQUES.DAT BLOQUE A BLOQUE (se armaron su lista/array dinámico auxiliar)
        // Primero (ó a lo último, como prefieran) escribo en memoria (RAM) el bloque de índice
            // Lógica de escritura del bloque de índice
            // msync() SÓLO CORRESPONDIENTE AL BLOQUE DE ÍNDICE EN SÍ
        // En el medio escribo en memoria (RAM) los bloques de datos
            for(...) {
                ... = memory_dump[i];
                // msync() SÓLO CORRESPONDIENTE AL BLOQUE EN SÍ
            }

    // Crear el archivo de metadata (es como escribir un config)

    send_return_value_with_header(MEMORY_DUMP_HEADER, 0, new_client->fd_client);
    close(new_client->fd_client);
    return NULL;
}

void set_bits_bitmap(t_Bitmap* bit_map, t_list* list_bit_index){

	// recorrer el bitarray seteando los indices de cero a uno
	for (int bit_index = 0; i < bitarray_get_max_bit(bit_map->bits_blocks); i++) {
			
		bool bit = bitarray_test_bit(bit_map->bits_blocks, bit_index);
		if(bit){ // entra si esta libre (0)
			
			// un bloque menos
			bit_map->bits_free -= 1;

			// lo cambia a ocupado (1)
			bitarray_set_bit(bit_map->bits_blocks, bit_index); 

			// guardar sus posiciones (nro indice) en lista.
			list_add(list_bit_index, bit_index);
		}
	}

	// Sincroniza el archivo. SINCRONIZAR EL ARCHIVO BITMAP.DAT ACTUALIZADO EN RAM COMPLETO EN DISCO
    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }
}

bool exist_free_bits_bitmap(t_Bitmap* bit_map, uint32_t count_block_demand){
	return (bit_map->bits_free >= (count_block_demand +1));
}

// y bits = redodear(x bytes / 8)
size_t necessary_bits(size_t bytes_size){
	return (size_t) ceil((double) bytes_size / 8);
}

// ok
t_Bitmap* create_bitmap() {

	// cantidad bits = cantidad de bloques (bytes) sobre 8.
	BITMAP_SIZE = necessary_bits(BLOCK_COUNT);
	
	// ruta al archivo
	char* path_file_bitmap = string_new();
	string_append(&path_file_bitmap, "/");
	string_append(&path_file_bitmap, "bitmap.dat");

	//Checkeo si el file ya esta creado, sino lo elimino
	
	// Abrir el archivo, si no existe lo crea
    int fd = open(path_file_bitmap, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
    if (fd == -1) { //NO pudo abrir el archivo 
        log_error(MODULE_LOGGER, "Error al abrir el archivo bitmap.dat: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

	// Darle el tamaño correcto al bitmap
    if (ftruncate(fd, BITMAP_SIZE) == -1) {// No puede darle al archivo el tamaño correcto para bitmap.dat (lo completa con ceros, si no lo recorta)
        log_error(MODULE_LOGGER, "Error al ajustar el tamaño del archivo bitmap.dat: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

	// traer un archivo a memoria, poder manejarlo 
	// PTRO_BITMAP = referencia en memoria del bitmap
    PTRO_BITMAP = mmap(NULL, BITMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
	if (PTRO_BITMAP == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bitmap.dat a memoria: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }
	
    //puntero a la estructura del bitarray (commons)
    t_bitarray bit_array = bitarray_create_with_mode((char *)PTRO_BITMAP, BITMAP_SIZE,LSB_FIRST);
    if (bitmap == NULL) {
        log_error(MODULE_LOGGER, "Error al crear la estructura del bitmap");
        munmap(PTRO_BITMAP, BITMAP_SIZE);//liberar la memoria reservada
        close(fd);
        exit(EXIT_FAILURE);
    }

	// Instanciar el bitmap
	t_Bitmap* bit_map = malloc(sizeof(t_Bitmap));

	bit_map->bits_blocks = bit_array;

	// Inicializar cada bit del bitmap en 0
	for (int bit_index = 0; i < bitarray_get_max_bit(bit_map->bits_blocks); i++) {
		bitarray_clean_bit( bit_map->bits_blocks, bit_index); // Limpia el bit, lo pone en 0
	}

	bit_map->bits_free = BITMAP_SIZE // Inicialmente todos los bloques estan disponibles

	// Forzamos que los cambios en momoria ppal se reflejen en el archivo.
	// vamos a trabajar siempre en memoria ppal?? si: no hace falta sicronizar siempre.
    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }

    log_info(MODULE_LOGGER, "Bitmap creado y mapeado correctamente.");

	return bit_map;
}

