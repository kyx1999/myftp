#include <stdio.h>
#include "util.h"
#include "ftpserver.h"

int main(int argc, char *argv[])
{
	char *ip = "127.0.0.1";
	int port;
	if (argc == 2)
	{
		if ((port = get_port(argv[1])) == -1)
		{
			printf("ERROR: Invalid port input.\n");
			return 0;
		}
	}
	else if (argc == 3)
	{
		if (!is_ip(ip = argv[1]))
		{
			printf("ERROR: Invalid ip input.\n");
			return 0;
		}
		if ((port = get_port(argv[2])) == -1)
		{
			printf("ERROR: Invalid port input.\n");
			return 0;
		}
	}
	else
	{
		printf("ERROR: Invalid parameter input. (ftpserver [ip] port)\n");
		return 0;
	}

	ftpserver(ip, port);

	return 0;
}
