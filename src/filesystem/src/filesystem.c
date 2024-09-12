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

t_bitarray *BITMAP;
char *PTRO_BLOCKS;
size_t BLOCKS_TOTAL_SIZE;

int module(int argc, char *argv[]) {

	initialize_configs(MODULE_CONFIG_PATHNAME);
	initialize_loggers();
	initialize_global_variables();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	//(ok) crea el archivo bitmap.dat, lo mapea a memoria ppal, lo agrega a la estructura bitarray (ppal)
    // return t_bitarray *BITMAP;
	initialize_bitmap(); 


	// Inicializar cada bit del bitmap en 0
	for (int bit_index = 0; i < bitarray_get_max_bit(BITMAP); i++) {
		// Limpia el bit, lo pone en 0
		int bit = (int) bitarray_test_bit(BITMAP, bit_index);
		
	}

	initialize_sockets();

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

void *filesystem_client_handler_for_memory(t_Client *new_client) {

	log_trace(MODULE_LOGGER, "Hilo receptor de [Cliente] Memoria [%d] iniciado", new_client->fd_client);

	// Borrar este while(1) (ciclo incluido) y reemplazarlo por la lógica necesaria para atender al cliente

	/*
		1-Validar si existe el archivo bitmap.dat (considerar la concurrencia) si no esta crearlo -->initialize_bitmap
		----------------------------------------------------------------------------
		2-Todos los bloques se inicialzian en 0.. reocrro el bitmap y le pongo 0 a todos los bloques
		
		TODO:CORREGIR ESTO******************
	for (int i = 0; i < bitarray_get_max_bit(bitarray); i++) {
        
		//aplico el  bitarray_set_bit(t_bitarray*, off_t bit_index);
			bitarray_set_bit(bitarray, i); //  El bit está limpio (0)
		

    }
		---------------------------------------------------------------------------------
		3-Uso semaforo para sincronziar el puntero de bitmap y proteger la zona critica 
		4-Recorro el array del bitmap y busco si tengo posiciones libres (en 0) .. 
				ejemplo---> tengo un bloque de 4 bloques libres (0,1,2,3) y entonces reservo un bloque mas para el de indices.. total = 5 bloques
		 
		
	*/




	while(1) {

	

		}

	close(new_client->fd_client);

	return NULL;
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

// ok
void initialize_bitmap() {

	// cantidad bytes = cantidad de bloques sobre 8.
	BITMAP_SIZE = (size_t) ceil((double) BLOCK_COUNT / 8);
	
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
    BITMAP = bitarray_create_with_mode((char *)PTRO_BITMAP, BITMAP_SIZE,LSB_FIRST);
    if (BITMAP == NULL) {
        log_error(MODULE_LOGGER, "Error al crear la estructura del bitmap");
        munmap(PTRO_BITMAP, BITMAP_SIZE);//liberar la memoria reservada
        close(fd);
        exit(EXIT_FAILURE);
    }

	// Inicializar cada bit del bitmap en 0
	for (int bit_index = 0; i < bitarray_get_max_bit(BITMAP); i++) {
		// Limpia el bit, lo pone en 0
		bitarray_clean_bit( BITMAP, bit_index);
	}

	// Forzamos que los cambios en momoria ppal se reflejen en el archivo.
	// vamos a trabajar siempre en memoria ppal?? si: no hace falta sicronizar siempre.
    if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bitmap.dat con el archivo: %s", strerror(errno));
    }

    log_info(MODULE_LOGGER, "Bitmap creado y mapeado correctamente.");
}

void *reserve_blocks(){

	t_list* list_aux_blocks = list_create();
	size_t block_count_total = BLOCK_COUNT +1; //sumo 1 del bloque indexado
	


	for (int i = 0; i < bitarray_get_max_bit(BITMAP); i++) {
        if (!bitarray_test_bit(BITMAP, i)) { 
          //  El bit está limpio (0)

    }
	
	}
/* 
  // t_list pointer_array_size = (BLOCK_COUNT+1) * sizeof(void*);
  // void** pointer_array = (void**) malloc(pointer_array_size);
    if (pointer_array == NULL) {
        log_error(MODULE_LOGGER, "Error al crear el array auxiliar de punteros");
        munmap(PTRO_BITMAP, BITMAP_SIZE);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Inicializar los punteros en el array auxiliar a NULL
    for (size_t i = 0; i < BLOCK_COUNT; i++) {
        pointer_array[i] = NULL;
    }

    log_info(MODULE_LOGGER, "Bitmap creado y mapeado correctamente.");
    log_info(MODULE_LOGGER, "Array auxiliar de punteros creado e inicializado correctamente.");

	  free(pointer_array);





*/


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
	/*
    if (munmap(PTRO_BLOCKS, (BLOCK_SIZE * BLOCK_COUNT)) == -1) {
        log_error(MODULE_LOGGER, "Error al desmapear el archivo bloques.dat de la memoria: %s", strerror(errno));
    }
    if (munmap(PTRO_BITMAP, BITMAP_SIZE) == -1) {
        log_error(MODULE_LOGGER, "Error al desmapear el archivo bitmap.dat de la memoria: %s", strerror(errno));
    }

	free(BITMAP);
	*/
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