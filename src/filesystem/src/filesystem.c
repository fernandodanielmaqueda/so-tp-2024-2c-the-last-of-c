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

t_Bitmap BITMAP;
pthread_mutex_t MUTEX_BITMAP;

char *PTRO_BLOCKS;
size_t BLOCKS_TOTAL_SIZE;

int module(int argc, char *argv[]) {

	if(initialize_configs(MODULE_CONFIG_PATHNAME)) {
        // TODO
        exit(EXIT_FAILURE);
    }

	initialize_loggers();
	initialize_global_variables();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	bitmap_init(&BITMAP); 

	initialize_sockets();// bucle infinito

	//free_bitmap_blocks();
	//finish_threads();
	finish_sockets();
	//finish_configs();
	finish_loggers();
	finish_global_variables();
   
    return EXIT_SUCCESS;
}

int initialize_global_variables(void) {
    return 0;
}

int finish_global_variables(void) {
    return 0;
}

int read_module_config(t_config* MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "PUERTO_ESCUCHA", "MOUNT_DIR", "BLOCK_SIZE", "BLOCK_COUNT", "RETARDO_ACCESO_BLOQUE", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

	SERVER_FILESYSTEM = (t_Server) {.server_type = FILESYSTEM_PORT_TYPE, .clients_type = MEMORY_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA")};
	MOUNT_DIR = config_get_string_value(MODULE_CONFIG, "MOUNT_DIR");
	BLOCK_SIZE = config_get_int_value(MODULE_CONFIG, "BLOCK_SIZE");
	BLOCK_COUNT = config_get_int_value(MODULE_CONFIG, "BLOCK_COUNT");
	BLOCK_ACCESS_DELAY = config_get_int_value(MODULE_CONFIG, "RETARDO_ACCESO_BLOQUE");
	LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));

    return 0;
}

// Función para los hilos atendededor de un boludo (Uno por cada solicitud de Memoria)
void *filesystem_client_handler_for_memory(t_Client *new_client) {
    char *filename;
    void *memory_dump;
    size_t dump_size;
	t_list* list_bit_index = list_create();

    receive_memory_dump(&filename, &memory_dump, &dump_size, new_client->fd_client);//bloqueante
	size_t necessary_bits_free = necessary_bits(dump_size) + 1; // mas el bloque de INDICE

    pthread_mutex_lock(&MUTEX_BITMAP);

		if( BITMAP.bits_free < necessary_bits_free){
			pthread_mutex_unlock(&MUTEX_BITMAP);
            send_return_value_with_header(MEMORY_DUMP_HEADER, 1, new_client->fd_client);
            close(new_client->fd_client);
            return NULL;
        }

		set_bits_bitmap(&BITMAP, list_bit_index, necessary_bits_free);

    pthread_mutex_unlock(&MUTEX_BITMAP);

    // ESCRIBIR EN BLOQUES.DAT BLOQUE A BLOQUE (se armaron su lista/array dinámico auxiliar)
        // Primero (ó a lo último, como prefieran) escribo en memoria (RAM) el bloque de índice
            // Lógica de escritura del bloque de índice
            // msync() SÓLO CORRESPONDIENTE AL BLOQUE DE ÍNDICE EN SÍ
        // En el medio escribo en memoria (RAM) los bloques de datos

        /*
            for(...) {
                ... = memory_dump[i];
                // msync() SÓLO CORRESPONDIENTE AL BLOQUE EN SÍ
            }
        */

    // Crear el archivo de metadata (es como escribir un config)

    send_return_value_with_header(MEMORY_DUMP_HEADER, 0, new_client->fd_client);
    close(new_client->fd_client);
    return NULL;
}

void set_bits_bitmap(t_Bitmap* bit_map, t_list* list_bit_index,size_t necessary_bits_free){

	bit_map->bits_free -= necessary_bits_free; 

    for (int bit_index = 0; necessary_bits_free > 0; bit_index++) { //5,4,3,2,1,0
            
        bool bit = bitarray_test_bit(bit_map->bits_blocks, bit_index);

        if(!bit) { // entra si está libre (0 es false)
            
            necessary_bits_free--; // restar después de confirmar que hay un bit libre

            // lo cambia a ocupado (1)
            bitarray_set_bit(bit_map->bits_blocks, bit_index); 

            // guardar sus posiciones (nro indice) en lista.
            //list_add(list_bit_index, bit_index); Fer: Van a tener que hacer mallocs si quieren guardar los indices en una lista
        }
    }

	// Sincroniza el archivo. SINCRONIZAR EL ARCHIVO BITMAP.DAT ACTUALIZADO EN RAM COMPLETO EN DISCO
    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }
}

bool exist_free_bits_bitmap(t_Bitmap* bit_map, uint32_t count_block_demand) {
	return (bit_map->bits_free >= (count_block_demand +1));
}

// y bits = redodear(x bytes / 8)
size_t necessary_bits(size_t bytes_size) {
	return (size_t) ceil((double) bytes_size / 8);
}

int bitmap_init(t_Bitmap *bitmap) {

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
        return -1;
    }

	// Darle el tamaño correcto al bitmap
    if (ftruncate(fd, BITMAP_SIZE) == -1) {// No puede darle al archivo el tamaño correcto para bitmap.dat (lo completa con ceros, si no lo recorta)
        log_error(MODULE_LOGGER, "Error al ajustar el tamaño del archivo bitmap.dat: %s", strerror(errno));
        close(fd);
        return -1;
    }

	// traer un archivo a memoria, poder manejarlo 
	// PTRO_BITMAP = referencia en memoria del bitmap
    PTRO_BITMAP = mmap(NULL, BITMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
	if (PTRO_BITMAP == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bitmap.dat a memoria: %s", strerror(errno));
        close(fd);
        return -1;
    }
	
    //puntero a la estructura del bitarray (commons)
    t_bitarray *bit_array = bitarray_create_with_mode((char *) PTRO_BITMAP, BITMAP_SIZE, LSB_FIRST);
    if (bit_array == NULL) {
        log_error(MODULE_LOGGER, "Error al crear la estructura del bitmap");
        munmap(PTRO_BITMAP, BITMAP_SIZE);//liberar la memoria reservada
        close(fd);
        return -1;
    }

	// Instanciar el bitmap
	t_Bitmap* bit_map = malloc(sizeof(t_Bitmap));
    if(bit_map == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el bitmap", sizeof(t_Bitmap));
        return -1;
    }

	bit_map->bits_blocks = bit_array;

	// Inicializar cada bit del bitmap en 0
	for (int bit_index = 0; bit_index < bitarray_get_max_bit(bit_map->bits_blocks); bit_index++) {
		bitarray_clean_bit( bit_map->bits_blocks, bit_index); // Limpia el bit, lo pone en 0
	}

	bit_map->bits_free = BITMAP_SIZE; // Inicialmente todos los bloques estan disponibles

	// Forzamos que los cambios en momoria ppal se reflejen en el archivo.
	// vamos a trabajar siempre en memoria ppal?? si: no hace falta sicronizar siempre.
    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
        return -1;
    }

    log_info(MODULE_LOGGER, "Bitmap creado y mapeado correctamente.");

	return 0;
}

