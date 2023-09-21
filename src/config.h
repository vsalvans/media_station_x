#ifndef CONFIG_H
#define CONFIG_H

enum ml_list_type {
	LIST_TYPE_FLAT,
	LIST_TYPE_DEFAULT,
};

enum ml_display {
	LIST_DISPLAY_GRID,
	LIST_DISPLAY_LIST,
};

struct media_location {
	char * slug;
	char * name;
	char * media_dir;
	enum ml_list_type list_type;
	short list_depth;
	char * file_types;
	enum ml_display display;
};

struct media_config {
	char * title;
	char * host;
	uint16_t port;
	int media_buffer;
	int locations_len;
	struct media_location ** locations;
};

struct media_config * read_config(const char * filepath); 

struct media_location * get_location(struct media_config * config, char * slug);

#endif
