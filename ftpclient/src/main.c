#include <stdio.h>
#include "util.h"
#include "myftp.h"

int main(int argc, char *argv[])
{
	char username[STRING_LENGTH], password[STRING_LENGTH], ip[STRING_LENGTH], port_str[STRING_LENGTH];
	int port;
	if (argc == 2)
	{
		int count, flag = 0, i = 0, j = 0, k = 0, l = 0;
		for (count = 0; argv[1][count] != '\0'; count++)
		{
			if (flag == 0)
			{
				if (argv[1][count] != ':')
				{
					username[i++] = argv[1][count];
					if (i == STRING_LENGTH)
					{
						printf("ERROR: Username must be less than %d charcaters!\n", STRING_LENGTH - 1);
						return 0;
					}
				}
				else
				{
					username[i] = '\0';
					i = count;
					flag++;
				}
			}
			else if (flag == 1)
			{
				if (argv[1][count] != '@')
				{
					password[j++] = argv[1][count];
					if (j == STRING_LENGTH)
					{
						printf("ERROR: password must be less than %d charcaters!\n", STRING_LENGTH - 1);
						return 0;
					}
				}
				else
				{
					password[j] = '\0';
					j = count;
					flag++;
				}
			}
			else if (flag == 2)
			{
				if (argv[1][count] != ':')
				{
					ip[k++] = argv[1][count];
					if (k == STRING_LENGTH)
					{
						printf("ERROR: ip must be less than %d charcaters!\n", STRING_LENGTH - 1);
						return 0;
					}
				}
				else
				{
					ip[k] = '\0';
					k = count;
					flag++;
				}
			}
			else
			{
				port_str[l++] = argv[1][count];
				if (l == STRING_LENGTH)
				{
					printf("ERROR: port must be less than %d charcaters!\n", STRING_LENGTH - 1);
					return 0;
				}
				if (argv[1][count + 1] == '\0')
				{
					port_str[l] = '\0';
					l = count + 1;
					break;
				}
			}
		}
		if (i != 0 && i < j && j < k && k < l)
		{
			if (!is_ip(ip))
			{
				printf("ERROR: Invalid ip input.\n");
				return 0;
			}
			if ((port = get_port(port_str)) == -1)
			{
				printf("ERROR: Invalid port input.\n");
				return 0;
			}
		}
		else
		{
			printf("ERROR: Invalid parameter input. (myftp username:password@ip:port)\n");
			return 0;
		}
	}
	else
	{
		printf("ERROR: Invalid parameter input. (myftp username:password@ip:port)\n");
		return 0;
	}

	myftp(username, password, ip, port);

	return 0;
}
