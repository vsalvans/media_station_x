#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "scan.h"
#include "request.h"
#include "config.h"
#include "logger.h"

#define BUFFER_SIZE 1024
#define JSON_MAX_SIZE 102400
#define RESPONSE_MAX_SIZE 2097152
#define CONFIG_FILE "media.cnf"

struct media_config * CONFIG;
int SERVER_CONNECTION;

void start_server(uint16_t port, int * p_connection);
void sigint_handler(int sig);

int main (int argc, char * args[]) {
	
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, SIG_IGN);
	
	init_logger(INFO);

  char * config_file = CONFIG_FILE;

  if (argc > 1) {
    config_file = args[1];
  }

  printf("Reading config file from %s", config_file);
 	
  CONFIG = read_config(config_file);
	
	if (CONFIG == NULL) {
		perror("Error while reading config file");
		exit(EXIT_FAILURE);
	}

	log_info("Media title: %s", CONFIG->title);
	log_info("Media host: %s", CONFIG->host);
	log_info("Media port: %i", CONFIG->port);
	log_info("Media buffer: %i", CONFIG->media_buffer);

	for(int i; i < CONFIG->locations_len; i++) {
		struct media_location * location = CONFIG->locations[i];
		log_info("Location slug %s named as %s", location->slug, location->name);
	}

	log_info("Starting Media Station X");
	start_server(CONFIG->port, &SERVER_CONNECTION);
}

void sigint_handler(int sig) {
	log_info("Closing server...");
	close(SERVER_CONNECTION);
	exit(EXIT_SUCCESS);
}

char * read_json(const char * filepath) {
	FILE * json_file = fopen(filepath, "r");

	if (json_file == NULL) {
		perror("Json file not found");
		exit(EXIT_FAILURE);
	}

	char line[120];
	char * json = (char *)malloc(sizeof(char) * JSON_MAX_SIZE);
	json[0] = '\0';

	while(fgets(line, sizeof(line), json_file)) {
		strncat(json, line, JSON_MAX_SIZE - strlen(json) - 1);
	}

	fclose(json_file);

	return json;
	
}

char * build_json_response(char * json) {
	char * response = (char *) malloc(sizeof(char) * RESPONSE_MAX_SIZE);

	sprintf(
		response, 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length:%ld\r\n"
		"\r\n"
		"%s", 
		strlen(json),
		json
	);

	return response;
}

int send_data(int client_fd, char * data, size_t data_len) {
	ssize_t bytes = send(client_fd, data, data_len, 0);
	
	if (bytes == -1) {
		perror("Error sending data");
		return -1;
	}

	return 0;
}

int send_404_response(struct request * request, int client_fd) {
	char * response = (char *) malloc(sizeof(char) * RESPONSE_MAX_SIZE);

	sprintf(
		response, 
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: plain/text\r\n"
		"\r\n"
		"404 Not Found" 
	);

	int error = send_data(client_fd, response, strlen(response));
	free(response);
	return error;
}

const char *get_file_extension(const char *file_name) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot + 1;
}

const char *get_media_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "link";
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return "link";
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return "image";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "imge";
    } else if (strcasecmp(file_ext, "mp4") == 0) {
        return "video";
    } else if (strcasecmp(file_ext, "mkv") == 0) {
        return "video";
    } else {
        return "link";
    }
}

const char *get_icon(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "link";
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return "link";
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return "image";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "imge";
    } else if (strcasecmp(file_ext, "mp4") == 0) {
        return "movie";
    } else if (strcasecmp(file_ext, "mkv") == 0) {
        return "movie";
    } else {
        return "link";
    }
}

const char *get_mime_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "text/html";
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "image/png";
    } else if (strcasecmp(file_ext, "mp4") == 0) {
        return "video/mp4";
    } else if (strcasecmp(file_ext, "mkv") == 0) {
        return "video/x-matroska";
    } else {
        return "application/octet-stream";
    }
}


char * generate_content_json(struct media_location * location, char * start_dir) {
	struct media_list * list = create_media_list();
	char dir[256];

	if (NULL != start_dir) {
		sprintf(dir, "%s/%s", location->media_dir, start_dir);
	} else {
		strcpy(dir, location->media_dir);
	}

	if (LIST_TYPE_FLAT == location->list_type) {
		list_files(dir, list, 1, location->list_depth);
	} else {
		list_files(dir, list, 0, 1);
	}
	
	char * json = (char *)malloc(sizeof(char) * JSON_MAX_SIZE);

	if (LIST_DISPLAY_GRID == location->display) {
		sprintf(json, "{"
			     " \"type\": \"list\","
			     " \"headline\": \"%s\","
			     " \"template\": {"
			     "     \"layout\": \"0,0,2,2\","
			     "     \"iconSize\": \"medium\","
			     "     \"alignment\": \"center\""
			     " },"
			     " \"items\": [",
			     start_dir ? start_dir : "List of contents"
			 );
	} else if (LIST_DISPLAY_LIST == location->display) {
		sprintf(json, "{"
			     " \"type\": \"list\","
			     " \"headline\": \"%s\","
			     " \"template\": {"
			     "     \"layout\": \"0,0,12,1\""
			     " },"
			     " \"items\": [",
			     start_dir ? start_dir : "List of contents"
			 );
	} else {
		free(json);
		perror("Invalid display configuration");
	}

	
	int offset = strlen(json);
	int items_count = 0;

	for(int i = 0; i < list->length; i++) {
		struct media_file * file = list->list[i];
		const char * file_extension = get_file_extension(file->filename);
		
		if (strstr(location->file_types, file_extension) == NULL) {
			continue;
		}

		char * item = (char *)malloc(sizeof(char) * 500);
		char * delimiter = ",";
		const char * media_type = get_media_type(file_extension);
	
		char action[256];
		char icon[50];
		
		if (MF_TYPE_DIR == file->type) {
			sprintf(
				action, 
				"content:http://%s:%i/content/%s/%s.json",
			        CONFIG->host,
				CONFIG->port,
				location->slug,
				file->path + strlen(location->media_dir) + 1
			       );
			strcpy(icon, "msx-white-soft:folder");

		} else if (MF_TYPE_FILE == file->type) {
			sprintf(
				action, 
				"%s:http://%s:%i/media/%s/%s",
				media_type,
				CONFIG->host,
				CONFIG->port,
				location->slug,
				file->path + strlen(location->media_dir) + 1
			);
			sprintf(icon, "msx-white-soft:%s", get_icon(file_extension));

		} else {
			free(item);
			continue;
		}

		if (LIST_DISPLAY_GRID == location->display) {
			sprintf(
				item, 
				"{"
				" \"text\":\"%s\","
				" \"action\":\"%s\","
				" \"icon\":\"%s\","
        	     		" \"color\": \"msx-glass\""
				"}%s",
				file->filename,
				action,
				icon,
				delimiter
			);
		} else if (LIST_DISPLAY_LIST == location->display) {
			sprintf(
				item, 
				"{"
				" \"text\":\"{ico:%s} %s\","
				" \"action\":\"%s\""
				"}%s",
				icon,
				file->filename,
				action,
				delimiter
			);
		} else {
			free(item);
			continue;
		}


		int item_length = strlen(item);

		strcpy(json + offset, item);
		offset  += item_length;
		free(item);
		items_count += 1;
	}

	strcpy(json + offset, "]}");
	
	if (items_count > 0) {
		json[offset - 1] = ' ';
	}

	free(list);
	return json;
}



int send_chunk(char * chunk, ssize_t len, int client_fd) {
	char chunk_len[30];	
	
	sprintf(chunk_len, "%zx\r\n", len);

	if (send(client_fd, chunk_len, strlen(chunk_len), 0) == -1) {
		perror("Error when sending chunk field");
		return -1;
	}
	
	if (send(client_fd, chunk, len, 0) == -1) {
		perror("Error when sending chunk");
		return -1;
	};
	
	if (send(client_fd, "\r\n", 2, 0) == -1) {
		perror("Error when sending chunk ending");
		return -1;
	}

	return 0;	
}

int send_chunked_file(struct request * request, int client_fd) {	
	char filepath[250];
	char full_filepath[500]; 
	
	struct media_location * location = CONFIG->locations[0];

	sscanf(request->uri, "/video%250[^\n]", filepath);
	strcpy(full_filepath, location->media_dir);
	strcat(full_filepath, filepath);
	
	log_info("Sending chunked video %s", full_filepath);
	
	int file = open(full_filepath, O_RDONLY);

	if (file == -1) {
		perror(strerror(errno));
		return -1;
	}

	const char * file_extension = get_file_extension(filepath);
	const char * mime_type = get_mime_type(file_extension);

	struct stat file_stat;
	fstat(file, &file_stat);

	long int file_size = (long int) file_stat.st_size;

	char * header = (char *)malloc(sizeof(char) * 200);
	sprintf(header, 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: %s\r\n"
		"Accept-Ranges: none\r\n"
		"Transfer-Encoding: chunked\r\n\r\n",
		mime_type);

	int header_len = strlen(header);
	char * response = (char *)malloc(CONFIG->media_buffer);
	
	send(client_fd, header, header_len, 0);
	free(header);

	char chunk_length_field[30]; 
	ssize_t bytes_read = 0;
	int chunk = 0;
	while((bytes_read = read(file, response, CONFIG->media_buffer)) > 0) {
		chunk +=1;
		log_info("Sending chunk %i of %i bytes...", chunk, bytes_read); 
		
		if (send_chunk(response, bytes_read, client_fd) == -1) {
			free(response);
			close(file);
			perror(strerror(errno));
			return -1;
		}

		log_info("The chunk sent is %s", chunk_length_field);
	}	

	log_info("Sending the closing chunk...");
	send(client_fd, "0\r\n\r\n", 5, 0);	
	free(response);
	close(file);
	
	return 0;
}

int send_json_response(char * json, int client_fd) {
	char * response = build_json_response(json);
	int error = send_data(client_fd, response, strlen(response));
	free(response);
	return error;
}

int send_start_json(struct request * request, int client_fd) {
	char * json = (char *)malloc(JSON_MAX_SIZE);
	sprintf(json, "{"
		      "  \"name\":\"%s\","
		      "  \"version\":\"1.0.1\","
		      "  \"parameter\":\"menu:http://%s:%i/msx/menu.json\","
		      "  \"welcome\":\"none\""
		      " }",
		      CONFIG->title,
		      CONFIG->host,
		      CONFIG->port
	        );
	int error = send_json_response(json, client_fd);
	free(json);
	return error;
}

int send_menu_json(struct request * request, int client_fd) {
	char * json = (char *)malloc(JSON_MAX_SIZE);
	sprintf(json, "{"
		      "  \"headline\":\"%s\","
		      "  \"menu\":[",
		      CONFIG->title
	        );
	char parameter[120];
	for(int i = 0; i < CONFIG->locations_len; i++) {
		char * delimiter = ",";
		
		if (i == CONFIG->locations_len - 1) {
			delimiter = "";
		}

		struct media_location * location = CONFIG->locations[i];
		sprintf(parameter, "{\"label\":\"%s\",", location->name);
		strcat(json, parameter); 
		sprintf(parameter, 
			"\"data\":\"http://%s:%i/content/%s.json\"}%s", 
			CONFIG->host,
			CONFIG->port,
			location->slug,
			delimiter
		);
		strcat(json, parameter); 
	}
	strcat(json, "]}");
	int error = send_json_response(json, client_fd);
	free(json);
	return error;
}

int send_content_json(struct request * request, int client_fd) {
	char content_path[256];
	char slug[121];
	char * start_dir = NULL;

	sscanf(request->uri, "/content/%255[^.].json", content_path);

	if (strstr(content_path, "/") > 0) {
		start_dir = (char *)malloc(135);
		sscanf(content_path, "%120[^/]/%135[^.].json", slug, start_dir);
	} else {
		strncpy(slug, content_path, 120);
	}

	struct media_location * location = get_location(CONFIG, slug);
	
	if (NULL == location) {
		free(start_dir);
		return send_404_response(request, client_fd);
	}



	char * json = generate_content_json(location, start_dir);
	int error = send_json_response(json, client_fd);
	free(start_dir);
	free(json);
	return error;
}

int send_streamed_file(struct request * request, int client_fd) {
	char slug[121];
	char filepath[251];
	char full_filepath[500];

	sscanf(request->uri, "/media/%120[^/]/%250[^\n]", slug, filepath);

	struct media_location * location = get_location(CONFIG, slug);

	if (NULL == location) {
		return send_404_response(request, client_fd);
	}

	sprintf(full_filepath, "%s/%s", location->media_dir, filepath);
	
	log_info("Streaming file %s ...", full_filepath);
	
	int file = open(full_filepath, O_RDONLY);

	if (file == -1) {
		perror(strerror(errno));
		return -1;
	}

	const char * file_extension = get_file_extension(filepath);
	const char * mime_type = get_mime_type(file_extension);

	struct stat file_stat;
	fstat(file, &file_stat);

	uint64_t file_size = (uint64_t) file_stat.st_size;

	char * response = (char *)malloc(CONFIG->media_buffer);
	
	response[0] = '\0';

	/**
	 * Range stuff
	 **/
	char * range = get_header(request, "Range");
	char header[256];
	uint64_t bytes_start = 0;
	uint64_t bytes_end = 0;
	int ranges = 0;
	
	if (range != NULL) {
	      	ranges = sscanf(range, "bytes=%lu-%lu", &bytes_start, &bytes_end);

		if (ranges == 1) {
			bytes_end = file_size - 1;
		}	

		strcat(response, "HTTP/1.1 206 Partial Content\r\n");
		sprintf(
			header,
			"Content-Range: bytes %lu-%lu/%lu\r\n", 
			bytes_start,
			bytes_end, 
			file_size
		);
		strcat(response, header);
		strcat(response, "Connection: keep-alive\r\n");
		sprintf(header, "Content-Length: %lu\r\n", bytes_end - bytes_start + 1);
		strcat(response, header);

		lseek(file, bytes_start, SEEK_SET);
	} else {

		strcat(response, "HTTP/1.1 200 OK\r\n");
		strcat(response, "Connection: close\r\n");
		strcat(response, "Accept-Ranges: bytes\r\n");
		sprintf(header, "Content-Length: %lu\r\n", file_size);
		strcat(response, header);
	}

	sprintf(header, "Content-Type: %s\r\n", mime_type);
	strcat(response, header);
	strcat(response, "\r\n");

	send_data(client_fd, response, strlen(response));
	
	ssize_t bytes_read = 0;
	int part = 0;
	while((bytes_read = read(file, response, CONFIG->media_buffer)) > 0) {
		part +=1;
		log_info("Sending part %i of %i bytes...", part, bytes_read); 

		if(send_data(client_fd, response, bytes_read)) {
			free(response);
			close(file);
			return -1;
		}

	}	

	free(response);
	close(file);
	
	return 0;
}

int route(struct request * request, int client_fd) {
	int error;

	if (strstr(request->uri, "msx/start.json")) {
		return send_start_json(request, client_fd);
	} 
	
	if (strstr(request->uri, "msx/menu.json")) {
		return send_menu_json(request, client_fd);
	}

	if (strstr(request->uri, "content/")) {
		return send_content_json(request, client_fd);
	}
	
	if (strstr(request->uri, "media/")) {
		return send_streamed_file(request, client_fd);
	}	


	return send_404_response(request, client_fd);


}

void *handle_request(void *arg) {

	int client_fd = *((int *)arg);
	char * buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
	
	bool close_connection = true;

	do {

		memset(buffer, '\0', BUFFER_SIZE);
		ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
	
		if (bytes_received > 0) {
			
			struct request * request = get_request(buffer);
			char * connection = get_header(request, "Connection");

			log_info("Request received %s", request->uri);

			if (connection != NULL && strcmp(connection, "keep-alive") == 0) {
				close_connection = false;
			}

			if (route(request, client_fd)) {
				close_connection = true;
			}	

			free(request);
		} else {
			log_info("Empty request received: %d", bytes_received);
			close_connection = true;
		}
		

	} while (!close_connection);
	
	log_info("Closing connection with the client %i", client_fd);
	close(client_fd);
	free(buffer);
	free(arg);

	return NULL;

}

void start_server(uint16_t port, int * p_connection) {
	int server_fd;
	struct sockaddr_in server_addr;

	//create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	*p_connection = server_fd;

	if (server_fd < 0) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	int reuse_addr = 1;

	if (setsockopt(
				server_fd, 
				SOL_SOCKET, 
				SO_REUSEADDR, 
				&reuse_addr, 
				sizeof(int)
	) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}

	#ifdef SO_REUSEPORT
	int reuse_port = 1;
	
	if (setsockopt(
				server_fd, 
				SOL_SOCKET, 
				SO_REUSEPORT, 
				&reuse_port, 
				sizeof(int)
	) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}
	#endif

	// config socket
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	// bind socket to port
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	// listen for connections
	if (listen(server_fd, 10) < 0) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	log_info("Server started listeting to port %d", port);

	while (1) {
	        struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
	        int * client_fd = (int *)malloc(sizeof(int));	

		if ((*client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
			perror("Accept connection failed");
			exit(EXIT_FAILURE);		
		}

		log_info("New client connection ID %d", *client_fd);

		//start a new thread to handle client request
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, handle_request, (void *)client_fd);
		pthread_detach(thread_id);


	}

} 
