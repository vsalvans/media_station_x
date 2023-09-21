#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "request.h"


void add_header(struct request * request, const char * name, const char * value) {
	struct header * header = (struct header *)malloc(sizeof(struct header));
	header->name = strdup(name);
 	header->value = strdup(value);
	request->headers[request->headers_len] = header;
	request->headers_len += 1;
}

char * get_header(struct request * request, const char * name) {
	for(int i = 0; i < request->headers_len; i++) {
		if (strcmp(request->headers[i]->name, name) == 0) {
			return request->headers[i]->value;
		}	       
	}

	return NULL;
}

char *url_decode(const char *src) {
    size_t src_len = strlen(src);
    char *decoded = (char *)malloc(src_len + 1);
    size_t decoded_len = 0;

    // decode %2x to hex
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        } else {
            decoded[decoded_len++] = src[i];
        }
    }

    // add null terminator
    decoded[decoded_len] = '\0';
    return decoded;
}

struct request * get_request(const char * raw_request) {
	struct request * request = (struct request *)malloc(sizeof(struct request));
	request->headers_len = 0;
	request->headers = (struct header **)malloc(20 * sizeof(struct header *));
	char method[5];
	char uri[255];
	char h_name[50];
	char h_value[255];
	int read_bytes;
	int offset;

	sscanf(raw_request, "%s %s HTTP/1.1\r\n%n", method, uri, &read_bytes);
	request->method = strdup(method);
	request->uri = strdup(url_decode(uri));
	offset = read_bytes;

	while(sscanf(
			raw_request + offset, 
			"%50[^:]: %255[^\r]%*c%n", 
			h_name, 
			h_value, 
			&read_bytes
	     ) == 2
	) {
		add_header(request, h_name, h_value);
		offset += read_bytes + 1;
	}

	return request;
}

