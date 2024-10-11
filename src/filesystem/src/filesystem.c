/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "filesystem.h"

char *MODULE_NAME = "filesystem";

t_log *MODULE_LOGGER = NULL;
char *MODULE_LOG_PATHNAME = "filesystem.log";

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME = "filesystem.config";

char *MOUNT_DIR;
size_t BLOCK_SIZE;
size_t BLOCK_COUNT;
int BLOCK_ACCESS_DELAY;

FILE *FILE_BLOCKS;
FILE *FILE_METADATA;
char *PTRO_BITMAP;
size_t BITMAP_FILE_SIZE;

t_Bitmap BITMAP;
pthread_mutex_t MUTEX_BITMAP;

void *PTRO_BLOCKS;

int module(int argc, char *argv[]) {

	if(initialize_configs(MODULE_CONFIG_PATHNAME)) {
        // TODO
        exit(EXIT_FAILURE);
    }

	initialize_loggers();
	initialize_global_variables();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	bitmap_init(&BITMAP); //bitmap.dat
    bloques_init(); //bloques.dat

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

int bitmap_init(t_Bitmap *bitmap) {

    // cantidad bits = cantidad de bloques (bytes) sobre 8.
	BITMAP_FILE_SIZE = necessary_bits(BLOCK_COUNT);

	// ruta al archivo
	char* path_file_bitmap = string_new();
	string_append(&path_file_bitmap, "./");
	string_append(&path_file_bitmap, "bitmap.dat");

	//Checkeo si el file ya esta creado, sino lo elimino
	
	// Abrir el archivo, si no existe lo crea
    int fd = open(path_file_bitmap, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(fd == -1) { //NO pudo abrir el archivo 
        log_error(MODULE_LOGGER, "Error al abrir el archivo bitmap.dat: %s", strerror(errno));
        return -1;
    }

	// Darle el tamaño correcto al bitmap
    if(ftruncate(fd, BITMAP_FILE_SIZE) == -1) {// No puede darle al archivo el tamaño correcto para bitmap.dat (lo completa con ceros, si no lo recorta)
        log_error(MODULE_LOGGER, "Error al ajustar el tamaño del archivo bitmap.dat: %s", strerror(errno));
        if(close(fd)) {
            log_error_close();
        }
        return -1;
    }

	// traer un archivo a memoria, poder manejarlo 
	// PTRO_BITMAP = referencia en memoria del bitmap
    PTRO_BITMAP = mmap(NULL, BITMAP_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
	if(PTRO_BITMAP == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bitmap.dat a memoria: %s", strerror(errno));
        if(close(fd)) {
            log_error_close();
        }
        return -1;
    }
	
    //puntero a la estructura del bitarray (commons)
    t_bitarray *bit_array = bitarray_create_with_mode((char *) PTRO_BITMAP, BITMAP_FILE_SIZE, LSB_FIRST);
    if(bit_array == NULL) {
        log_error(MODULE_LOGGER, "Error al crear la estructura del bitmap");
        munmap(PTRO_BITMAP, BITMAP_FILE_SIZE);//liberar la memoria reservada
        if(close(fd)) {
            log_error_close();
        }
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
	for(int bit_index = 0; bit_index < bitarray_get_max_bit(bit_map->bits_blocks); bit_index++) {
		bitarray_clean_bit( bit_map->bits_blocks, bit_index); // Limpia el bit, lo pone en 0
	}

	bit_map->blocks_free = BLOCK_COUNT; // Inicialmente todos los bloques estan disponibles

	// Forzamos que los cambios en momoria ppal se reflejen en el archivo.
	// vamos a trabajar siempre en memoria ppal?? si: no hace falta sicronizar siempre.
    if(msync(PTRO_BITMAP, BITMAP_FILE_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
        return -1;
    }

    log_info(MODULE_LOGGER, "Bitmap creado y mapeado correctamente.");

	return 0;
}

//bloques.dat
int bloques_init(void) {

    // Ruta al archivo
    char *path_file_blocks = string_new();
    string_append(&path_file_blocks, "./");
    string_append(&path_file_blocks, "bloques.dat");

    // Checkeo si el file ya esta creado, sino lo elimino

    // Abrir el archivo, si no existe lo crea
    int fd = open(path_file_blocks, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(fd == -1) { //NO pudo abrir el archivo 
        log_error(MODULE_LOGGER, "Error al abrir el archivo bloques.dat: %s", strerror(errno));
        return -1;
    }

    // Darle el tamaño correcto al bitmap
    if(ftruncate(fd, BLOCKS_TOTAL_SIZE) == -1) {// No puede darle al archivo el tamaño correcto para bitmap.dat (lo completa con ceros, si no lo recorta)
        log_error(MODULE_LOGGER, "Error al ajustar el tamaño del archivo bloques.dat: %s", strerror(errno));
        if(close(fd)) {
            log_error_close();
        }
        return -1;
    }

    // Traer un archivo a memoria, poder manejarlo 
    // PTRO_BITMAP = referencia en memoria del bitmap
    PTRO_BLOCKS = mmap(NULL, BLOCKS_TOTAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(PTRO_BLOCKS == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bloques.dat a memoria: %s", strerror(errno));
        if(close(fd)) {
            log_error_close();
        }
        return -1;
    }

    //SINCRONIZO CON EL ARCHIVO
    if(msync(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        return -1;
    }

    log_info(MODULE_LOGGER, "Bloques.dat creado y mapeado correctamente.");

    return 0;
}

void filesystem_client_handler_for_memory(int fd_client) {
    char *filename;
    void *memory_dump;
    size_t dump_size;
    size_t blocks_necessary;
    int status;

    receive_memory_dump(&filename, &memory_dump, &dump_size, fd_client);//bloqueante

    // suponiendo que dump_size esta en bytes, BLOCK_SIZE suponemos es en bytes
    blocks_necessary = (size_t) ceil((double) dump_size / BLOCK_SIZE) + 1 ; // datos: 2 indice: 1 = 3 bloques 

    t_Block_Pointer array[blocks_necessary]; // array[3]: 0,1,2
 	
    // Verificar si hay suficientes bloques libres para almacenar el archivo
    if((status = pthread_mutex_lock(&MUTEX_BITMAP))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }
		if(BITMAP.blocks_free < blocks_necessary) {
			if((status = pthread_mutex_unlock(&MUTEX_BITMAP))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }
            send_result_with_header(MEMORY_DUMP_HEADER, 1, fd_client);
			log_warning(MODULE_LOGGER, "No hay suficientes bloques libres para almacenar el archivo %s", filename);
            return;
        }

        BITMAP.blocks_free -= blocks_necessary;

        // Setear los bits correspondientes en el bitmap, completar el array de posiciones del indice de bloques.dat.
		set_bits_bitmap(&BITMAP, array, blocks_necessary);


    // dar acceso al bitmap.dat a los otros hilos.
    if((status = pthread_mutex_unlock(&MUTEX_BITMAP))) {
        log_error_pthread_mutex_unlock(status);
        // TODO

    }

   
    // ESCRIBIR EN BLOQUES.DAT BLOQUE A BLOQUE (se armaron su lista/array dinámico auxiliar)

    // Primero escribo en memoria (RAM) el bloque de índice
    write_block(array[0], array, blocks_necessary * sizeof(t_Block_Pointer)); //array 0 porque el primero es el indice
    block_msync(array[0]); // msync() SÓLO CORRESPONDIENTE AL BLOQUE DE ÍNDICE EN SÍ


    // En el medio escribo en memoria (RAM) los bloques de datos
    // blocks_necessary es igual al tamaño del array
    for(size_t i = 1; i < blocks_necessary; i++) {
        // ptro al inicio de cada bloque dentro de los datos del memory dump (NO INDEXADO).
        char *ptro_memory_dump_block = get_pointer_to_block(memory_dump, BLOCK_SIZE, i - 1);

        // array[i]: NRO DE INDICE de cada bloque
        write_block(array[i], ptro_memory_dump_block, BLOCK_SIZE);

        block_msync(array[i]); // msync() SÓLO CORRESPONDIENTE AL BLOQUE EN SÍ
    }


    // Crear el archivo de metadata (es como escribir un config)
    create_metadata_file();


    send_result_with_header(MEMORY_DUMP_HEADER, 0, fd_client);
    return;
}

void create_metadata_file(){
    
}

void set_bits_bitmap(t_Bitmap *bit_map, t_Block_Pointer *array, size_t blocks_necessary) { // 3 bloques: 2 de datos y 1 de índice

    // Ya sabemos de antemano que hay suficientes bloques libres para almacenar el archivo
    register size_t block_index = 0, logic_index = 0;
    for(; blocks_necessary > 0; block_index++) {

        bool bit = bitarray_test_bit(bit_map->bits_blocks, block_index);

        if(!bit) { // entra si está libre (0 es false)
                        // lo cambia a ocupado (1)
            bitarray_set_bit(bit_map->bits_blocks, block_index); 
            // guardar sus posiciones (nro indice) en lista.
            array[logic_index] = block_index;

            logic_index++;
            blocks_necessary--; // restar después de confirmar que hay un bit libre
        }
    }

    // Llevar a una función aparte
	// Sincroniza el archivo. SINCRONIZAR EL ARCHIVO BITMAP.DAT ACTUALIZADO EN RAM COMPLETO EN DISCO
    if(msync(PTRO_BITMAP, BITMAP_FILE_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }
}


// y bits = redodear(x bytes / 8)
size_t necessary_bits(size_t bytes_size) {
	return (size_t) ceil((double) bytes_size / 8);
}

// cada block es de BLOCK_SIZE
//////////////////////////////////////////  TODO ///////////////////////////////

void *get_pointer_to_block(void *file_ptr, size_t file_block_size, t_Block_Pointer file_block_pos) {
    // Calcular la dirección del bloque deseado
    void *block_ptr = (void *) (((uint8_t *) file_ptr) + (file_block_size * file_block_pos));
    return block_ptr;
}

void *get_pointer_index_bloquesdat(t_Block_Pointer file_block_pos) { //el indice es un bloque
    if(file_block_pos >= BLOCK_COUNT) {
         
        log_error(MODULE_LOGGER, "Error: el bloque %d no existe en bloques.dat", file_block_pos);
        return NULL;
    }

    return get_pointer_to_block(PTRO_BLOCKS, BLOCK_SIZE, file_block_pos);
}

// array[0]=2 (t_Block_Pointer), arry[1]=0, array[2]=null : bytes: INDICE t_Block_Pointer(4bytes),t_Block_Pointer 

/*

    ----
    memoria Array: 3 (4 bytes), 4 (4 bytes), 5 (4 bytes)
    copia  memoria Array  --> memoria indice: x1 (4 bytes), x2 (4 bytes), x3 (4 bytes) 
    ---
    block ocupados pos: 0 (1), 1 (1), 2 (1), 3 (0), 4 (0), 5 (0), 6 (0), ...
    dos bloques + block indice = 3 block -->  3 (0), 4 (0), 5 (0),
    array[0]=3  // (OK) Q POSICION LE ASIGNAMOS AL BLOQUE INDICE ?? SIEMPRE EL DE LA POSICION 0 ??
    array[1]=4
    array[3]=5  // Q POSICION LE ASIGNAMOS AL BLOQUE INDICE ?? SIEMPRE EL DE LA POSICION FINAL SIZE-1 ??

   write_block(nro_bloque, ptro_datos, desplazamiento)
      1) calcular donde inicia el ptro al bloque donde vamos a copiar los datos
        
        // apuntar a la posicion del index del bloquest.dat donde vamos a copiar
        char* ptro_bloque_indice = get_pointer_to_block(PTRO_BLOCKS, BLOCK_SIZE, nro_bloque)
      
      2) copiar los datos al bloque respectivo
        
        // ptro donde vas a copiar, puntero donde esta los bytes a copiar, cantidad de bytes a copiar  
        copy_in_block(ptro_bloque_indice, ptro_datos, desplazamiento)


   write_indice() 

        t_Block_Pointer nro_bloque = array[0]
        char* ptro_datos = array->datos
        size_t  cantidad_bloques = array->size
        size_t desplazamiento = cantidad_bloques * size(t_Block_Pointer)

        write_block(nro_bloque, ptro_datos, desplazamiento)
    
   
   write_data()
      
        for(size_t pos_index=1, pos_index =< cantidad_bloques, pos_index++) {

                t_Block_Pointer nro_bloque = array[pos_index]

                // apuntar a la posicion de los datos del memory dump desde donde vamos a copiar
                char* ptro_memory_dump_block = get_pointer_to_block(memory_dump, BLOCK_SIZE, nro_bloque)

                write_block(nro_bloque, ptro_datos, BLOCK_SIZE)
        }





*/


// t_Block_Pointer file_block_pos = get_block_pos();
// get_pointer_to_block(PTRO_BLOCKS, BLOCK_SIZE, file_block_pos);

void block_msync(t_Block_Pointer block_number) { // 2
    void *init_group_blocks = get_pointer_index_bloquesdat(block_number);
    if(init_group_blocks == NULL) {
        log_error(MODULE_LOGGER, "Error al obtener el puntero al bloque %d de bloques.dat", block_number);
        return;
    }

    // Sincroniza el archivo. SINCRONIZAR EL ARCHIVO BLOQUES.DAT ACTUALIZADO EN RAM COMPLETO EN DISCO
    if(msync(init_group_blocks, BLOCK_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
    }
            
}

void copy_in_block(void* ptro_bloque_indice, void* ptro_datos, size_t desplazamiento) {
    // Copiar los datos al bloque respectivo
    memcpy(ptro_bloque_indice, ptro_datos, desplazamiento);
}



/*

memory RAM:  datos del memory dump:  |(ptro 1) bloque_size |(ptro 2) bloque_size | ... 

  

memory RAM:  bloques.dat: |(index 0 --> ptro x1) bloque_size |(index 1 --> ptro x2) bloque_size | ... 




*/
void write_block(t_Block_Pointer nro_bloque, void* ptro_datos, size_t desplazamiento) {
  
    // 1) calcular donde inicia el ptro al bloque donde vamos a copiar los datos
        
        // apuntar a la posicion del index del bloques.dat donde vamos a copiar
        void* ptro_bloque_indice = get_pointer_index_bloquesdat(nro_bloque);
      
    //  2) copiar los datos al bloque respectivo
        
        // ptro donde vas a copiar, puntero donde esta los bytes a copiar, cantidad de bytes a copiar  
        copy_in_block(ptro_bloque_indice, ptro_datos, desplazamiento);
}
    


void write_indice(void) {
    /*
    t_Block_Pointer nro_bloque = array[0];
    char *ptro_datos = array;
    size_t  cantidad_bloques = array->size;
    size_t desplazamiento = cantidad_bloques * size(t_Block_Pointer);

    write_block(nro_bloque, ptro_datos, desplazamiento);
    */
} 

    
   
void write_data(void *memory_dump) {
    /*
    size_t  cantidad_bloques = array->size;

    for(size_t pos_index=1; cantidad_bloques <= pos_index; pos_index++) {

            t_Block_Pointer nro_bloque = array[pos_index];

            // apuntar a la posicion de los datos del memory dump desde donde vamos a copiar
            char* ptro_memory_dump_block = get_pointer_to_block(memory_dump, BLOCK_SIZE, nro_bloque);

            write_block(nro_bloque, ptro_memory_dump_block, BLOCK_SIZE);
    }
    */
}
