/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "filesystem.h"

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
    int status;

    MODULE_NAME = "filesystem";
    MODULE_CONFIG_PATHNAME = "filesystem.config";


	// Bloquea todas las señales para este y los hilos creados
	sigset_t set;
	if(sigfillset(&set)) {
		report_error_sigfillset();
		return EXIT_FAILURE;
	}
	if((status = pthread_sigmask(SIG_BLOCK, &set, NULL))) {
		report_error_pthread_sigmask(status);
		return EXIT_FAILURE;
	}


    pthread_t thread_main = pthread_self();


	// Crea hilo para manejar señales
	if((status = pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main))) {
		report_error_pthread_create(status);
		return EXIT_FAILURE;
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


	// Config
	if((MODULE_CONFIG = config_create(MODULE_CONFIG_PATHNAME)) == NULL) {
		fprintf(stderr, "%s: No se pudo abrir el archivo de configuracion\n", MODULE_CONFIG_PATHNAME);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) config_destroy, MODULE_CONFIG);


	// Parse config
	if(read_module_config(MODULE_CONFIG)) {
		fprintf(stderr, "%s: El archivo de configuración no se pudo leer correctamente\n", MODULE_CONFIG_PATHNAME);
		exit_sigint();
	}


	// Loggers
	if((status = pthread_mutex_init(&MUTEX_LOGGERS, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_LOGGERS);

	if(logger_init(&MODULE_LOGGER, MODULE_LOGGER_INIT_ENABLED, MODULE_LOGGER_PATHNAME, MODULE_LOGGER_NAME, MODULE_LOGGER_INIT_ACTIVE_CONSOLE, MODULE_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &MODULE_LOGGER);

	if(logger_init(&MINIMAL_LOGGER, MINIMAL_LOGGER_INIT_ENABLED, MINIMAL_LOGGER_PATHNAME, MINIMAL_LOGGER_NAME, MINIMAL_LOGGER_ACTIVE_CONSOLE, MINIMAL_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &MINIMAL_LOGGER);

	if(logger_init(&SOCKET_LOGGER, SOCKET_LOGGER_INIT_ENABLED, SOCKET_LOGGER_PATHNAME, SOCKET_LOGGER_NAME, SOCKET_LOGGER_INIT_ACTIVE_CONSOLE, SOCKET_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &SOCKET_LOGGER);

	if(logger_init(&SERIALIZE_LOGGER, SERIALIZE_LOGGER_INIT_ENABLED, SERIALIZE_LOGGER_PATHNAME, SERIALIZE_LOGGER_NAME, SERIALIZE_LOGGER_INIT_ACTIVE_CONSOLE, SERIALIZE_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &SERIALIZE_LOGGER);


	log_debug_r(&MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

    
    create_directory(MOUNT_DIR);

    char *path_files = string_from_format("%s/files", MOUNT_DIR);
    pthread_cleanup_push((void (*)(void *)) free, path_files);
        create_directory(path_files);
    pthread_cleanup_pop(1); // path_files


	bitmap_init(); //bitmap.dat
    bloques_init(); //bloques.dat


	// COND_CLIENTS
	if((status = pthread_cond_init(&COND_CLIENTS, NULL))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_CLIENTS);

    // LIST_FILESYSTEM_JOBS
    if((status = pthread_mutex_init(&(SHARED_LIST_CLIENTS.mutex), NULL))) {
        report_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_CLIENTS.mutex));

    // LIST_FILESYSTEM_JOBS
    SHARED_LIST_CLIENTS.list = list_create();
    if(SHARED_LIST_CLIENTS.list == NULL) {
        log_error_r(&MODULE_LOGGER, "list_create: No se pudo crear la lista de clientes del kernel");
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) list_destroy, SHARED_LIST_CLIENTS.list);


	// [Servidor] Filesystem <- [Cliente(s)] Memoria
    pthread_cleanup_push((void (*)(void *)) wait_client_threads, NULL);
	server_thread_coordinator(&SERVER_FILESYSTEM, filesystem_client_handler);

    // Cleanup

    pthread_cleanup_pop(1); // wait_client_threads
	pthread_cleanup_pop(1); // LIST_FILESYSTEM_JOBS
	pthread_cleanup_pop(1); // MUTEX_FILESYSTEM_JOBS
	pthread_cleanup_pop(1); // COND_CLIENTS
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_LOGGERS
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // THREAD_SIGNAL_MANAGER

    return EXIT_SUCCESS;
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

int bitmap_init() {

    // cantidad de Bytes del archivo bitmap.dat.  Nota: cantidad bits = cantidad de bloques (bytes) sobre 8.
	BITMAP_FILE_SIZE = necessary_bits(BLOCK_COUNT);

    //QUIERO IMPRIMIR EL TAMANIO BITMAP_FILE_SIZE
    log_trace_r(&MODULE_LOGGER, "######## TAMANIO DE BITMAP_FILE_SIZE: %zu", BITMAP_FILE_SIZE);

	// ruta al archivo
	char* path_file_bitmap = string_new();
	string_append(&path_file_bitmap, MOUNT_DIR);
	string_append(&path_file_bitmap, "/bitmap.dat");
	
	// Abrir el archivo, si no existe lo crea
    int fd = open(path_file_bitmap, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
    if(fd == -1) { //NO pudo abrir el archivo 
    log_error_r(&MODULE_LOGGER, "Error al abrir el archivo bitmap.dat: %s", strerror(errno));
    return -1;
    }

	// Darle el tamaño correcto al bitmap -- y nos inicia todo en 0
    if(ftruncate(fd, BITMAP_FILE_SIZE) == -1) {// No puede darle al archivo el tamaño correcto para bitmap.dat (lo completa con ceros, si no lo recorta)
        log_error_r(&MODULE_LOGGER, "Error al ajustar el tamaño del archivo bitmap.dat: %s", strerror(errno));
        if(close(fd)) {
            report_error_close();
        }
        return -1;
    }

	// traer un archivo a memoria, poder manejarlo 
	// PTRO_BITMAP = referencia en memoria del bitmap
    PTRO_BITMAP = mmap(NULL, BITMAP_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    
	if(PTRO_BITMAP == MAP_FAILED) {
        log_error_r(&MODULE_LOGGER, "Error al mapear el archivo bitmap.dat a memoria: %s", strerror(errno));
        if(close(fd)) {
            report_error_close();
        }
        return -1;
    }

    char *data_string = mem_hexstring(PTRO_BITMAP, BITMAP_FILE_SIZE);
    log_trace_r(&MODULE_LOGGER, "## DATA STRING BITMAP.dat:\n%s", data_string);
    free(data_string);

    //puntero a la estructura del bitarray (commons)
    t_bitarray *bit_array = bitarray_create_with_mode((char *) PTRO_BITMAP, BITMAP_FILE_SIZE, LSB_FIRST);
    
    if(bit_array == NULL) {
        log_error_r(&MODULE_LOGGER, "Error al crear la estructura del bitmap");
        munmap(PTRO_BITMAP, BITMAP_FILE_SIZE);//liberar la memoria reservada
        if(close(fd)) {
            report_error_close();
        }
        return -1;
    }

	// Instanciar el bitmap
	//t_Bitmap* bit_map = malloc(sizeof(t_Bitmap));
   	BITMAP.bits_blocks = bit_array;
   // log_trace_r(&MODULE_LOGGER, "##### Tamanio bits_blocks: %zu", bitarray_get_max_bit(BITMAP.bits_blocks));
    
   
	//BITMAP.blocks_free = BLOCK_COUNT; // Inicialmente todos los bloques estan disponibles
    set_bitmap_bits_free(&BITMAP);
    //log_trace_r(&MODULE_LOGGER, "### Tamanio de los blocks_free: %zu", BITMAP.blocks_free);

	// Forzamos que los cambios en momoria ppal se reflejen en el archivo.
	// vamos a trabajar siempre en memoria ppal?? si: no hace falta sicronizar siempre.
    if(msync(PTRO_BITMAP, BITMAP_FILE_SIZE, MS_SYNC) == -1) {
        log_error_r(&MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
        return -1;
    }
    log_trace_r(&MODULE_LOGGER," Archivo Creado y sincronizado: bitmap.dat. Tamaño: <%zu>", BITMAP_FILE_SIZE);

    
	return 0;
}

//bloques.dat
int bloques_init(void) {

    // Ruta al archivo
    char *path_file_blocks = string_new();
    string_append(&path_file_blocks, MOUNT_DIR);
    string_append(&path_file_blocks, "/bloques.dat");

    // Abrir el archivo, si no existe lo crea
    int fd = open(path_file_blocks, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(fd == -1) { //NO pudo abrir el archivo 
        log_error_r(&MODULE_LOGGER, "Error al abrir el archivo bloques.dat: %s", strerror(errno));
        return -1;
    }

    // Darle el tamaño correcto al bitmap -- y nos inicia todo en 0
    if(ftruncate(fd, BLOCKS_TOTAL_SIZE) == -1) {// No puede darle al archivo el tamaño correcto para bitmap.dat (lo completa con ceros, si no lo recorta)
        log_error_r(&MODULE_LOGGER, "Error al ajustar el tamaño del archivo bloques.dat: %s", strerror(errno));
        if(close(fd)) {
            report_error_close();
        }
        return -1;
    }

    // Traer un archivo a memoria, poder manejarlo 
    // PTRO_BITMAP = referencia en memoria del bitmap
    PTRO_BLOCKS = mmap(NULL, BLOCKS_TOTAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(PTRO_BLOCKS == MAP_FAILED) {
        log_error_r(&MODULE_LOGGER, "Error al mapear el archivo bloques.dat a memoria: %s", strerror(errno));
        if(close(fd)) {
            report_error_close();
        }
        return -1;
    }

    //SINCRONIZO CON EL ARCHIVO
    if(msync(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE, MS_SYNC) == -1) {
        log_error_r(&MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        return -1;
    }

    char *data_string = mem_hexstring(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE);
    log_trace_r(&MODULE_LOGGER, "## DATA COMPOSICION  BLOQUES.dat:\n%s", data_string);
  

    return 0;
}

bool is_address_in_mapped_area(void *addr) {
    // Calcular la dirección de fin del área mapeada
    void *end_addr = (char *)PTRO_BLOCKS + BLOCKS_TOTAL_SIZE;

    // Verificar si la dirección está dentro del rango
    return (addr >= PTRO_BLOCKS && addr < end_addr);
}


void print_memory_as_ints(void *ptro_memory_dump_block) {
    int *int_ptr = (int *)ptro_memory_dump_block;
    for (int i = 0; i < 8; i++) {
        log_trace_r(&MODULE_LOGGER, "Valor %d: %d", i, int_ptr[i]);
    }
}

void print_size_t_array(void *array, size_t total_size) {
    size_t num_elements = total_size / sizeof(size_t);
   // log_error_r(&MODULE_LOGGER, "num_elements: %zu", num_elements);
    size_t *size_t_array = (size_t *)array;

    for (size_t i = 0; i < num_elements; i++) {
      //  log_error_r(&MODULE_LOGGER, "Elemento %zu: %zu", i, size_t_array[i]);
    }
}



void filesystem_client_handler_for_memory(int fd_client) {
    char *filename;
    void *memory_dump;
    size_t dump_size;
    size_t blocks_necessary;
    int status;

    receive_dump_memory(&filename, &memory_dump, &dump_size, fd_client);//bloqueante
    

    // agregamos un log para ver los datos recibidos
    log_trace_r(&MODULE_LOGGER, "##### Recibi la solicitud - Archivo: <%s> - Dump Size: <%zu> Bytes - BLOCKS_TOTAL_SIZE: <%zu> Bytes  #####", filename, dump_size, BLOCKS_TOTAL_SIZE);

    // suponiendo que dump_size esta en bytes, BLOCK_SIZE  (blocks necesary es del dump)
    blocks_necessary = (size_t) ceil((double) dump_size / BLOCK_SIZE) + 1 ; // datos: 2 indice: 1 = 3 bloques 

    log_trace_r(&MODULE_LOGGER, "##### Calculo bloques q necesita el mem dump: <%lu> Bytes - BITMAP blocks_free: <%zu> Bytes #####", blocks_necessary, BITMAP.blocks_free);

    t_Block_Pointer array[blocks_necessary]; // array[3]: 0,1,2
 	//Inicio bloqueo zona critica 
    if((status = pthread_mutex_lock(&MUTEX_BITMAP))) {
        report_error_pthread_mutex_lock(status);
        // TODO
    }

        // Verificar que no excedas la cantidad de punteros que un indice puede almacenar
        size_t bytes_ptro = 4;
        size_t nro_max_ptros =  (size_t) ceil((double)  BLOCK_SIZE / bytes_ptro) ;
        size_t nro_bloques_datos = (blocks_necessary-1) ;

        //log_trace_r(&MODULE_LOGGER, "Numero maximo de punteros <%zu> para almacenar los punteros a los bloques de datos <%zu>", nro_max_ptros, nro_bloques_datos);

        if(nro_bloques_datos > nro_max_ptros) {
            if((status = pthread_mutex_unlock(&MUTEX_BITMAP))) {
                report_error_pthread_mutex_unlock(status);
                // TODO
            }
            send_result_with_header(DUMP_MEMORY_HEADER, 1, fd_client);
			log_warning_r(&MODULE_LOGGER, "Excede el maximo numero de punteros <%zu> para almacenar los punteros a los bloques de datos <%zu>", nro_max_ptros, nro_bloques_datos);
            return;
        }

        // Verificar si hay suficientes bloques libres para almacenar el archivo
		if(BITMAP.blocks_free < blocks_necessary ) {
			if((status = pthread_mutex_unlock(&MUTEX_BITMAP))) {
                report_error_pthread_mutex_unlock(status);
                // TODO
            }
            send_result_with_header(DUMP_MEMORY_HEADER, 1, fd_client);
			log_warning_r(&MODULE_LOGGER, "No hay suficientes bloques libres para almacenar el archivo %s", filename);
            return;
        }

        // Setear los bits correspondientes en el bitmap, completar el array de posiciones del indice de bloques.dat.
		set_bits_bitmap(&BITMAP, array, blocks_necessary, filename);


    // dar acceso al bitmap.dat a los otros hilos.
    if((status = pthread_mutex_unlock(&MUTEX_BITMAP))) {
        report_error_pthread_mutex_unlock(status);
        // TODO

    }

    // Crear el archivo de metadata (es como escribir un config)
    create_metadata_file( filename,dump_size, array[0]);


    // ------------ ESCRIBIR EN BLOQUES.DAT BLOQUE A BLOQUE (se armaron su lista/array dinámico auxiliar) ----------------
    //write_Complete_data(array, blocks_necessary);
    

    // Primero escribo en memoria (RAM) el bloque INDICE ===========*//
    size_t array_size = blocks_necessary * sizeof(t_Block_Pointer);
    t_Block_Pointer bloques_index_pos = array[0];
  
    // ptro al espacio de memoria de bloques.dat donde inicia el bloque de la posicion [pos]
    void *pointer_to_block_index = get_pointer_to_memory(PTRO_BLOCKS, BLOCK_SIZE, bloques_index_pos);

    // Copiamos todo el array en el bloque de indice 
    memcpy(pointer_to_block_index, array, array_size); //array 0 porque el primero es el indice
    log_info_r(&MODULE_LOGGER, "##  Archivo: <%s> - Bloque Index - Nro Bloque <%u>", filename, bloques_index_pos);
    
    // printear los punteros (4 bytes) del bloque indice
    print_memory_as_ints(pointer_to_block_index);
    // Log de acceso a bloque.
     
     usleep(BLOCK_ACCESS_DELAY * 1000);//Tiempo en milisegundos que se deberá esperar luego de cada acceso a bloques (de datos o punteros)
   
    // En el medio escribo en memoria (RAM) los bloques de datos
    // blocks_necessary es igual al tamaño del array

    uint32_t dump_pos = 0;
    //  array_pos = 1 porque desde la posicion en adelante el array tiene value los punteros a los bloques de datos. La posicion tiene como value el puntero el bloque de indice.

        for(size_t array_pos = 1; array_pos < blocks_necessary; array_pos++) {
        // ptro al inicio de cada bloque dentro de los datos del memory dump (NO INDEXADO).
        // ptro al inicio de cada particion (block size) del memory dump  
        void *ptro_memory_dump_block = get_pointer_to_memory_dump(memory_dump, BLOCK_SIZE, dump_pos);
        dump_pos++;
        // array[array_pos]: NRO DE INDICE de cada bloque
        t_Block_Pointer bloques_data_pos_init = array[array_pos]; //empieza  arecorrer desde el bloque 1 porque en el bloque 0 esta el indice

        // deplazamiento o tamaño de la particion: cantidad de bytes
        // nro de particion: posicion del bloque o particion (0,1,2,...)
        size_t memory_partition_size = BLOCK_SIZE;
          
        // Entra en el caso del ultimo bloque del datos dump
        if( array_pos == (blocks_necessary-1)){
            memory_partition_size = ((BLOCK_SIZE * (blocks_necessary-1)) - dump_size) ; // 250 - ( 64 * (4-1)) =   250 - 192 = 58
        }
        log_info_r(&MODULE_LOGGER, "##  Archivo: <%s> - Bloque Datos - Nro Bloque <%u>", filename, bloques_data_pos_init);
             
      
        // ptro al espacio de memoria de bloques.dat donde inicia el bloque de la posicion [pos]
        void *pointer_to_block_data = get_pointer_to_memory(PTRO_BLOCKS, BLOCK_SIZE, bloques_data_pos_init);

        // Copiamos  particiones del dump en los bloques        -- ex write block  
        memcpy(pointer_to_block_data, ptro_memory_dump_block, memory_partition_size);   

        usleep(BLOCK_ACCESS_DELAY * 1000);//Tiempo en milisegundos que se deberá esperar luego de cada acceso a bloques (de datos o punteros)
       
        block_msync(pointer_to_block_data); // msync() SÓLO CORRESPONDIENTE AL BLOQUE EN SÍ
    }

    send_result_with_header(DUMP_MEMORY_HEADER, 0, fd_client);
    // Log de fin de petición
    log_info_r(&MINIMAL_LOGGER, "## Fin de solicitud - Archivo: %s", filename);
    free(filename);
    return;
}


void create_metadata_file(const char *filename, size_t size, t_Block_Pointer index_block) {
    // Crear la ruta completa del archivo de metadata
    char *metadata_path = string_from_format("%s/files/%s", MOUNT_DIR, filename);
    if (metadata_path == NULL) {
        log_error_r(&MODULE_LOGGER, "No se pudo crear la ruta del archivo de metadata.");
        return;
    }

    // Crear el archivo de configuración
    //crear el archivo en la ruta MOUNT_DIR/files/
    FILE* fd_metadata = fopen(metadata_path, "w");
    fclose(fd_metadata);

    t_config *metadata_config = config_create(metadata_path);
    if (metadata_config == NULL) {
        log_error_r(&MODULE_LOGGER, "No se pudo crear el archivo de metadata %s.", metadata_path);
        free(metadata_path);
        return;
    }

    // Agregar las claves y valores al archivo de configuración
    char *size_ptr = string_from_format("%zu", size);
    config_set_value(metadata_config, "SIZE",size_ptr);
    free(size_ptr);

    char *index_block_ptr = string_from_format("%u", index_block);
    config_set_value(metadata_config, "INDEX_BLOCK", index_block_ptr);
    free(index_block_ptr);

    // Guardar y destruir el archivo de configuración
    config_save(metadata_config);
    config_destroy(metadata_config);

    // Liberar la memoria reservada para la ruta del archivo de metadata
    free(metadata_path);

    // path completo (metadata_path) o solo filename??
    log_info_r(&MINIMAL_LOGGER, "## Archivo Creado: %s - Tamaño: %zu", filename, size);
}



void set_bitmap_bits_free(t_Bitmap * bit_map){

    bit_map->blocks_free = 0;
    size_t bit_map_size =  bitarray_get_max_bit(bit_map->bits_blocks);
    //haceme un log donde me diga bitarray_get_max_bit
    log_trace_r(&MODULE_LOGGER, "##### Tamanio de bit_map_size: %zu", bit_map_size);
    

    for (size_t nro_bloque = 0; nro_bloque < bit_map_size; nro_bloque++) {

        bool bit = bitarray_test_bit(bit_map->bits_blocks, nro_bloque);

        if(!bit) { // entra si está libre (0 es false)
                    
            bit_map->blocks_free++;
        }
    }
    //haceme un log para imprimir el bit_map->blocks_free
    log_trace_r(&MODULE_LOGGER, "##### Tamanio de bit_map->blocks_free: %zu", bit_map->blocks_free);
}

void set_bits_bitmap(t_Bitmap *bit_map, t_Block_Pointer *array, size_t blocks_necessary, char* filename) { // 3 bloques: 2 de datos y 1 de índice

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

            bit_map->blocks_free--;

            // Log de asignación de bloque
            log_info_r(&MINIMAL_LOGGER, "## Bloque asignado: %zu - Archivo: %s - Bloques Libres: %zu", block_index, filename, bit_map->blocks_free);
        }
    }

    // Llevar a una función aparte
	// Sincroniza el archivo. SINCRONIZAR EL ARCHIVO BITMAP.DAT ACTUALIZADO EN RAM COMPLETO EN DISCO
    if(msync(PTRO_BITMAP, BITMAP_FILE_SIZE, MS_SYNC) == -1) {
        log_error_r(&MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }
}


// y bits = redodear(x bytes / 8)
size_t necessary_bits(size_t bytes_size) {
	return (size_t) ceil((double) bytes_size / 8);
}

// Calcular la dirección de memoria de una particion (ya sea para el archivo q envia memoria o el bloques .dat )
void *get_pointer_to_memory(void * memory_ptr, size_t memory_partition_size, t_Block_Pointer memory_partition_pos) {
    
    log_trace_r(&MODULE_LOGGER, "## GET POINTER TO MEMORY BLOQUE.dat - Particion: <%u> - Tamaño de Particion: <%zu> Bytes", memory_partition_pos, memory_partition_size);
    return (void *) (((uint8_t *) memory_ptr) + (memory_partition_size * memory_partition_pos)); //uint32 porque por enunciado nos dice 4bytes d etamaño puntero
}

void *get_pointer_to_memory_dump(void * memory_ptr, size_t memory_partition_size, t_Block_Pointer memory_partition_pos) {

    log_trace_r(&MODULE_LOGGER, "## GET POINTER TO MEMORY DUMP - Particion: <%u> - Tamaño de Particion: <%zu> Bytes", memory_partition_pos, memory_partition_size);
    return (void *) (((uint8_t *) memory_ptr) + (memory_partition_size * memory_partition_pos)); //uint32 porque por enunciado nos dice 4bytes d etamaño puntero
}

// array[0]=2 (t_Block_Pointer), arry[1]=0, array[2]=null : bytes: INDICE t_Block_Pointer(4bytes),t_Block_Pointer 


void block_msync(void* get_pointer_to_memory) { // 2
    
    //SINCRONIZO CON EL ARCHIVO
    if(msync(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE, MS_SYNC) == -1) {
        log_error_r(&MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        
    }
}

void create_directory(const char *path) {
    // Crear el directorio con permisos de lectura, escritura y ejecución para el propietario
    if (mkdir(path, 0755) == -1) {
        if (errno == EEXIST) {
            log_info_r(&MODULE_LOGGER, "%s: El directorio ya existe.", path);
        } else {

           log_error_r(&MODULE_LOGGER, "Error al crear el directorio %s: %s", path, strerror(errno));
        }
    } else {
    log_debug_r(&MODULE_LOGGER, "Directorio %s creado exitosamente.", path);
    }
}