#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#include "config.h"
#include "logger.h"

#define DEFAULT_SERVER_TITLE "My media server" 
#define DEFAULT_SERVER_HOST "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080
#define DEFAULT_MEDIA_BUFFER 4092

enum expected_t { 
	SERVER,
	START_CONFIG,
	SERVER_CONFIG,
	END_CONFIG,
	LOCATION,
	LOCATION_SLUG,
	LOCATION_CONFIG,
	NOTHING,
};

void invalid_parameter_config_error(char * parameter, int line_num) {
	char error[120];
	sprintf(error, "Invalid parameter %s in line %i", parameter, line_num);
	perror(error);
	exit(EXIT_FAILURE);
}

void invalid_value_parameter_config_error(char * parameter, char * value, int line_num) {
	char error[120];
	sprintf(error, "Invalid value %s for parameter %s in line %i", value, parameter, line_num);
	perror(error);
	exit(EXIT_FAILURE);
}

struct media_config * create_media_config(void) {
	struct media_config * config = (struct media_config *)malloc(sizeof(struct media_config));
	config->title = strdup(DEFAULT_SERVER_TITLE);
	config->host = strdup(DEFAULT_SERVER_HOST);
	config->port = DEFAULT_SERVER_PORT;
	config->media_buffer = DEFAULT_MEDIA_BUFFER;
	config->locations_len = 0;
	config->locations = (struct media_location **)malloc(50 * sizeof(struct media_location **));
	return config;
}

void add_location(struct media_config * config, char * slug) {
	 struct media_location * location = (struct media_location *)malloc(sizeof(struct media_location));
	 location->slug = slug;
	 location->name = NULL;
	 location->media_dir = NULL;
	 location->list_type = LIST_TYPE_DEFAULT;
	 location->list_depth = 0;
	 location->file_types = NULL;
	 location->display = LIST_DISPLAY_GRID;

	 config->locations[config->locations_len] = location;
	 config->locations_len +=1;
}

void add_server_config(
	struct media_config * config, 
	char * parameter, 
	char * value,
	int line_num
) { 
	if (strcmp(parameter, "title") == 0) {
		config->title = strdup(value);
	} else if (strcmp(parameter, "host") == 0) {
		config->host = strdup(value);
	} else if (strcmp(parameter, "port") == 0) {
		config->port = atoi(value);
	} else if (strcmp(parameter, "media_buffer") == 0) {
		config->media_buffer = atoi(value);
	} else {
		invalid_parameter_config_error(parameter, line_num);
	}
}

void add_last_location_config(
	struct media_config * config, 
	char * parameter, 
	char * value,
	int line_num
) { 
	struct media_location * location = config->locations[config->locations_len -1];

	if (strcmp(parameter, "name") == 0) {
		location->name = strdup(value);
	} else if (strcmp(parameter, "media_dir") == 0) {
		location->media_dir = strdup(value);
	} else if (strcmp(parameter, "list_depth") == 0) {
		location->list_depth = atoi(value);
	} else if (strcmp(parameter, "list_type") == 0) {
		if (strcmp(value, "flat") == 0) {
			location->list_type = LIST_TYPE_FLAT;
		} else if (strcmp(value, "default") == 0) {
			location->list_type = LIST_TYPE_DEFAULT;
		} else {
			invalid_value_parameter_config_error(parameter, value, line_num);
		}
	} else if (strcmp(parameter, "file_types") == 0) {
		location->file_types = strdup(value);
	} else if (strcmp(parameter, "display") == 0) {
		if (strcmp(value, "grid") == 0) {
			location->display = LIST_DISPLAY_GRID;
		} else if (strcmp(value, "list") == 0) {
			location->display = LIST_DISPLAY_LIST;
		} else {
			invalid_value_parameter_config_error(parameter, value, line_num);
		}
	} else {
		invalid_parameter_config_error(parameter, line_num);
	}
}

bool is_empty_line(char * line) {
	while(*line != '\0') {
		if (!isspace(*line)) {
			return false;
		}
		line +=1;
	}
	return true;
}

void config_error(const char * error, int line_num) {
	char message[200];
	sprintf(message, "Bad config file: %s in line %d", error, line_num);
	perror(message);
	exit(EXIT_FAILURE);
}	

enum expected_t expect_start_config(
		char * line, 
		int * offset,
		int line_num, 
		enum expected_t expected
) {
	if (strstr(line + *offset, "{") != NULL) {
		*offset = strlen(line);
		return expected;
	}

	config_error("{ expected", line_num);
}

enum expected_t expect_end_config(
		char * line, 
		int * offset,
		int line_num, 
		enum expected_t expected
) {
	if (strstr(line + *offset, "}") != NULL) {
		*offset = strlen(line);
		if (LOCATION_CONFIG == expected) {
			return LOCATION;
		}

		if (SERVER_CONFIG == expected) {
			return NOTHING;
		}
	}

	config_error("} expected", line_num);
}

enum expected_t expect_server(char * line, int * offset, int line_num) {
	if (strstr(line + *offset, "server") != NULL) {
		*offset += strlen("server");
		return START_CONFIG;
	}

	config_error("SERVER expected", line_num);
}

enum expected_t expect_location(char * line, int * offset, int line_num) {
	if (strstr(line + *offset, "location") != NULL) {
		*offset += strlen("location");
		return LOCATION_SLUG;
	}

	if (is_empty_line(line)) {
		*offset = strlen(line);
		return LOCATION;
	}

	return expect_end_config(line, offset, line_num, SERVER_CONFIG);
}

enum expected_t expect_location_slug(
		char * line, 
		int * offset, 
		int line_num,
		struct media_config * config
) {
	char slug[128];
	if (sscanf(line + *offset, "%127s", slug) == 1) {
		log_info("location detected [%s]", slug);
		*offset += strlen(slug);
		
		add_location(config, strdup(slug));

		return START_CONFIG;
	}
	
	config_error("LOCATION SLUG expected", line_num);
}	

enum expected_t expect_config(
	char * line, 
	int * offset, 
	int line_num, 
	enum expected_t expected, 
	struct media_config * config
) {
	char conf[128];
	char value[128];

	if (is_empty_line(line) || 
	    sscanf(line, "%*c %127[^:]: %127[^\n]%*c", conf, value) == 2) {
		log_info("config detected [%s:%s]", conf, value);
		*offset = strlen(line);
		
		if (SERVER_CONFIG == expected) {
			add_server_config(config, conf, value, line_num);
		} 
		
		if (LOCATION_CONFIG == expected) {
			add_last_location_config(config, conf, value, line_num);
		}

		return expected;
	}

	*offset -= 1;

	if (SERVER_CONFIG == expected) {
		return LOCATION;
	}

	return END_CONFIG;
}


enum expected_t expect_nothing(char * line, int * offset, int line_num) {
	if (is_empty_line(line)) {
		*offset = strlen(line);
		return NOTHING;
	}

	config_error("Unexpected token", line_num);
}



struct media_config * read_config(const char * filepath) {
	FILE * file = fopen(filepath, "r");

	if (NULL == file) {
		perror("Error reading config file media.cnf");
	}

	struct media_config * config = create_media_config();

	int line_num = 1;
	char line[300];
	enum expected_t expected = SERVER;
	enum expected_t next_expected = SERVER_CONFIG;

	while(fgets(line, sizeof(line), file) != NULL) {

		if (is_empty_line(line)) {
			line_num += 1;
			continue;
		};

		for(int i = 0; i < strlen(line); i++) {
			switch (expected) {
				case SERVER:
					expected = expect_server(line, &i, line_num);
					next_expected = SERVER_CONFIG;
					break;
				case START_CONFIG:
					expected = expect_start_config(line, &i, line_num, next_expected);
					break;
				case END_CONFIG:
					expected = expect_end_config(line, &i, line_num, next_expected);
					break;
				case LOCATION:
					expected = expect_location(line, &i, line_num);
					break;
				case LOCATION_SLUG:
					expected = expect_location_slug(line, &i, line_num, config);
					next_expected = LOCATION_CONFIG;
					break;
				case SERVER_CONFIG:
					expected = expect_config(line, &i, line_num, expected, config);
					break;
				case LOCATION_CONFIG:
					expected = expect_config(line, &i, line_num, expected, config);
					break;
				case NOTHING:
					expected = expect_nothing(line, &i, line_num);
					break;

			}
		}

		line_num +=1;
	}

	return config;
}

struct media_location * get_location(struct media_config * config, char * slug) {
	for(int i = 0; i < config->locations_len; i++) {
		struct media_location * location = config->locations[i];
		if (strcmp(slug, location->slug) == 0) {
		       return location;
		}
	}

	return NULL;
}	

