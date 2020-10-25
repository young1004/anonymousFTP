#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFSIZE 100

void *clnt_connection(void *arg);
void send_message(char *message, int len);
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
    char message[BUFSIZE];
    char buf[BUFSIZE];
    char command[BUFSIZE];
    char *fname;

    FILE *fp;

    while ((str_len = read(clnt_sock, message, sizeof(message))) != 0)
    {
        // 초기화
        strcpy(command, "");
        fname = NULL;

        // 명령어와 인수 분리
        strcpy(command, strtok(message, " "));
        fname = strtok(NULL, " ");
        // cd, ls, pwd, *hash 구현 예정
        if(!strcmp(command, "ls"))
        {
            if(fname == NULL)
            {
                system("ls>temp.txt");
                fp = fopen("temp.txt", "r");

                while(1)
                {
                    strcpy(buf, "");
                    fscanf(fp, "%[^\n]\n", buf);
                    if(!strcmp(buf, ""))
                        break;

                    write(clnt_sock, buf, BUFSIZE);
                }
                
            }
            else 
                write(clnt_sock, "ls 명령에는 인수를 줄 수 없습니다!", BUFSIZE);
        }
        else if(!strcmp(command, "get"))
        {
            if(fname != NULL)
            {
                write(clnt_sock, "get 명령 실행", BUFSIZE);
            }
            else 
                write(clnt_sock, "다운받은 파일명을 입력하세요!", BUFSIZE);
        }
        else if(!strcmp(command, "put"))
        {
            if(fname != NULL)
            {
                write(clnt_sock, "put 명령 실행", BUFSIZE);
            }
            else 
                write(clnt_sock, "업로드할 파일명을 입력하세요!", BUFSIZE);
        }
        else
        {
            write(clnt_sock, "잘못된 명령", BUFSIZE);
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

void send_message(char *message, int len)
{
    // pthread_mutex_lock(&mutx); // 필요 x!
    for (int i = 0; i < clnt_number; i++)
        write(clnt_socks[i], message, len);
    // pthread_mutex_unlock(&mutx); // 필요 x!
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}