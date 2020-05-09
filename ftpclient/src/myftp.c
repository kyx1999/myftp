#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "util.h"

void myftp(char *username, char *password, char *ip, int port)
{
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("ERROR: Creating socket failed!\n");
		return;
	}
	int recv_buf = 1024 * 32;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const void*)&recv_buf, sizeof(int));
	int send_buf = 1024 * 32;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const void*)&send_buf, sizeof(int));
	//创建socket

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr(ip);
	dest_addr.sin_port = htons(port);
	if (connect(sockfd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in)) == -1)
	{
		printf("ERROR: Creating connection failed!\n");
		return;
	}
	//连接服务器

	char login[STRING_LENGTH * 2 + 1];
	login[0] = LOGIN;
	strcpy(login + 1, username);
	strcpy(login + STRING_LENGTH + 1, password);
	if (send(sockfd, (void*)&login, sizeof(login), 0) < 1)
	{
		close(sockfd);
		printf("ERROR: myftp login failed!\n");
		return;
	}
	int ack;
	if (recv(sockfd, (void*)&ack, sizeof(int), 0) < 1)
	{
		close(sockfd);
		printf("ERROR: myftp login failed!\n");
		return;
	}
	//发送用户名和密码

	if ((ack = ntohl(ack)) > 0)
	{
		printf("You are client %d.\n", ack);
	}
	else
	{
		printf("Username doesn't exist or password is error!\n");
		return;
	}

	while (chdir(getenv("HOME")) == -1);
	char mode = BINARY;
	char buffer[BUFFER_SIZE];
	while (1)
	{
		printf("myftp> ");
		char cmd[STRING_LENGTH];
		char c;
		for (int i = 0; i < STRING_LENGTH + 1; i++)
		{
			if (i == STRING_LENGTH)
			{
				printf("ERROR: Command must be less than %d charcaters!\nmyftp> ", STRING_LENGTH - 1);
				i = -1;
				while ((c = getchar()) != '\n' && c != EOF);
				continue;
			}
			if (i == 0)
			{
				c = getchar();
				while (c == ' ' || c == '\t' || c == '\n' || c == EOF)
				{
					c = getchar();
				}
			}
			else
			{
				c = getchar();
			}
			if (c == ' ' || c == '\t' || c == '\n' || c == EOF)
			{
				cmd[i] = '\0';
				break;
			}
			cmd[i] = c;
		}

		if (strcmp(cmd, "binary") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				mode = BINARY;
				printf("The transmission mode is \"binary\" now.\n");
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (binary)\n");
			}
		}
		else if (strcmp(cmd, "ascii") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				mode = ASCII;
				printf("The transmission mode is \"ascii\" now.\n");
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (ascii)\n");
			}
		}
		else if (strcmp(cmd, "mkdir") == 0)
		{
			char pathname[STRING_LENGTH];
			char c = read_next(pathname);

			if (c != 0)
			{
				buffer[0] = MKDIR;
				strcpy(buffer + 1, pathname);
				if (send(sockfd, (void*)&buffer, strlen(pathname) + 2, 0) < 1)
				{
					printf("ERROR: myftp mkdir failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp mkdir failed!\n");
					continue;
				}
				if (buffer[0] != SUCCESS)
				{
					printf("%s", buffer + 1);
				}
			}
		}
		else if (strcmp(cmd, "rmdir") == 0)
		{
			char pathname[STRING_LENGTH];
			char c = read_next(pathname);

			if (c != 0)
			{
				buffer[0] = RMDIR;
				strcpy(buffer + 1, pathname);
				if (send(sockfd, (void*)&buffer, strlen(pathname) + 2, 0) < 1)
				{
					printf("ERROR: myftp rmdir failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp rmdir failed!\n");
					continue;
				}
				if (buffer[0] != SUCCESS)
				{
					printf("%s", buffer + 1);
				}
			}
		}
		else if (strcmp(cmd, "pwd") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				buffer[0] = PWD;
				if (send(sockfd, (void*)&buffer, 1, 0) < 1)
				{
					printf("ERROR: myftp pwd failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp pwd failed!\n");
					continue;
				}
				printf("%s", buffer + 1);
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (pwd)\n");
			}
		}
		else if (strcmp(cmd, "cd") == 0)
		{
			char path[STRING_LENGTH];
			char c = read_next(path);

			if (c != 0)
			{
				buffer[0] = CD;
				strcpy(buffer + 1, path);
				if (send(sockfd, (void*)&buffer, strlen(path) + 2, 0) < 1)
				{
					printf("ERROR: myftp cd failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp cd failed!\n");
					continue;
				}
				if (buffer[0] != SUCCESS)
				{
					printf("%s", buffer + 1);
				}
			}
		}
		else if (strcmp(cmd, "ls") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				buffer[0] = LS;
				if (send(sockfd, (void*)&buffer, 1, 0) < 1)
				{
					printf("ERROR: myftp ls failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp ls failed!\n");
					continue;
				}
				printf("%s", buffer + 1);
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (ls)\n");
			}
		}
		else if (strcmp(cmd, "lmkdir") == 0)
		{
			char pathname[STRING_LENGTH];
			char c = read_next(pathname);

			if (c != 0)
			{
				if (mkdir(pathname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
				{
					printf("ERROR: myftp lmkdir failed!\n");
				}
			}
		}
		else if (strcmp(cmd, "lrmdir") == 0)
		{
			char pathname[STRING_LENGTH];
			char c = read_next(pathname);

			if (c != 0)
			{
				if (rmdir(pathname) == -1)
				{
					printf("ERROR: myftp lrmdir failed!\n");
				}
			}
		}
		else if (strcmp(cmd, "lpwd") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				char *path;
				if ((path = getcwd(NULL, 0)) != NULL)
				{
					printf("%s\n", path);
					free(path);
					path = NULL;
				}
				else
				{
					printf("ERROR: myftp lpwd failed!\n");
				}
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (lpwd)\n");
			}

		}
		else if (strcmp(cmd, "lcd") == 0)
		{
			char path[STRING_LENGTH];
			char c = read_next(path);

			if (c != 0)
			{
				if (chdir(path) == -1)
				{
					printf("ERROR: myftp lcd failed!\n");
				}
			}
		}
		else if (strcmp(cmd, "dir") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				DIR *dp;
				if ((dp = opendir(".")) != NULL)
				{
					struct dirent *entry;
					while ((entry = readdir(dp)) != NULL)
					{
						if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, ".."))
						{
							printf("%s\n", entry->d_name);
						}
					}
					closedir(dp);
				}
				else
				{
					printf("ERROR: myftp dir failed!\n");
				}
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (dir)\n");
			}
		}
		else if (strcmp(cmd, "put") == 0)
		{
			char filename[STRING_LENGTH];
			char c = read_next(filename);

			if (c != 0)
			{
				if (strchr(filename, '/') != NULL || strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || access(filename, R_OK) == -1)
				{
					printf("ERROR: myftp put failed!\n");
					continue;
				}
				buffer[0] = PUT;
				buffer[1] = mode;
				char *result = buffer + 1;
				strcpy(result + 1, filename);
				if (send(sockfd, (void*)&buffer, strlen(result + 1) + 3, 0) < 1)
				{
					printf("ERROR: myftp put failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp put failed!\n");
					continue;
				}
				if (buffer[0] != SUCCESS)
				{
					printf("ftpserver: put failed!\n");
					continue;
				}
				int *temp = (int*)result;
				int breakpoint = ntohl(*temp);
				FILE *fp;
				if ((fp = fopen(filename, "rb")) == NULL)
				{
					buffer[0] = FAILED;
					send(sockfd, (void*)&buffer, 1, 0);
					printf("ERROR: myftp put failed!\n");
					continue;
				}
				fseek(fp, breakpoint, SEEK_SET);
				buffer[0] = DATA;
				int size;
				while ((size = fread(buffer + sizeof(int) + 1, sizeof(char), BUFFER_SIZE - sizeof(int) - 1, fp)) == BUFFER_SIZE - sizeof(int) - 1)
				{
					if (send(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)//此处可以测试断点
					{
						buffer[0] = FAILED;
						send(sockfd, (void*)&buffer, 1, 0);
						printf("ERROR: myftp put failed!\n");
						break;
					}
				}
				fclose(fp);
				if (buffer[0] != FAILED)
				{
					buffer[0] = END;
					*temp = htonl(size);
					if (send(sockfd, (void*)&buffer, size + sizeof(int) + 1, 0) < 1)
					{
						buffer[0] = FAILED;
						send(sockfd, (void*)&buffer, 1, 0);
						printf("gtpserver: put failed!\n");
						continue;
					}
					printf("myftp: 1 file has been put on.\n");
				}
			}
		}
		else if (strcmp(cmd, "mput") == 0)
		{
			char filenames[STRING_LENGTH];
			char filename[STRING_LENGTH];
			char c = read_next(filenames);

			if (c != 0)
			{
				int i = 0, s = 0, success_count = 0, failed_count = 0;
				while (filenames[i] != '\0')
				{
					while (filenames[i] != ' ' && filenames[i] != '\t' && filenames[i] != '\0')
					{
						i++;
					}
					strncpy(filename, filenames + s, i - s);
					filename[i - s] = '\0';
					s = i;
					while (filenames[s] == ' ' || filenames[s] == '\t')
					{
						s++;
					}
					i = s;
					if (strchr(filename, '/') != NULL || strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || access(filename, R_OK) == -1)
					{
						failed_count++;
						continue;
					}
					buffer[0] = PUT;
					buffer[1] = mode;
					char *result = buffer + 1;
					strcpy(result + 1, filename);
					if (send(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
					{
						failed_count++;
						continue;
					}
					if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
					{
						failed_count++;
						continue;
					}
					if (buffer[0] != SUCCESS)
					{
						failed_count++;
						continue;
					}
					int *temp = (int*)result;
					int breakpoint = ntohl(*temp);
					FILE *fp;
					if ((fp = fopen(filename, "rb")) == NULL)
					{
						buffer[0] = FAILED;
						send(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
						failed_count++;
						continue;
					}
					fseek(fp, breakpoint, SEEK_SET);
					buffer[0] = DATA;
					int size;
					while ((size = fread(buffer + sizeof(int) + 1, sizeof(char), BUFFER_SIZE - sizeof(int) - 1, fp)) == BUFFER_SIZE - sizeof(int) - 1)
					{
						if (send(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)//此处可以测试断点
						{
							buffer[0] = FAILED;
							send(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
							failed_count++;
							success_count--;
							break;
						}
					}
					fclose(fp);
					if (buffer[0] != FAILED)
					{
						buffer[0] = END;
						*temp = htonl(size);
						if (send(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
						{
							buffer[0] = FAILED;
							send(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
							failed_count++;
							continue;
						}
					}
					success_count++;
				}
				printf("myftp: %d file has been put on. %d failed.\n", success_count, failed_count);
			}
		}
		else if (strcmp(cmd, "get") == 0)
		{
			char filename[STRING_LENGTH];
			char c = read_next(filename);

			if (c != 0)
			{
				buffer[0] = GET;
				char *result = buffer + 1;
				strcpy(result, filename);
				if (send(sockfd, (void*)&buffer, strlen(result) + 2, 0) < 1)
				{
					printf("ERROR: myftp get failed!\n");
					continue;
				}
				if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
				{
					printf("ERROR: myftp get failed!\n");
					continue;
				}
				if (buffer[0] != SUCCESS)
				{
					printf("ftpserver: get failed!\n");
					continue;
				}
				strcat(filename, ".temp");
				int breakpoint = 0;
				char last = '\0';
				if (access(filename, R_OK) == 0)
				{
					FILE *fp;
					if ((fp = fopen(filename, "r")) == NULL)
					{
						buffer[0] = FAILED;
						send(sockfd, (void*)&buffer, 1, 0);
						printf("ERROR: myftp get failed!\n");
						continue;
					}
					while (fread(&breakpoint, sizeof(int), 1, fp) != 1)
					{
						fseek(fp, 0, SEEK_SET);
					}
					if (mode == ASCII && breakpoint > 0)
					{
						last = fgetc(fp);
					}
					fclose(fp);
				}
				FILE *fp;
				filename[strlen(filename) - 5] = '\0';
				if (breakpoint == 0 ? ((fp = fopen(filename, "wb+")) == NULL) : ((fp = fopen(filename, "ab+")) == NULL))
				{
					buffer[0] = FAILED;
					send(sockfd, (void*)&buffer, 1, 0);
					printf("ERROR: myftp get failed!\n");
					continue;
				}
				buffer[0] = SUCCESS;
				int *temp = (int*)result;
				*temp = htonl(breakpoint);
				if (send(sockfd, (void*)&buffer, sizeof(int) + 1, 0) < 1)
				{
					fclose(fp);
					buffer[0] = FAILED;
					send(sockfd, (void*)&buffer, 1, 0);
					printf("ERROR: myftp get failed!\n");
					continue;
				}
				breakpoint = 0;
				int ret = recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
				while (ret > 0)
				{
					if (buffer[0] == DATA)
					{
						if (mode == BINARY)
						{
							fwrite(buffer + sizeof(int) + 1, sizeof(char), BUFFER_SIZE - sizeof(int) - 1, fp);
						}
						else
						{
							for (int i = sizeof(int) + 1; i < BUFFER_SIZE; i++)
							{
								if (last != '\r' && buffer[i] == '\n')
								{
									fputc('\r', fp);
								}
								fputc(buffer[i], fp);
								last = buffer[i];
							}
						}
						breakpoint += BUFFER_SIZE - sizeof(int) - 1;
					}
					else if (buffer[0] == END)
					{
						if (mode == BINARY)
						{
							fwrite(buffer + sizeof(int) + 1, sizeof(char), ntohl(*temp), fp);
						}
						else
						{
							for (int i = sizeof(int) + 1; i < ntohl(*temp) + sizeof(int) + 1; i++)
							{
								if (last != '\r' && buffer[i] == '\n')
								{
									fputc('\r', fp);
								}
								fputc(buffer[i], fp);
								last = buffer[i];
							}
						}
						filename[strlen(filename)] = '.';
						if (access(filename, F_OK) == 0)
						{
							remove(filename);
						}
						filename[strlen(filename) - 5] = '\0';
						break;
					}
					else
					{
						break;
					}
					ret = recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
				}
				if (buffer[0] != DATA && buffer[0] != END || ret < 1)
				{
					filename[strlen(filename)] = '.';
					FILE *tfp;
					if ((tfp = fopen(filename, "w")) != NULL)
					{
						fwrite(&breakpoint, sizeof(int), 1, tfp);
						if (mode == ASCII)
						{
							fputc(last, tfp);
						}
						fclose(tfp);
					}
					fclose(fp);
					printf("ftpserver: get failed!\n");
					continue;
				}
				fclose(fp);
				if (buffer[0] != FAILED)
				{
					printf("myftp: 1 file has been get down.\n");
				}
			}
		}
		else if (strcmp(cmd, "mget") == 0)
		{
			char filenames[STRING_LENGTH];
			char filename[STRING_LENGTH];
			char c = read_next(filenames);

			if (c != 0)
			{
				int i = 0, s = 0, success_count = 0, failed_count = 0;
				while (filenames[i] != '\0')
				{
					while (filenames[i] != ' ' && filenames[i] != '\t' && filenames[i] != '\0')
					{
						i++;
					}
					strncpy(filename, filenames + s, i - s);
					filename[i - s] = '\0';
					s = i;
					while (filenames[s] == ' ' || filenames[s] == '\t')
					{
						s++;
					}
					i = s;
					buffer[0] = GET;
					char *result = buffer + 1;
					strcpy(result, filename);
					if (send(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
					{
						failed_count++;
						continue;
					}
					if (recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
					{
						failed_count++;
						continue;
					}
					if (buffer[0] != SUCCESS)
					{
						failed_count++;
						continue;
					}
					strcat(filename, ".temp");
					int breakpoint = 0;
					char last = '\0';
					if (access(filename, R_OK) == 0)
					{
						FILE *fp;
						if ((fp = fopen(filename, "r")) == NULL)
						{
							buffer[0] = FAILED;
							send(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
							failed_count++;
							continue;
						}
						while (fread(&breakpoint, sizeof(int), 1, fp) != 1)
						{
							fseek(fp, 0, SEEK_SET);
						}
						if (mode == ASCII && breakpoint > 0)
						{
							last = fgetc(fp);
						}
						fclose(fp);
					}
					FILE *fp;
					filename[strlen(filename) - 5] = '\0';
					if (breakpoint == 0 ? ((fp = fopen(filename, "wb+")) == NULL) : ((fp = fopen(filename, "ab+")) == NULL))
					{
						buffer[0] = FAILED;
						send(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
						failed_count++;
						continue;
					}
					buffer[0] = SUCCESS;
					int *temp = (int*)result;
					*temp = htonl(breakpoint);
					if (send(sockfd, (void*)&buffer, BUFFER_SIZE, 0) < 1)
					{
						fclose(fp);
						buffer[0] = FAILED;
						send(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
						failed_count++;
						continue;
					}
					breakpoint = 0;
					int ret = recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
					while (ret > 0)
					{
						if (buffer[0] == DATA)
						{
							if (mode == BINARY)
							{
								fwrite(buffer + sizeof(int) + 1, sizeof(char), BUFFER_SIZE - sizeof(int) - 1, fp);
							}
							else
							{
								for (int i = sizeof(int) + 1; i < BUFFER_SIZE; i++)
								{
									if (last != '\r' && buffer[i] == '\n')
									{
										fputc('\r', fp);
									}
									fputc(buffer[i], fp);
									last = buffer[i];
								}
							}
							breakpoint += BUFFER_SIZE - sizeof(int) - 1;
						}
						else if (buffer[0] == END)
						{
							if (mode == BINARY)
							{
								fwrite(buffer + sizeof(int) + 1, sizeof(char), ntohl(*temp), fp);
							}
							else
							{
								for (int i = sizeof(int) + 1; i < ntohl(*temp) + sizeof(int) + 1; i++)
								{
									if (last != '\r' && buffer[i] == '\n')
									{
										fputc('\r', fp);
									}
									fputc(buffer[i], fp);
									last = buffer[i];
								}
							}
							filename[strlen(filename)] = '.';
							if (access(filename, F_OK) == 0)
							{
								remove(filename);
							}
							filename[strlen(filename) - 5] = '\0';
							break;
						}
						else
						{
							break;
						}
						ret = recv(sockfd, (void*)&buffer, BUFFER_SIZE, 0);
					}
					if (buffer[0] != DATA && buffer[0] != END || ret < 1)
					{
						filename[strlen(filename)] = '.';
						FILE *tfp;
						if ((tfp = fopen(filename, "w")) != NULL)
						{
							fwrite(&breakpoint, sizeof(int), 1, tfp);
							if (mode == ASCII)
							{
								fputc(last, tfp);
							}
							fclose(tfp);
						}
						fclose(fp);
						failed_count++;
						continue;
					}
					fclose(fp);
					if (buffer[0] != FAILED)
					{
						success_count++;
					}
				}
				printf("myftp: %d file has been get down. %d failed.\n", success_count, failed_count);
			}
		}
		else if (strcmp(cmd, "quit") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				buffer[0] = QUIT;
				send(sockfd, (void*)&buffer, 1, 0);
				close(sockfd);
				return;
			}
			while ((c = getchar()) != '\n' && c != EOF);
			printf("ERROR: Invalid parameter input. (quit)\n");
		}
		else
		{
			if (c != '\n' && c != EOF)
			{
				while ((c = getchar()) != '\n' && c != EOF);
			}
			printf("ERROR: Command doesn't exist!\n");
		}
	}
}
