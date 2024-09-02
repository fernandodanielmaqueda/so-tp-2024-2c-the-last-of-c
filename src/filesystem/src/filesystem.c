/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "filesystem.h"

char *INTERFACE_NAME;

char *MODULE_NAME = "filesystem";
char *MODULE_LOG_PATHNAME = "filesystem.log";

t_log *MODULE_LOGGER;
t_config *MODULE_CONFIG;

t_Connection CONNECTION_KERNEL;
t_Connection CONNECTION_MEMORY;

int WORK_UNIT_TIME;

char *PATH_BASE_DIALFS;
size_t BLOCK_SIZE;
size_t BLOCK_COUNT;
int COMPRESSION_DELAY;
FILE* FILE_BLOCKS;
FILE* FILE_METADATA;
FILE* FILE_BITMAP;
char* PTRO_BITMAP;
size_t BITMAP_SIZE;

t_list *LIST_FILES;
t_bitarray *BITMAP;
char* PTRO_BLOCKS;
size_t BLOCKS_TOTAL_SIZE;

t_IO_Type IO_TYPES[] = {
    [GENERIC_IO_TYPE] = {.name = "GENERICA", .function = generic_interface_function },
    [STDIN_IO_TYPE] = {.name = "STDIN", .function = stdin_interface_function },
    [STDOUT_IO_TYPE] = {.name = "STDOUT", .function = stdout_interface_function },
    [DIALFS_IO_TYPE] = {.name = "DIALFS", .function = dialfs_interface_function}
};

enum e_IO_Type IO_TYPE;

t_IO_Operation IO_OPERATIONS[] = {
    [IO_GEN_SLEEP_CPU_OPCODE] = {.name = "IO_GEN_SLEEP" , .function = io_gen_sleep_io_operation},
    [IO_STDIN_READ_CPU_OPCODE] = {.name = "IO_STDIN_READ" , .function = io_stdin_read_io_operation},
    [IO_STDOUT_WRITE_CPU_OPCODE] = {.name = "IO_STDOUT_WRITE" , .function = io_stdout_write_io_operation},
    [IO_FS_CREATE_CPU_OPCODE] = {.name = "IO_FS_CREATE" , .function = io_fs_create_io_operation},
    [IO_FS_DELETE_CPU_OPCODE] = {.name = "IO_FS_DELETE" , .function = io_fs_delete_io_operation},
    [IO_FS_TRUNCATE_CPU_OPCODE] = {.name = "IO_FS_TRUNCATE" , .function = io_fs_truncate_io_operation},
    [IO_FS_WRITE_CPU_OPCODE] = {.name = "IO_FS_WRITE" , .function = io_fs_write_io_operation},
    [IO_FS_READ_CPU_OPCODE] = {.name = "IO_FS_READ" , .function = io_fs_read_io_operation},
};

t_PID PID;

int module(int argc, char *argv[]) {

	if(argc != 3) {
		fprintf(stderr, "Uso: %s <NOMBRE_INTERFAZ> <ARCHIVO_DE_CONFIGURACION>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	INTERFACE_NAME = argv[1];

	initialize_configs(argv[2]);
	initialize_loggers();

	initialize_sockets();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	t_Return_Value return_value;
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
	IO_TYPES[IO_TYPE].function();

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

	free_bitmap_blocks();
	//finish_threads();
	finish_sockets();
	//finish_configs();
	finish_loggers();
   
    return EXIT_SUCCESS;
}

void read_module_config(t_config* MODULE_CONFIG) {
    char *io_type_name = config_get_string_value(MODULE_CONFIG, "TIPO_INTERFAZ");
	if(io_type_find(io_type_name, &IO_TYPE)) {
		log_error(MODULE_LOGGER, "%s: No se reconoce el TIPO_INTERFAZ", io_type_name);
		exit(EXIT_FAILURE);
	}

	CONNECTION_KERNEL = (t_Connection) {.client_type = IO_PORT_TYPE, .server_type = KERNEL_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_KERNEL"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_KERNEL")};

	switch(IO_TYPE) {
		case GENERIC_IO_TYPE:
		    WORK_UNIT_TIME = config_get_int_value(MODULE_CONFIG, "TIEMPO_UNIDAD_TRABAJO");
			break;
		case STDIN_IO_TYPE:
		case STDOUT_IO_TYPE:
			CONNECTION_MEMORY = (t_Connection) {.client_type = IO_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
			break;
		case DIALFS_IO_TYPE:
			WORK_UNIT_TIME = config_get_int_value(MODULE_CONFIG, "TIEMPO_UNIDAD_TRABAJO");
			CONNECTION_MEMORY = (t_Connection) {.client_type = IO_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
			PATH_BASE_DIALFS = config_get_string_value(MODULE_CONFIG, "PATH_BASE_DIALFS");
			BLOCK_SIZE = config_get_int_value(MODULE_CONFIG, "BLOCK_SIZE");
			BLOCK_COUNT = config_get_int_value(MODULE_CONFIG, "BLOCK_COUNT");
			COMPRESSION_DELAY = config_get_int_value(MODULE_CONFIG, "RETRASO_COMPACTACION");
			break;
	}

	LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));
}

int io_type_find(char *name, e_IO_Type *destination) {
    if(name == NULL || destination == NULL)
        return 1;
    
    size_t io_types_number = sizeof(IO_TYPES) / sizeof(IO_TYPES[0]);
    for (register e_IO_Type io_type = 0; io_type < io_types_number; io_type++)
        if (strcmp(IO_TYPES[io_type].name, name) == 0) {
            *destination = io_type;
            return 0;
        }

    return 1;
}

void initialize_sockets(void) {
	

	// [Client] Entrada/Salida -> [Server] Kernel
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
}

void finish_sockets(void) {
	close(CONNECTION_KERNEL.fd_connection);
	close(CONNECTION_MEMORY.fd_connection);
}

void generic_interface_function(void) {
	
}

void stdin_interface_function(void) {

}

void stdout_interface_function(void) {

}

void dialfs_interface_function(void) {
	initialize_blocks();
	initialize_bitmap();
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
					list_add(LIST_FILES, new_entry);
				}
			} 
		}
	}
	closedir(dir);

	//int list_len = list_size(LIST_FILES);
}

int io_operation_execute(t_Payload *io_operation) {

	e_CPU_OpCode io_opcode;
	cpu_opcode_deserialize(io_operation, &io_opcode);

    if(IO_OPERATIONS[io_opcode].function == NULL) {
		payload_destroy(io_operation);
        log_warning(MODULE_LOGGER, "Funcion de operacion de IO no encontrada");
        return 1;
    }

    int exit_status = IO_OPERATIONS[io_opcode].function(io_operation);
	payload_destroy(io_operation);
	return exit_status;
}

int io_gen_sleep_io_operation(t_Payload *operation_arguments) {

	if(IO_TYPE != GENERIC_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

	uint32_t work_units;
	payload_remove(operation_arguments, &work_units, sizeof(work_units));

	log_info(MINIMAL_LOGGER, "PID: <%d> - OPERACION <IO_GEN_SLEEP>", (int) PID);

	usleep(WORK_UNIT_TIME * work_units * 1000);

    return 0;
}

int io_stdin_read_io_operation(t_Payload *operation_arguments) {

	if(IO_TYPE != STDIN_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

	t_list *physical_addresses = list_create();
	size_t bytes;

	//Empiezo a "desencolar" el payload recibido
	
	size_deserialize(operation_arguments, &bytes);
	list_deserialize(operation_arguments, physical_addresses, size_deserialize_element);

	char text_to_send[bytes + 1]; // Le agrego 1 para el '\0' por las dudas
	char* pointer_verifier;
	int char_to_verify = '\n';

	//Aviso que operacion voy a hacer
	log_info(MINIMAL_LOGGER, "PID: <%d> - OPERACION <IO_STDIN_READ>", (int) PID);

	log_info(MODULE_LOGGER, "Escriba una cadena de %d caracteres", (int) bytes);
	fgets(text_to_send, bytes + 1, stdin);
	
	// while(strlen(text_to_send) != bytes){
		// log_info("Escriba una cadena de %d caracteres", (int)bytes);
		// fgets(text_to_send, bytes + 1, stdin);		
	// }

	pointer_verifier = strchr(text_to_send, char_to_verify);

	while(pointer_verifier != NULL){
		log_info(MODULE_LOGGER, "Escriba una cadena de %d caracteres", (int) bytes);
		fgets(text_to_send, bytes + 1, stdin);
		pointer_verifier = strchr(text_to_send, char_to_verify);
	}

	//char text_to_send[bytes + 1] = "WAR NEVER CHANGES...";

	log_info(MODULE_LOGGER, "[IO] Mensaje escrito: <%s>", text_to_send); // FALTA EL \0

	//Creo paquete y argumentos necesarios para enviarle a memoria
	t_Package *package = package_create_with_header(IO_STDIN_WRITE_MEMORY_HEADER);
	payload_add(&(package->payload), &PID, sizeof(PID));
	list_serialize(&(package->payload), *physical_addresses, size_serialize_element);
	size_serialize(&(package->payload), bytes);
	payload_add(&(package->payload), text_to_send, (size_t) bytes);
	package_send(package, CONNECTION_MEMORY.fd_connection);
	package_destroy(package);

	//Recibo si salio bien la operacion
	receive_return_value_with_expected_header(WRITE_REQUEST_HEADER, 0, CONNECTION_MEMORY.fd_connection);

	// free(pointer_verifier);
	
	return 0;
}

int io_stdout_write_io_operation(t_Payload *operation_arguments) {

	if(IO_TYPE != STDOUT_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}
			
	t_list *physical_addresses = list_create();
	size_t bytes;

	//text_deserialize(operation_arguments, &text_received);
	size_deserialize(operation_arguments, &bytes);
	list_deserialize(operation_arguments, physical_addresses, size_deserialize_element);

	log_info(MINIMAL_LOGGER, "PID: <%d> - OPERACION <IO_STDOUT_WRITE>", (int) PID);

	t_Package* package;

	//Creo header para memoria y el paquete con los argumentos
	package = package_create_with_header(IO_STDOUT_READ_MEMORY_HEADER);
	payload_add(&(package->payload), &PID, sizeof(PID));
	list_serialize(&(package->payload), *physical_addresses, size_serialize_element);
	size_serialize(&(package->payload), bytes);
	package_send(package, CONNECTION_MEMORY.fd_connection);
	package_destroy(package);
	
	char text_received[bytes + 1]; // Agrego 1 para el '\0'

	package_receive(&package, CONNECTION_MEMORY.fd_connection);
	payload_remove(&(package->payload), text_received, (size_t) bytes);

	text_received[bytes] = '\0';

	log_info(MODULE_LOGGER, "[IO] Mensaje leido: <%s>", text_received);

	//fputs(text_received, stdout);
    
    return 0;
}

int io_fs_create_io_operation(t_Payload *operation_arguments) {

	if(IO_TYPE != DIALFS_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

    log_trace(MODULE_LOGGER, "[FS] Pedido del tipo IO_FS_CREATE recibido.");
	char* file_name;

	usleep(WORK_UNIT_TIME * 1000);
    text_deserialize(operation_arguments, &(file_name));
	uint32_t location = seek_first_free_block();

	log_info(MINIMAL_LOGGER, "PID: <%d> - Crear archivo: <%s>", PID, file_name);
	//Crear variable de control de archivo
	t_FS_File* new_entry = malloc(sizeof(t_FS_File));
	new_entry->name = malloc(sizeof(file_name));
	strcpy(new_entry->name , file_name);
//	new_entry->process_pid = PID;
	new_entry->initial_bloq = location;
	new_entry->len = 1;
	new_entry->size = 0;

	create_file(file_name, location); //Creo archivo y lo seteo
	bitarray_set_bit(BITMAP, location);

	list_add(LIST_FILES, new_entry);
	
    return 0;
}

int io_fs_delete_io_operation(t_Payload *operation_arguments) {

	if(IO_TYPE != DIALFS_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

	char* file_name = NULL;

	usleep(WORK_UNIT_TIME * 1000);
    text_deserialize(operation_arguments, &(file_name));

    log_info(MINIMAL_LOGGER, "PID: <%d> - Eliminar archivo: <%s>", PID, file_name);
	
	uint32_t size = list_size(LIST_FILES);

	if(size > 0){
		t_FS_File* file = list_get(LIST_FILES,0);
		size_t file_target = -1;

		for (size_t i = 0; i < size; i++) //busqueda del file indicado
		{
			t_FS_File* file = list_get(LIST_FILES,i);
			if (strcmp(file->name, file_name)){
				file_target = i;
				i = size;
			}
		}

		if(file_target == -1) {
			log_info(MODULE_LOGGER, "[ERROR] PID: <%d> - Archivo <%s> a eliminar no encontrado", PID, file_name);
			return 1;
		}

	log_info(MODULE_LOGGER, "El len del archivo es %d", file->len);
		//Liberacion del bitarray
		uint32_t initial_pos = file->initial_bloq;
		for (size_t i = 1; i <= file->len; i++)
		{
			bitarray_clean_bit(BITMAP, initial_pos);
			initial_pos++;
		}

		if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        	log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        	exit(EXIT_FAILURE);
    	}

		list_remove(LIST_FILES, file_target);
		//free(file_target);
		//update_file(file_name,0,-1);b
	}
	

	char* path_file = string_new();
	strcpy (path_file, PATH_BASE_DIALFS);
	string_append(&path_file, "/");
	string_append(&path_file, file_name);
	remove(path_file);

	free(path_file);
    return 0;
}

int io_fs_truncate_io_operation(t_Payload *operation_arguments) {

	if(IO_TYPE != DIALFS_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

    log_trace(MODULE_LOGGER, "[FS] Pedido del tipo IO_FS_TRUNCATE recibido.");

	char *file_name;
	size_t value;
	
	usleep(WORK_UNIT_TIME * 1000);
    text_deserialize(operation_arguments, &(file_name));
    size_deserialize(operation_arguments, &value);

	size_t valueNUM = (size_t) ceil((double)value / (double)BLOCK_SIZE);

	t_FS_File* file = seek_file(file_name);
	size_t initial_pos = file->initial_bloq + file->len;
	if(file->len > valueNUM)
	{//Se restan bloques
			log_debug(MODULE_LOGGER, "SE RESTAN BLOQUES");
		size_t diff = file->len - valueNUM;
		for (size_t i = 0; i < diff; i++)
		{
			bitarray_clean_bit(BITMAP, initial_pos);
			initial_pos--;
		}
		file->len = valueNUM;
		file->size = value;
		
		update_file(file_name, value, file->initial_bloq);
	}
	if (file->len < valueNUM)
	{// Se agregan bloques
		size_t diff = valueNUM - file->len;
		if(can_assign_block(file->initial_bloq, file->len, valueNUM)){
			log_debug(MODULE_LOGGER, "SE SUMAN BLOQUES");

			for (size_t i = 0; i < diff; i++)
			{
				bitarray_set_bit(BITMAP, initial_pos);
				initial_pos++;
			}
			file->len = valueNUM;
			file->size = value;
		update_file(file_name, value, file->initial_bloq);

		}
		else if(quantity_free_blocks() >= valueNUM){//VERIFICA SI COMPACTAR SOLUCIONA EL PROBLEMA

			log_info(MINIMAL_LOGGER, "PID: <%d> - Inicio Compactacion", PID);
			compact_blocks(file, valueNUM, value);
			log_info(MINIMAL_LOGGER, "PID: <%d> - Fin Compactacion", PID);

/*			//initial_pos = file->initial_bloq + file->len;
 			for (size_t i = 0; i < diff; i++)
			{
				bitarray_set_bit(BITMAP, initial_pos);
				initial_pos++;
			}
			file->len = valueNUM;
			file->size = value;
 */
		}
		else{
        	log_error(MODULE_LOGGER, "[FS] ERROR: OUT_OF_MEMORY --> Can't assing blocks");
			return 1;
		}
	}

	
		if (msync(PTRO_BITMAP, BITMAP_SIZE, MS_SYNC) == -1) {
        	log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        	exit(EXIT_FAILURE);
    	}
	
    log_info(MINIMAL_LOGGER, "PID: <%d> - Truncar archivo: <%s> - Tamaño: <%d>", PID, file_name, (int) value);
	
/* 	t_Package* respond = package_create_with_header(IO_FS_TRUNCATE_CPU_OPCODE);
	payload_add(respond->payload, &PID, sizeof(t_PID));
	package_send(respond, CONNECTION_KERNEL.fd_connection);
	package_destroy(respond); */

    return 0;
}

int io_fs_write_io_operation(t_Payload *operation_arguments) {

/*O_FS_WRITE (Interfaz, Nombre Archivo, Registro Dirección, Registro Tamaño, Registro
Puntero Archivo): Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se
lea desde Memoria la cantidad de bytes indicadas por el Registro Tamaño a partir de la
dirección lógica que se encuentra en el Registro Dirección y se escriban en el archivo a partir
del valor del Registro Puntero Archivo.*/

	if(IO_TYPE != DIALFS_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

    log_trace(MODULE_LOGGER, "[FS] Pedido del tipo IO_FS_WRITE recibido.");

	char *file_name = NULL;
	size_t ptro;
	size_t bytes;
	t_list* list_dfs = list_create();

	usleep(WORK_UNIT_TIME * 1000);
	//Leo el payload recibido de kernel
    text_deserialize(operation_arguments, &file_name);
    size_deserialize(operation_arguments, &ptro);
    size_deserialize(operation_arguments, &bytes);
	list_deserialize(operation_arguments, list_dfs, size_deserialize_element);

	
	//Busco el file buscado
	t_FS_File* file = seek_file(file_name);
	//size_t block_initial = (size_t) floor((double) ptro / (double) BLOCK_SIZE);
	uint32_t block_initial = file->initial_bloq;

	//Envio paquete a memoria
	t_Package* pack_request = package_create_with_header(IO_FS_WRITE_MEMORY_HEADER);
	payload_add(&pack_request->payload, &PID, sizeof(PID));
    list_serialize(&pack_request->payload, *list_dfs, size_serialize_element);
	size_serialize(&pack_request->payload, bytes);
	package_send(pack_request,CONNECTION_MEMORY.fd_connection);
	package_destroy(pack_request);

	//Recibo respuesta de memoria con el contenido
	t_Package* package_memory;
	package_receive(&package_memory, CONNECTION_MEMORY.fd_connection);
	void *posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + (block_initial * BLOCK_SIZE + ptro));

    payload_remove(&package_memory->payload, posicion, (size_t) bytes);
	package_destroy(package_memory); 


	log_info(MINIMAL_LOGGER, "PID: <%d> - Escribir Archivo: <%s> - Tamaño a Escribir: <%d> - Puntero Archivo: <%d>",
				 (int) PID, file_name, (int)bytes, (int)ptro);

	
    if (msync(PTRO_BLOCKS, BLOCKS_TOTAL_SIZE, MS_SYNC) == -1) {
        log_error(MODULE_LOGGER, "Error al sincronizar los cambios en bloques.dat con el archivo: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
	
    return 0;
}

int io_fs_read_io_operation(t_Payload *operation_arguments) {

	/*O_FS_READ (Interfaz, Nombre Archivo, Registro Dirección, Registro Tamaño, Registro
Puntero Archivo): Esta instrucción solicita al Kernel que mediante la interfaz seleccionada, se
lea desde el archivo a partir del valor del Registro Puntero Archivo la cantidad de bytes
indicada por Registro Tamaño y se escriban en la Memoria a partir de la dirección lógica
indicada en el Registro Dirección*/

	if(IO_TYPE != DIALFS_IO_TYPE) {
		log_info(MODULE_LOGGER, "No puedo realizar esta instruccion");
		return 1;
	}

    log_trace(MODULE_LOGGER, "[FS] Pedido del tipo IO_FS_READ recibido.");

	char* file_name = NULL;
	size_t ptro;
	size_t bytes;
	t_list* list_dfs = list_create();

	usleep(WORK_UNIT_TIME * 1000);
	//Leo el payload recibido
    text_deserialize(operation_arguments, &(file_name));
    size_deserialize(operation_arguments, &ptro);
    size_deserialize(operation_arguments, &bytes);
	list_deserialize(operation_arguments, list_dfs, size_deserialize_element);

	//Busco el file buscado
	t_FS_File* file = seek_file(file_name);
	//size_t block_initial = (size_t) floor((double) ptro / (double) BLOCK_SIZE);
	size_t block_initial = file->initial_bloq;
    //size_t offset = (size_t) (ptro - block_initial * BLOCK_SIZE);;
	//size_t block_quantity_required = seek_quantity_blocks_required(ptro, bytes);

    void* context = malloc(bytes);
	void *posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + (block_initial * BLOCK_SIZE + ptro));
	//void *posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + (ptro));
    memcpy(context, posicion, bytes); 
/* 	posicion = (void *)(((uint8_t *) PTRO_BLOCKS) + ((location - free_spaces) * BLOCK_SIZE));
    memcpy(posicion, context, size); 
*/

	//Se crea paquete y se envia a memoria
	t_Package* pack_request = package_create_with_header(IO_FS_READ_MEMORY_HEADER);
	payload_add(&pack_request->payload, &PID, sizeof(t_PID));
    list_serialize(&pack_request->payload, *list_dfs, size_serialize_element);
	payload_add(&pack_request->payload, &bytes, sizeof(bytes));
	payload_add(&pack_request->payload, context, bytes);
	package_send(pack_request,CONNECTION_MEMORY.fd_connection);
	package_destroy(pack_request);

	free(context);

	log_info(MINIMAL_LOGGER, "PID: <%d> - Leer Archivo: <%s> - Tamaño a Leer: <%d> - Puntero Archivo: <%d>",
				 (int) PID, file_name, (int)bytes, (int)ptro);

	if(receive_return_value_with_expected_header(WRITE_REQUEST_HEADER,0,CONNECTION_MEMORY.fd_connection)){
		
        exit(1);
	}

/*     if(send_return_value_with_header(WRITE_REQUEST, 0, CONNECTION_KERNEL.fd_connection)) {
        // TODO
        exit(1);
    } */

    return 0;
}

void initialize_blocks() {
    BLOCKS_TOTAL_SIZE = BLOCK_SIZE * BLOCK_COUNT;
    //size_t path_len_bloqs = strlen(PATH_BASE_DIALFS) + 1 +strlen("bitmap.dat"); //1 por la '/'
	char* path_file_blocks = string_new();
	strcpy (path_file_blocks, PATH_BASE_DIALFS);
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
	strcpy (path_file_bitmap, PATH_BASE_DIALFS);
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

    PTRO_BITMAP = mmap(NULL, BITMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (PTRO_BITMAP == MAP_FAILED) {
        log_error(MODULE_LOGGER, "Error al mapear el archivo bitmap.dat a memoria: %s", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

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
	strcpy (path_file, PATH_BASE_DIALFS);
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
	strcpy (path_file, PATH_BASE_DIALFS);
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

	for (size_t i = 0; i < list_size(LIST_FILES); i++)
	{
		magic = list_get(LIST_FILES,i);
		if (magic->initial_bloq == position) return magic;
	}

	return magic;
}

void compact_blocks(t_FS_File* file, size_t nuevoLen, size_t nuevoSize){
	usleep(COMPRESSION_DELAY * 1000);
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