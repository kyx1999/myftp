#ifndef _UTIL_H_
#define _UTIL_H_

#define STRING_LENGTH 256

#define BUFFER_SIZE 1024

#define BACKLOG 128

#define SUCCESS 1

#define FAILED 0

#define BINARY 0

#define ASCII 1

#define LOGIN 0

#define MKDIR 1

#define RMDIR 2

#define PWD 3

#define CD 4

#define LS 5

#define PUT 6

#define GET 7

#define DATA 8

#define END 9

#define QUIT 10

int is_ip(char *ip);

int get_port(char *port);

char read_next(char *buffer);

unsigned int get_crc32c(unsigned char *start, unsigned int size);

#endif
