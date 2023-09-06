#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_SIZE 256
#define JSON_FILE "test.json"
#define JSON_MAX_SIZE 1024
#define RESPONSE_MAX_SIZE 2048

void start_server(uint16_t port);


int main (int argc, char * args[]) {
	printf("Starting Media Station X v2\n");
	start_server(8080);
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
		"Content-Length:%d\r\n"
		"\r\n"
		"%s", 
		strlen(json),
		json
	);

	return response;
}

char * build_404_response() {
	char * response = (char *) malloc(sizeof(char) * RESPONSE_MAX_SIZE);

	sprintf(
		response, 
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: plain/text\r\n"
		"\r\n"
		"404 Not Found" 
	);

	return response;
}

void *handle_request(void *arg) {

	printf("Handle request");

	int client_fd = *((int *)arg);
	char * buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
	
	ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
	
	if (bytes_received > 0) {
	        char * json = NULL;
		char * response;
		
		if (strstr(buffer, "msx/start.json")) {
			json = read_json("start.json");
		} else if (strstr(buffer, "msx/content.json")) {
			json = read_json("content.json");
		}	

		if (json != NULL) {
			response = build_json_response(json);
		} else {
			response = build_404_response();
		}

		int response_len = strlen(response); 
		send(client_fd, response, response_len, 0);	
		printf("Request received %s\n", buffer);
		free(response);
		free(json);
	}
	
	close(client_fd);
	free(buffer);
	free(arg);

	return NULL;

}

void start_server(uint16_t port) {
	int server_fd;
	struct sockaddr_in server_addr;

	//create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

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

	printf("Server started listeting to port %d\n", port);

	while (1) {
	        struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
	        int * client_fd = (int *)malloc(sizeof(int));	

		if ((*client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
			perror("Accept connection failed");
			exit(EXIT_FAILURE);		
		}

		printf("Something received in client ID %d\n", *client_fd);

		//start a new thread to handle client request
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, handle_request, (void *)client_fd);
		printf("The thread ID is %d", thread_id);
		pthread_detach(thread_id);


	}

} 
