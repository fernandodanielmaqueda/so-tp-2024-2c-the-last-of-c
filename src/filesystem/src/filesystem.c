/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "filesystem.h"

char *MODULE_NAME = "filesystem";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "filesystem.log";

t_config *MODULE_CONFIG;
char *MODULE_CONFIG_PATHNAME = "filesystem.config";

t_Server SERVER_FILESYSTEM;

t_Shared_List SHARED_LIST_CLIENTS_MEMORY;

char *MOUNT_DIR;
size_t BLOCK_SIZE;
size_t BLOCK_COUNT;
int BLOCK_ACCESS_DELAY;

FILE *FILE_BLOCKS;
FILE *FILE_METADATA;
char *PTRO_BITMAP;
size_t BITMAP_SIZE;

t_bitarray *BITMAP;
char *PTRO_BLOCKS;
size_t BLOCKS_TOTAL_SIZE;

int module(int argc, char *argv[]) {

	initialize_configs(MODULE_CONFIG_PATHNAME);
	initialize_loggers();
	initialize_global_variables();

	initialize_sockets();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	//t_Return_Value return_value;
	/* 
	if(receive_expected_header(INTERFACE_DATA_REQUEST_HEADER, CONNECTION_KERNEL.fd_connection)) {
		// TODO
		exit(1);
	}	
	if(send_interface_data(INTERFACE_NAME, IO_TYPE, CONNECTION_KERNEL.fd_connection)) {
		// TODO
		exit(1);
	}
	if(receive_return_value_with_expected_header(INTERFACE_DATA_REQUEST_HEADER, &return_value, CONNECTION_KERNEL.fd_connection)) {
		// TODO
		exit(1);
	}
	
	if(return_value) {
		log_error(MODULE_LOGGER, "No se pudo registrar la interfaz %s en el Kernel", INTERFACE_NAME);
		exit(EXIT_FAILURE);
	}


	// Invoco a la función que corresponda
	//IO_TYPES[IO_TYPE].function();

	t_Payload io_operation;
	//escuchar peticion siempre
	while(1) {
		if(receive_io_operation_dispatch(&PID, &io_operation, CONNECTION_KERNEL.fd_connection)) {
			exit(1);
		}

		t_Return_Value return_value = (t_Return_Value) io_operation_execute(&io_operation);

		if(send_io_operation_finished(PID, return_value, CONNECTION_KERNEL.fd_connection)) {
			exit(1);
		}
	}
	*/
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

void initialize_sockets(void) {

	// [Client] Entrada/Salida -> [Server] Kernel
	/* 
	pthread_t thread_io_connect_to_kernel;
	pthread_create(&thread_io_connect_to_kernel, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_KERNEL);
	switch(IO_TYPE) {
		case GENERIC_IO_TYPE:
			break;
		case STDIN_IO_TYPE:
		case STDOUT_IO_TYPE:
		case DIALFS_IO_TYPE:
		{
			// [Client] Entrada/Salida -> [Server] Memoria
			pthread_t thread_io_connect_to_memory;
			pthread_create(&thread_io_connect_to_memory, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_MEMORY);
			pthread_join(thread_io_connect_to_memory, NULL);
			break;
		}
	}	
	pthread_join(thread_io_connect_to_kernel, NULL);
	*/
}

void finish_sockets(void) {
	//close(CONNECTION_KERNEL.fd_connection);
	//close(CONNECTION_MEMORY.fd_connection);
}

void dialfs_interface_function(void) {
	initialize_blocks();
	initialize_bitmap();
	
	/*
	LIST_FILES = list_create();
	struct dirent* entrada;
	DIR *dir = opendir(PATH_BASE_DIALFS); 
	if (dir != NULL) {
		while((entrada = readdir(dir)) != NULL) {
			if (entrada->d_type == DT_REG) {
				const char *ext = strrchr(entrada->d_name, '.');
				if(ext != NULL && strcmp(ext, ".txt") == 0){
					log_info(MODULE_LOGGER, "%s/%s\n", PATH_BASE_DIALFS, entrada->d_name);
					t_FS_File* new_entry = malloc(sizeof(t_FS_File));
					new_entry->name = malloc(sizeof(entrada->d_name));
					strcpy(new_entry->name , entrada->d_name);	
					char* file_to_get = malloc(strlen(PATH_BASE_DIALFS)+ 1 + strlen(entrada->d_name));
					strcpy(file_to_get,PATH_BASE_DIALFS);
					strcat(file_to_get,"/");
					strcat(file_to_get,entrada->d_name);

					t_config* data = config_create(file_to_get);
					new_entry->initial_bloq = config_get_int_value(data, "BLOQUE_INICIAL");
					new_entry->size = config_get_int_value(data, "TAMAÑO_ARCHIVO");
					if(new_entry->size == 0){
						new_entry->len = 1;
					}else{
						new_entry->len = ceil((double)new_entry->size / (double)BLOCK_SIZE);
					}
					
					//new_entry->process_pid = 0; 
					//list_add(LIST_FILES, new_entry);
				}
			} 
		}
	}
	closedir(dir);
	*/

	//int list_len = list_size(LIST_FILES);
}


void initialize_blocks() {
    BLOCKS_TOTAL_SIZE = BLOCK_SIZE * BLOCK_COUNT;
    //size_t path_len_bloqs = strlen(PATH_BASE_DIALFS) + 1 +strlen("bitmap.dat"); //1 por la '/'
	char* path_file_blocks = string_new();
	//strcpy(path_file_blocks, PATH_BASE_DIALFS);
	string_append(&path_file_blocks, "/");
	string_append(&path_file_blocks, "bloques.dat");

	//Checkeo si el file ya esta creado, sino lo elimino
	//if (access(path_file_blocks, F_OK) == 0)remove(path_file_blocks);

    int fd = open(path_file_blocks, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_error(MODULE_LOGGER, "Error al abrir el archivo bloques.dat: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, BLOCKS_TOTAL_SIZE) == -1) {
        log_error(MODULE_LOGGER, "Error al ajustar el tamaño del archivo bloques.dat: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    PTRO_BLOCKS = mmap(NULL, BLOCKS_TOTAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (PTRO_BLOCKS == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bloques.dat a memoria: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (msync(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
/*
    if (munmap(BLOCKS_DATA, BLOCKS_TOTAL_SIZE) == -1) {
        log_error(MODULE_LOGGER, "Error al desmapear el archivo bloques.dat de la memoria: %s", strerror(errno));
    }
*/
    log_info(MODULE_LOGGER, "Bloques creados y mapeados correctamente.");
}


void initialize_bitmap() {
	BITMAP_SIZE = (size_t) ceil((double) BLOCK_COUNT / 8);
	
    //size_t path_len_bloqs = strlen(PATH_BASE_DIALFS) + 1 +strlen("bitmap.dat"); //1 por la '/'
	char* path_file_bitmap = string_new();
	//strcpy(path_file_bitmap, PATH_BASE_DIALFS);
	string_append(&path_file_bitmap, "/");
	string_append(&path_file_bitmap, "bitmap.dat");

	//Checkeo si el file ya esta creado, sino lo elimino
	
    int fd = open(path_file_bitmap, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_error(MODULE_LOGGER, "Error al abrir el archivo bitmap.dat: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, BITMAP_SIZE) == -1) {
        log_error(MODULE_LOGGER, "Error al ajustar el tamaño del archivo bitmap.dat: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

	/*
    PTRO_BITMAP = mmap(NULL, BITMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (PTRO_BITMAP == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bitmap.dat a memoria: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }
	*/

    BITMAP = bitarray_create_with_mode((char *)PTRO_BITMAP, BITMAP_SIZE,LSB_FIRST);
    if (BITMAP == NULL) {
        log_error(MODULE_LOGGER, "Error al crear la estructura del bitmap");
        munmap(PTRO_BITMAP, BITMAP_SIZE);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }
/*
    if (munmap(PTRO_BITMAP, BITMAP_SIZE) == -1) {
        log_error(MODULE_LOGGER, "Error al desmapear el archivo bitmap.dat de la memoria: %s", strerror(errno));
    }
*/
    log_info(MODULE_LOGGER, "Bitmap creado y mapeado correctamente.");
}

uint32_t seek_first_free_block(){
	int magic = 0;

	for (size_t i = 0; i < (BLOCK_COUNT +1); i++)
	{
		if(!(bitarray_test_bit(BITMAP, i))){
			magic = i;
			i = BLOCK_COUNT;
		}
	}

	return magic;
}

/*
t_FS_File* seek_file(char* file_name){

    t_FS_File* found_file = NULL;

    for (size_t i = 0; i < list_size(LIST_FILES); i++) {
        t_FS_File* file = list_get(LIST_FILES, i);
        
        if (strcmp(file->name, file_name) == 0) {
            found_file = file;  
            break;  
        }
    }
    
    return found_file;
}
*/

bool can_assign_block(size_t initial_position, size_t len, size_t final_len){
	size_t addition = final_len - len;
	size_t final_pos = initial_position + len + addition;

	for (size_t i = (initial_position + len); i < final_pos; i++)
	{
		if(	bitarray_test_bit (BITMAP, i)) return false;
	}

	return true;
}

int quantity_free_blocks(){
	int magic = 0;

	if(BLOCK_COUNT == 0) return magic;

	for (size_t i = 0; i < BLOCK_COUNT; i++)
	{
		if(	!(bitarray_test_bit (BITMAP, i))) magic++;
	}

	return magic;
}

size_t seek_quantity_blocks_required(size_t puntero, size_t bytes){
    size_t quantity_blocks = 0;

    size_t nro_page = (size_t) floor((double) puntero / (double) BLOCK_SIZE);
    size_t offset = (size_t) (puntero - nro_page * BLOCK_SIZE);;

    if (offset != 0)
    {
        bytes -= (BLOCK_SIZE - offset);
        quantity_blocks++;
    }

    quantity_blocks += (size_t) floor((double) bytes / (double) BLOCK_SIZE);
    
    return quantity_blocks;
}


void free_bitmap_blocks(void){
	
    if (munmap(PTRO_BLOCKS, (BLOCK_SIZE * BLOCK_COUNT)) == -1) {
        log_error(MODULE_LOGGER, "Error al desmapear el archivo bloques.dat de la memoria: %s", strerror(errno));
    }
    if (munmap(PTRO_BITMAP, BITMAP_SIZE) == -1) {
        log_error(MODULE_LOGGER, "Error al desmapear el archivo bitmap.dat de la memoria: %s", strerror(errno));
    }

	free(BITMAP);

}
/*
void create_blocks(){
    size_t path_len_bloqs = strlen(PATH_BASE_DIALFS) + 1 +strlen("bloques.dat"); //1 por la '/'
	char* path_file_bloqs = string_new();
	strcpy (path_file_bloqs, PATH_BASE_DIALFS);
	string_append(&path_file_bloqs, "/");
	string_append(&path_file_bloqs, "bloques.dat");

	//Checkeo si el file ya esta creado, sino lo elimino
	if (access(path_file_bloqs, F_OK) == 0)remove(path_file_bloqs);

	FILE_BLOCKS = fopen(path_file_bloqs, "w+"); 
	
    if (FILE_BLOCKS == -1)
    {
        log_error(MODULE_LOGGER, "Error al abrir el archivo bloques.dat");
        exit(EXIT_FAILURE);
    }
}
*/
void create_file(char* file_name, size_t initial_block){
    //size_t path_len = strlen(PATH_BASE_DIALFS) + 1 +strlen(file_name); //1 por la '/'
	char* path_file = string_new();
	//strcpy(path_file, PATH_BASE_DIALFS);
	string_append(&path_file, "/");
	string_append(&path_file, file_name);

	//Checkeo si el file ya esta creado, sino lo elimino
	if (access(path_file, F_OK) == 0)remove(path_file);
 
	FILE_METADATA = fopen(path_file, "wb");
    if (FILE_METADATA == NULL) {
        log_error(MODULE_LOGGER, "Error al abrir el archivo %s", file_name);
        exit(EXIT_FAILURE);
    }

	t_config* config_temp = config_create(path_file);
    config_set_value(config_temp, "BLOQUE_INICIAL", string_itoa(initial_block));
    config_set_value(config_temp, "TAMAÑO_ARCHIVO", "0");
	config_save_in_file(config_temp,path_file);
	config_destroy(config_temp);
		
	free(path_file);
	fclose(FILE_METADATA);
}

void update_file(char* file_name, size_t size, size_t location){
	char* path_file = string_new();
	//strcpy(path_file, PATH_BASE_DIALFS);
	string_append(&path_file, "/");
	string_append(&path_file, file_name);

	t_config* config_temp = config_create(path_file);
    config_set_value(config_temp, "BLOQUE_INICIAL", string_itoa(location));
    config_set_value(config_temp, "TAMAÑO_ARCHIVO", string_itoa(size));
	config_save_in_file(config_temp,path_file);
	config_destroy(config_temp);

	free(path_file);
}


t_FS_File* seek_file_by_header_index(size_t position){

	t_FS_File* magic = NULL;

	/*
	for (size_t i = 0; i < list_size(LIST_FILES); i++)
	{
		magic = list_get(LIST_FILES,i);
		if (magic->initial_bloq == position) return magic;
	}
	*/

	return magic;
}

void compact_blocks(t_FS_File* file, size_t nuevoLen, size_t nuevoSize){
	//usleep(COMPRESSION_DELAY * 1000);
	int total_free_spaces = 0;
	int len = 0;
	void* aux_memory = malloc(file->len * BLOCK_SIZE);
	void *posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + (file->initial_bloq * BLOCK_SIZE));

			for (uint32_t i = 0; i < BLOCK_COUNT; i++)
			{
				if (!(bitarray_test_bit(BITMAP,i)))//Cuento los espacios vacios
				{
					total_free_spaces++;
				}
				else if(file->initial_bloq == i) //Copio la memoria del archivo a agrandar a un aux 
				{
					memcpy(aux_memory, posicion, (file->len * BLOCK_SIZE)); 
					int pos = i;

					for (size_t q = 0; q < file->len; q++)
					{
						bitarray_clean_bit(BITMAP,pos);
						pos++;
					}
					i += file->len -1;
					total_free_spaces++;

				}
				else if(file->initial_bloq != i){//BITMAP no libre, no es el archivo en cuestion
					t_FS_File* temp_entry = seek_file_by_header_index(i);
					len = temp_entry->len;
					if (total_free_spaces != 0){//Mueve el bloque y actualiza el bitmap
						moveBlock(temp_entry->len, total_free_spaces, i);
						temp_entry->initial_bloq = i - total_free_spaces;
						update_file(temp_entry->name, temp_entry->size, i - total_free_spaces);
					}
					i+=len -1; //Salteo los casos ya contemplados en moveBlock

				}
				
			}
		
		//Actualizo el proceso copiado
		size_t new_pos = BLOCK_COUNT - total_free_spaces;
		file->initial_bloq = new_pos;
		file->len = nuevoLen;
		file->size = nuevoSize;
		posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + (new_pos * BLOCK_SIZE));
		memcpy(posicion, aux_memory, (file->len * BLOCK_SIZE)); 
		update_file(file->name, file->size, new_pos);
					for (size_t r = 0; r < file->len; r++)
					{
						bitarray_set_bit(BITMAP,new_pos);
						new_pos++;
					}
		free(aux_memory);
		
			
    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }
	
    if (msync(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

}

void moveBlock(size_t blocks_to_move, size_t free_spaces, size_t location){
	//Mueve el bloque y actualiza el bitmap
	size_t size = blocks_to_move * BLOCK_SIZE;
    void* context = malloc(size);
	void *posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + (location * BLOCK_SIZE));
    memcpy(context, posicion, size); 
	posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + ((location - free_spaces) * BLOCK_SIZE));
    memcpy(posicion, context, size); 

	for (size_t i = 0; i < blocks_to_move; i++)
	{
		bitarray_clean_bit(BITMAP,(location + i));
		bitarray_set_bit(BITMAP,(location + i - free_spaces));
	}

	free(context);
	
}