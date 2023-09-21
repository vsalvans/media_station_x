#ifndef SCAN_H
#define SCAN_H

#define MEDIA_DIR "/home/pi/projects/media_station_x/media"

enum scan_mode {
	SCAN_DEFAULT,
	SCAN_FLAT
};

enum media_file_type {
	MF_TYPE_FILE,
	MF_TYPE_DIR
};


struct media_file {
	char * filename;
	char * path;
	enum media_file_type type;
};

struct media_list {
	int length;
	struct media_file ** list;
};

struct media_list * create_media_list();
void list_files(char * path, struct media_list * list, enum scan_mode mode, int max_depth);
void add_media_file(struct media_file * file, struct media_list * list);

#endif
