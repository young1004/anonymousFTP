#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFSIZE 100
#define MAXSIZE 30

void *clnt_connection(void *arg);
void error_handling(char *message);

int clnt_number = 0;
int clnt_socks[10];
pthread_mutex_t mutx;

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;
    pthread_t thread;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    if (pthread_mutex_init(&mutx, NULL))
        error_handling("mutex init error");
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    chdir("server_files");

    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_number++] = clnt_sock;
        pthread_mutex_unlock(&mutx);
        pthread_create(&thread, NULL, clnt_connection, (void *)clnt_sock);
        printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    return 0;
}

void *clnt_connection(void *arg)
{
    int clnt_sock = (int)arg;
    int str_len = 0;
    char ftp_cmd[MAXSIZE];
    char ftp_arg[BUFSIZE];
    // char serv_msg[BUFSIZE];
    char buf[BUFSIZE];
    char tmp_name[MAXSIZE];
    bool hash = true;

    FILE *fp;

    sprintf(tmp_name, ".txt%d", clnt_sock);

    while ((str_len = read(clnt_sock, ftp_cmd, sizeof(ftp_cmd))) != 0)
    {
        // cd, ls, pwd, *hash 구현 예정
        if (!strcmp(ftp_cmd, "ls")) // ls 명령 구현
        {
            sprintf(buf, "ls>%s", tmp_name);
            system(buf);

            fp = fopen(tmp_name, "r");

            write(clnt_sock, "[파일 목록]\n", BUFSIZE);
            write(clnt_sock, "=====================\n", BUFSIZE);
            while (1)
            {
                strcpy(buf, "");
                fscanf(fp, "%[^\n]\n", buf);
                if (!strcmp(buf, ""))
                    break;
                sprintf(buf, "%s\n", buf);

                write(clnt_sock, buf, BUFSIZE);
            }
            write(clnt_sock, "=====================\n", BUFSIZE);
            write(clnt_sock, "", 1);

            fclose(fp);
            remove(tmp_name);
        }
        else if (!strcmp(ftp_cmd, "get")) // get 명령 구현
        {
            read(clnt_sock, ftp_arg, BUFSIZE);
            if ((fp = fopen("test.txt", "r")) == NULL)
            {
                write(clnt_sock, "200 error : file not found.\n", BUFSIZE);
                write(clnt_sock, "", 1);
                continue;
            }
            else
            {
                write(clnt_sock, "get 명령 실행\n", BUFSIZE);
                write(clnt_sock, "", 1);
            }
        }
        else if (!strcmp(ftp_cmd, "put"))
        {
            read(clnt_sock, ftp_arg, BUFSIZE);

            write(clnt_sock, "put 명령 실행\n", BUFSIZE);
            write(clnt_sock, "", 1);
        }
    }

    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_number; i++)
    { /* 클라이언트 연결 종료 시 */
        if (clnt_sock == clnt_socks[i])
        {
            for (; i < clnt_number - 1; i++)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_number--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}