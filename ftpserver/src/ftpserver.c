#include <dirent.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "util.h"

pthread_t tid_handle, tid_task;

int all = 0, current = 0;
pthread_mutex_t current_mutex = PTHREAD_MUTEX_INITIALIZER;

char work_path[STRING_LENGTH];
pthread_mutex_t path_mutex = PTHREAD_MUTEX_INITIALIZER;

int sock_users[BACKLOG];
pthread_t tid_users[BACKLOG];
char users[BACKLOG][STRING_LENGTH];

struct file_times
{
	char exist;
	ino_t f_ino;
	time_t f_mtime;
	int times;
};
struct file_times files[BACKLOG];
char *most = NULL;
int max;
pthread_mutex_t mem_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client(void *index)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	int i = (pthread_t*)index - tid_users;
	char buffer[BUFFER_SIZE];
	if (recv(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) < 1)
	{
		pthread_detach(pthread_self());
		close(sock_users[i]);
		users[i][0] = '\0';
		return NULL;
	}
	if (buffer[0] != LOGIN)
	{
		pthread_detach(pthread_self());
		close(sock_users[i]);
		users[i][0] = '\0';
		return NULL;
	}
	char *username = buffer + 1;
	char *password = buffer + STRING_LENGTH + 1;
	char identity[STRING_LENGTH * 2];
	strcpy(identity, username);
	strcat(identity, ":");
	strcat(identity, password);
	strcat(identity, "\n");
	//获取登录信息

	int ack = htonl(0);
	FILE *fp;
	char login_path[STRING_LENGTH * 2];
	strcpy(login_path, work_path);
	login_path[strlen(login_path) - 4] = '\0';
	strcat(login_path, "/users");
	if ((fp = fopen(login_path, "r")) == NULL)
	{
		pthread_detach(pthread_self());
		close(sock_users[i]);
		users[i][0] = '\0';
		return NULL;
	}
	char login[STRING_LENGTH * 2];
	while (fgets(login, STRING_LENGTH * 2, fp) != NULL)
	{
		if (strcmp(identity, login) == 0)
		{
			ack = htonl(i + 1);
			break;
		}
	}
	fclose(fp);
	if (ack == htonl(0))
	{
		pthread_detach(pthread_self());
		send(sock_users[i], (void*)&ack, sizeof(int), 0);
		close(sock_users[i]);
		users[i][0] = '\0';
		return NULL;
	}
	if (send(sock_users[i], (void*)&ack, sizeof(int), 0) > 0)
	{
		strcpy(users[i], username);
		pthread_mutex_lock(&current_mutex);
		current++;
		pthread_mutex_unlock(&current_mutex);
	}
	else
	{
		pthread_detach(pthread_self());
		send(sock_users[i], (void*)&ack, sizeof(int), 0);
		close(sock_users[i]);
		users[i][0] = '\0';
		return NULL;
	}
	//验证登录信息

	char current_path[STRING_LENGTH];
	strcpy(current_path, work_path);
	struct timeval tv_out;
	tv_out.tv_sec = 1200;
	tv_out.tv_usec = 0;
	setsockopt(sock_users[i], SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	while (1)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		if (recv(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) > 0)
		{
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			if (buffer[0] == MKDIR)
			{
				char *result = buffer + 1;
				if (strchr(result, '/') != NULL || strcmp(result, ".") == 0 || strcmp(result, "..") == 0)
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: Invalid folder name!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					continue;
				}
				struct passwd *pw;
				struct stat buf;
				if ((pw = getpwnam(users[i])) == NULL || stat(current_path, &buf) == -1)
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: ftpserver mkdir failed!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					continue;
				}
				if (buf.st_uid == pw->pw_uid || strcmp(current_path, work_path) == 0)
				{
					char temp[STRING_LENGTH];
					strcpy(temp, current_path);
					strcat(temp, "/");
					strcat(temp, result);
					if (mkdir(temp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
					{
						buffer[0] = FAILED;
						strcpy(result, "ftpserver: ftpserver mkdir failed!\n");
						send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
						continue;
					}
					while (chown(temp, pw->pw_uid, pw->pw_gid) == -1);
					buffer[0] = SUCCESS;
					send(sock_users[i], (void*)&buffer, 1, 0);
				}
				else
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: ftpserver mkdir permission denied!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
				}
			}
			else if (buffer[0] == RMDIR)
			{
				char *result = buffer + 1;
				if (strchr(result, '/') != NULL || strcmp(result, ".") == 0 || strcmp(result, "..") == 0)
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: Invalid folder name!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					continue;
				}
				char temp[STRING_LENGTH];
				strcpy(temp, current_path);
				strcat(temp, "/");
				strcat(temp, result);
				struct passwd *pw;
				struct stat buf;
				if ((pw = getpwnam(users[i])) == NULL || stat(temp, &buf) == -1)
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: ftpserver rmdir failed!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					continue;
				}
				if (buf.st_uid == pw->pw_uid)
				{
					if (rmdir(temp) == -1)
					{
						buffer[0] = FAILED;
						strcpy(result, "ftpserver: ftpserver rmdir failed!\n");
						send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
						continue;
					}
					buffer[0] = SUCCESS;
					send(sock_users[i], (void*)&buffer, 1, 0);
				}
				else
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: ftpserver rmdir permission denied!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
				}
			}
			else if (buffer[0] == PWD)
			{
				buffer[0] = SUCCESS;
				char *result = buffer + 1;
				strcpy(result, current_path + strlen(work_path) - 4);
				strcat(result, "\n");
				send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
			}
			else if (buffer[0] == CD)
			{
				char *result = buffer + 1;
				char last_path[STRING_LENGTH];
				pthread_mutex_lock(&path_mutex);
				if (getcwd(last_path, STRING_LENGTH) == NULL)
				{
					pthread_mutex_unlock(&path_mutex);
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: Path cannot be reached!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					continue;
				}
				if (chdir(current_path) == -1)
				{
					pthread_mutex_unlock(&path_mutex);
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: Path cannot be reached!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					continue;
				}
				if (chdir(result) == 0)
				{
					char temp[STRING_LENGTH];
					while (getcwd(temp, STRING_LENGTH) == NULL);
					if (strncmp(temp, work_path, strlen(work_path)) == 0)
					{
						buffer[0] = SUCCESS;
						strcpy(current_path, temp);
						send(sock_users[i], (void*)&buffer, 1, 0);
					}
					else
					{
						buffer[0] = FAILED;
						strcpy(result, "ftpserver: Path cannot be reached!\n");
						send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
					}
				}
				else
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: Path cannot be reached!\n");
					send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
				}
				while (chdir(last_path) == -1);
				pthread_mutex_unlock(&path_mutex);
			}
			else if (buffer[0] == LS)
			{
				char *result = buffer + 1;
				DIR *dp;
				if ((dp = opendir(current_path)) != NULL)
				{
					buffer[0] = SUCCESS;
					strcpy(result, "");
					struct dirent *entry;
					while ((entry = readdir(dp)) != NULL)
					{
						if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, ".."))
						{
							strcat(result, entry->d_name);
							strcat(result, "\n");
						}
					}
					closedir(dp);
				}
				else
				{
					buffer[0] = FAILED;
					strcpy(result, "ftpserver: Directory open failed!\n");
				}
				send(sock_users[i], (void*)&buffer, strlen(result) + 2, 0);
			}
			else if (buffer[0] == PUT)
			{
				char *result = buffer + 1;
				char mode = *result;
				if (strchr(result + 1, '/') != NULL || strcmp(result + 1, ".") == 0 || strcmp(result + 1, "..") == 0)
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
					continue;
				}
				struct passwd *pw;
				struct stat buf;
				if ((pw = getpwnam(users[i])) == NULL || stat(current_path, &buf) == -1)
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
					continue;
				}
				if (buf.st_uid == pw->pw_uid || strcmp(current_path, work_path) == 0)
				{
					char temp_name[STRING_LENGTH * 2];
					strcpy(temp_name, current_path);
					strcat(temp_name, "/");
					strcat(temp_name, result + 1);
					strcat(temp_name, ".temp");
					int breakpoint = 0;
					char last = '\0';
					if (access(temp_name, R_OK) == 0)
					{
						FILE *fp;
						if ((fp = fopen(temp_name, "r")) == NULL)
						{
							buffer[0] = FAILED;
							send(sock_users[i], (void*)&buffer, 1, 0);
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
					temp_name[strlen(temp_name) - 5] = '\0';
					if (breakpoint == 0 ? ((fp = fopen(temp_name, "wb+")) == NULL) : ((fp = fopen(temp_name, "ab+")) == NULL))
					{
						buffer[0] = FAILED;
						send(sock_users[i], (void*)&buffer, 1, 0);
						continue;
					}
					buffer[0] = SUCCESS;
					int *temp = (int*)result;
					*temp = htonl(breakpoint);
					if (send(sock_users[i], (void*)&buffer, sizeof(int) + 1, 0) < 1)
					{
						fclose(fp);
						buffer[0] = FAILED;
						send(sock_users[i], (void*)&buffer, 1, 0);
						continue;
					}
					breakpoint = 0;
					int ret = recv(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0);
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
									if (last == '\r' && buffer[i] != '\n')
									{
										fputc('\r', fp);
									}
									if (buffer[i] != '\r')
									{
										fputc(buffer[i], fp);
									}
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
									if (last == '\r' && buffer[i] != '\n')
									{
										fputc('\r', fp);
									}
									if (buffer[i] != '\r')
									{
										fputc(buffer[i], fp);
									}
									last = buffer[i];
								}
							}
							temp_name[strlen(temp_name)] = '.';
							if (access(temp_name, F_OK) == 0)
							{
								remove(temp_name);
							}
							temp_name[strlen(temp_name) - 5] = '\0';
							break;
						}
						else
						{
							break;
						}
						ret = recv(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0);
					}
					if (buffer[0] != DATA && buffer[0] != END || ret < 1)
					{
						temp_name[strlen(temp_name)] = '.';
						FILE *tfp;
						if ((tfp = fopen(temp_name, "w")) != NULL)
						{
							fwrite(&breakpoint, sizeof(int), 1, tfp);
							if (mode == ASCII)
							{
								fputc(last, tfp);
							}
							fclose(tfp);
						}
						fclose(fp);
						continue;
					}
					fclose(fp);
					while (chmod(temp_name, S_IRUSR | S_IWUSR | S_IRGRP) == -1);
					while (chown(temp_name, pw->pw_uid, pw->pw_gid) == -1);
				}
				else
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
				}
			}
			else if (buffer[0] == GET)
			{
				char *result = buffer + 1;
				if (strchr(result, '/') != NULL || strcmp(result, ".") == 0 || strcmp(result, "..") == 0)
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
					continue;
				}
				char filename[STRING_LENGTH];
				strcpy(filename, current_path);
				strcat(filename, "/");
				strcat(filename, result);
				if (access(filename, R_OK) == -1)
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
					continue;
				}
				struct passwd *pw;
				struct stat buf;
				if ((pw = getpwnam(users[i])) == NULL || stat(filename, &buf) == -1)
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
					continue;
				}
				if (buf.st_gid == pw->pw_gid)
				{
					buffer[0] = SUCCESS;
					if (send(sock_users[i], (void*)&buffer, 1, 0) < 1)
					{
						buffer[0] = FAILED;
						send(sock_users[i], (void*)&buffer, 1, 0);
						continue;
					}
					if (recv(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) < 1)
					{
						buffer[0] = FAILED;
						send(sock_users[i], (void*)&buffer, 1, 0);
						continue;
					}
					if (buffer[0] != SUCCESS)
					{
						continue;
					}
					int *temp = (int*)result;
					int breakpoint = ntohl(*temp);
					FILE *fp;
					if ((fp = fopen(filename, "rb")) == NULL)
					{
						buffer[0] = FAILED;
						send(sock_users[i], (void*)&buffer, 1, 0);
						continue;
					}
					pthread_mutex_lock(&mem_mutex);
					int fi;
					for (fi = 0; fi < BACKLOG; fi++)
					{
						if (files[fi].exist == 1)
						{
							if (files[fi].f_ino == buf.st_ino && files[fi].f_mtime == buf.st_mtime)
							{
								files[fi].times++;
								break;
							}
						}
						else
						{
							files[fi].exist = 1;
							files[fi].f_ino = buf.st_ino;
							files[fi].f_mtime = buf.st_mtime;
							files[fi].times = 1;
							break;
						}
					}
					if (most == NULL || fi < BACKLOG && files[fi].times > files[max].times)
					{
						if (most != NULL)
						{
							free(most);
						}
						max = i;
						if (buf.st_size > 0)
						{
							most = malloc(buf.st_size);
							while (fread(most, sizeof(char), buf.st_size, fp) < 1)
							{
								fseek(fp, 0, SEEK_SET);
							}
						}
						else
						{
							most = malloc(1);
							*most = 0;
						}
					}
					if (fi >= BACKLOG)
					{
						int min = 0;
						for (fi = 0; fi < BACKLOG; fi++)
						{
							min = files[fi].times < files[min].times ? fi : min;
						}
						files[min].f_ino = buf.st_ino;
						files[min].f_mtime = buf.st_mtime;
						files[min].times = 1;
					}
					fseek(fp, breakpoint, SEEK_SET);
					buffer[0] = DATA;
					int size;
					if (most != NULL && buf.st_ino == files[max].f_ino && buf.st_mtime == files[max].f_mtime)
					{
						fclose(fp);
						int finished = breakpoint, unfinished = buf.st_size - breakpoint;
						while (unfinished >= BUFFER_SIZE - sizeof(int) - 1)
						{
							memcpy(buffer + sizeof(int) + 1, most + finished, BUFFER_SIZE - sizeof(int) - 1);
							finished += BUFFER_SIZE - sizeof(int) - 1;
							unfinished -= BUFFER_SIZE - sizeof(int) - 1;
							if (send(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) < 1)//此处可以测试断点
							{
								buffer[0] = FAILED;
								send(sock_users[i], (void*)&buffer, 1, 0);
								break;
							}
						}
						if (buffer[0] != FAILED)
						{
							buffer[0] = END;
							*temp = htonl(unfinished);
							memcpy(buffer + sizeof(int) + 1, most + finished, unfinished);
							if (send(sock_users[i], (void*)&buffer, unfinished + sizeof(int) + 1, 0) >= 1)
							{
								pthread_mutex_unlock(&mem_mutex);
								buffer[0] = FAILED;
								send(sock_users[i], (void*)&buffer, 1, 0);
								continue;
							}
						}
						pthread_mutex_unlock(&mem_mutex);
					}
					else
					{
						pthread_mutex_unlock(&mem_mutex);
						while ((size = fread(buffer + sizeof(int) + 1, sizeof(char), BUFFER_SIZE - sizeof(int) - 1, fp)) == BUFFER_SIZE - sizeof(int) - 1)
						{
							if (send(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) < 1)//此处可以测试断点
							{
								buffer[0] = FAILED;
								send(sock_users[i], (void*)&buffer, 1, 0);
								break;
							}
						}
						fclose(fp);
						if (buffer[0] != FAILED)
						{
							buffer[0] = END;
							*temp = htonl(size);
							if (send(sock_users[i], (void*)&buffer, size + sizeof(int) + 1, 0) < 1)
							{
								buffer[0] = FAILED;
								send(sock_users[i], (void*)&buffer, 1, 0);
								continue;
							}
						}
					}
				}
				else
				{
					buffer[0] = FAILED;
					send(sock_users[i], (void*)&buffer, 1, 0);
				}
				// char *result = buffer + 1;
				// if (strchr(result, '/') != NULL || strcmp(result, ".") == 0 || strcmp(result, "..") == 0)
				// {
				// 	buffer[0] = FAILED;
				// 	send(sock_users[i], (void*)&buffer, 1, 0);
				// 	continue;
				// }
				// char filename[STRING_LENGTH];
				// strcpy(filename, current_path);
				// strcat(filename, "/");
				// strcat(filename, result);
				// if (access(filename, R_OK) == -1)
				// {
				// 	buffer[0] = FAILED;
				// 	send(sock_users[i], (void*)&buffer, 1, 0);
				// 	continue;
				// }
				// struct passwd *pw;
				// struct stat buf;
				// if ((pw = getpwnam(users[i])) == NULL || stat(filename, &buf) == -1)
				// {
				// 	buffer[0] = FAILED;
				// 	send(sock_users[i], (void*)&buffer, 1, 0);
				// 	continue;
				// }
				// if (buf.st_gid == pw->pw_gid)
				// {
				// 	buffer[0] = SUCCESS;
				// 	if (send(sock_users[i], (void*)&buffer, 1, 0) < 1)
				// 	{
				// 		buffer[0] = FAILED;
				// 		send(sock_users[i], (void*)&buffer, 1, 0);
				// 		continue;
				// 	}
				// 	if (recv(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) < 1)
				// 	{
				// 		buffer[0] = FAILED;
				// 		send(sock_users[i], (void*)&buffer, 1, 0);
				// 		continue;
				// 	}
				// 	if (buffer[0] != SUCCESS)
				// 	{
				// 		continue;
				// 	}
				// 	int *temp = (int*)result;
				// 	int breakpoint = ntohl(*temp);
				// 	FILE *fp;
				// 	if ((fp = fopen(filename, "rb")) == NULL)
				// 	{
				// 		buffer[0] = FAILED;
				// 		send(sock_users[i], (void*)&buffer, 1, 0);
				// 		continue;
				// 	}
				// 	fseek(fp, breakpoint, SEEK_SET);
				// 	buffer[0] = DATA;
				// 	int size;
				// 	while ((size = fread(buffer + sizeof(int) + 1, sizeof(char), BUFFER_SIZE - sizeof(int) - 1, fp)) == BUFFER_SIZE - sizeof(int) - 1)
				// 	{
				// 		if (send(sock_users[i], (void*)&buffer, BUFFER_SIZE, 0) < 1)
				// 		{
				// 			buffer[0] = FAILED;
				// 			send(sock_users[i], (void*)&buffer, 1, 0);
				// 			break;
				// 		}
				// 	}
				// 	fclose(fp);
				// 	if (buffer[0] != FAILED)
				// 	{
				// 		buffer[0] = END;
				// 		*temp = htonl(size);
				// 		if (send(sock_users[i], (void*)&buffer, size + sizeof(int) + 1, 0) < 1)
				// 		{
				// 			buffer[0] = FAILED;
				// 			send(sock_users[i], (void*)&buffer, 1, 0);
				// 			continue;
				// 		}
				// 	}
				// }
				// else
				// {
				// 	buffer[0] = FAILED;
				// 	send(sock_users[i], (void*)&buffer, 1, 0);
				// }
			}
			else if (buffer[0] == QUIT)
			{
				pthread_detach(pthread_self());
				close(sock_users[i]);
				pthread_mutex_lock(&current_mutex);
				current--;
				pthread_mutex_unlock(&current_mutex);
				users[i][0] = '\0';
				break;
			}
		}
		else
		{
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			pthread_detach(pthread_self());
			close(sock_users[i]);
			pthread_mutex_lock(&current_mutex);
			current--;
			pthread_mutex_unlock(&current_mutex);
			users[i][0] = '\0';
			break;
		}
	}
}

void *handle(void *sockfd)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	while (1)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_testcancel();
		struct sockaddr_in remote_addr;
		int len = sizeof(struct sockaddr_in);
		int client_sockfd = accept(*(int*)sockfd, (struct sockaddr*)&remote_addr, &len);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		if (current >= BACKLOG)
		{
			close(client_sockfd);
			continue;
		}
		if (client_sockfd != -1)
		{
			for (int i = 0; i < BACKLOG; i++)
			{
				if (users[i][0] == '\0')
				{
					strcpy(users[i], "guest");
					all++;
					sock_users[i] = client_sockfd;
					pthread_create(&tid_users[i], NULL, client, (void*)&tid_users[i]);
					break;
				}
			}
		}
	}
}

void *task(void *sockfd)
{
	while (1)
	{
		printf("ftpserver> ");
		char cmd[STRING_LENGTH];
		char c;
		for (int i = 0; i < STRING_LENGTH + 1; i++)
		{
			if (i == STRING_LENGTH)
			{
				printf("ERROR: Command must be less than %d charcaters!\nftpserver> ", STRING_LENGTH - 1);
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

		if (strcmp(cmd, "count") == 0)
		{
			char pathname[STRING_LENGTH];
			char c = read_next(pathname);

			if (c != 0)
			{
				if (strcmp(pathname, "current") == 0)
				{
					printf("Number of currently active users: %d\n", current);
				}
				else if (strcmp(pathname, "all") == 0)
				{
					printf("Total system visitors: %d\n", all);
				}
				else
				{
					printf("ERROR: Invalid parameter input. (count current|all)\n");
				}
			}
		}
		else if (strcmp(cmd, "list") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				for (int i = 0; i < BACKLOG; i++)
				{
					if (users[i][0] != '\0')
					{
						printf("%s\n", users[i]);
					}
				}
			}
			else
			{
				while ((c = getchar()) != '\n' && c != EOF);
				printf("ERROR: Invalid parameter input. (list)\n");
			}
		}
		else if (strcmp(cmd, "kill") == 0)
		{
			char pathname[STRING_LENGTH];
			char c = read_next(pathname);

			if (c != 0)
			{
				for (int i = 0; i < BACKLOG; i++)
				{
					if (strcmp(users[i], pathname) == 0)
					{
						void *tret;
						pthread_cancel(tid_users[i]);
						if (pthread_join(tid_users[i], &tret) == 0)
						{
							close(sock_users[i]);
							pthread_mutex_lock(&current_mutex);
							current--;
							pthread_mutex_unlock(&current_mutex);
							users[i][0] = '\0';
						}
						break;
					}
					if (i == BACKLOG - 1)
					{
						printf("ERROR: Username couldn't found!\n");
					}
				}
			}
		}
		else if (strcmp(cmd, "quit") == 0)
		{
			if (c == '\n' || c == EOF)
			{
				if (pthread_cancel(tid_handle) != 0)
				{
					printf("ERROR: ftpserver quit failed!\n");
					continue;
				}
				void *tret;
				while (pthread_join(tid_handle, &tret) != 0);
				close(*(int*)sockfd);
				for (int i = 0; i < BACKLOG; i++)
				{
					if (users[i][0] != '\0')
					{
						pthread_cancel(tid_users[i]);
						if (pthread_join(tid_users[i], &tret) == 0)
						{
							close(sock_users[i]);
						}
					}
				}
				if (most != NULL)
				{
					free(most);
				}
				pthread_mutex_destroy(&current_mutex);
				pthread_mutex_destroy(&path_mutex);
				pthread_mutex_destroy(&mem_mutex);
				break;
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

void ftpserver(char *ip, int port)
{
	if (access("/usr/local/share/ftpserver/ftp", F_OK) == -1)
	{
		if (mkdir("/usr/local/share/ftpserver/ftp", S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		{
			printf("ERROR: Loading data failed!\n");
			return;
		}
		if (chmod("/usr/local/share/ftpserver/ftp", S_IRWXU | S_IRWXG | S_IRWXO) == -1)
		{
			printf("ERROR: Loading data failed!\n");
			return;
		}
		//不知道为什么不加上这句会导致创建出来的文件权限不对
	}
	if (chmod("/usr/local/share/ftpserver/users", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1)
	{
		printf("ERROR: Loading data failed!\n");
		return;
	}
	//初始化ftp数据

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

	struct sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = inet_addr(ip);
	local_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr_in)) == -1)
	{
		printf("ERROR: Binding service failed!\n");
		return;
	}
	//绑定服务器

	if (listen(sockfd, BACKLOG) == -1)
	{
		close(sockfd);
		printf("ERROR: Listening request failed!\n");
		return;
	}
	//设定监听

	memset(users, '\0', sizeof(users));
	memset(files, 0, sizeof(files));
	if (chdir("/usr/local/share/ftpserver/ftp") == -1)
	{
		close(sockfd);
		printf("ERROR: Loading data failed!\n");
		return;
	}
	while (getcwd(work_path, STRING_LENGTH) == NULL);
	if (pthread_create(&tid_handle, NULL, handle, (void*)&sockfd) != 0)
	{
		close(sockfd);
		printf("ERROR: Creating handle failed!\n");
		return;
	}
	printf("ftpserver is running on %s:%d\n", ip, port);
	if (pthread_create(&tid_task, NULL, task, (void*)&sockfd) != 0)
	{
		close(sockfd);
		printf("ERROR: Creating task failed!\n");
		return;
	}

	void *tret;
	while (pthread_join(tid_task, &tret) != 0);
}
