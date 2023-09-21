#ifndef REQUEST_H
#define REQUEST_H

struct header {
	char * name;
	char * value;
};

struct request {
	char * method;
	char * uri;
	int headers_len;
	struct header ** headers;
};

struct request * get_request(const char * raw_request);

char * get_header(struct request * request, const char * name);

#endif
