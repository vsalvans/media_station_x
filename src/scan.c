#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "scan.h"

struct media_list * create_media_list() {
	struct media_list * list = (struct media_list *)malloc(sizeof(struct media_list));

	list->length = 0;
	list->list = (struct media_file **)malloc(sizeof(void *) * 1024);

	return list;
}


void add_media_file(struct media_file * file, struct media_list * list) {
	list->list[list->length] = file;
	list->length += 1;
}


void list_files(
	char * path, 
	struct media_list * list,
	enum scan_mode mode,
	int max_depth
) {
	if (0 == max_depth) {
		return;
	}

	printf("Scanning files in %s\n", path);

	DIR * dir = opendir(path);

	struct dirent * file;
	char * media_path = (char *)malloc(500);

	if (dir != NULL) {
		while(file = readdir(dir)) {
			if (file->d_name[0] == '.') {
				continue;
			}

			sprintf(media_path, "%s/%s", path, file->d_name);

			enum media_file_type file_type;

			if (DT_REG == file->d_type) {
				file_type = MF_TYPE_FILE;
			} else if (DT_DIR == file->d_type) {
				file_type = MF_TYPE_DIR;
			} else if (DT_UNKNOWN == file->d_type) {
				struct stat stat;
				lstat(media_path, &stat);
				if (S_ISREG(stat.st_mode)) {
					file_type = MF_TYPE_FILE;
				} else if (S_ISDIR(stat.st_mode)) {
					file_type = MF_TYPE_DIR;
				} else {
					continue;
				}
			} else {
				continue;
			}

			if (SCAN_FLAT == mode && MF_TYPE_DIR == file_type) {
				list_files(media_path, list, mode, max_depth -1);
				continue;
			}

			struct media_file * media = (struct media_file *)malloc(sizeof(struct media_file));
			media->path = strdup(media_path);
			media->filename = strdup(file->d_name);
			media->type = file_type;

			add_media_file(media, list);

		}
		closedir(dir);
		free(file);
	} else {
		perror("Error when scaning files");
	}
	free(media_path);

}
