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

void *send_message(void *arg);
void *recv_message(void *arg);
void error_handling(char *message);

void sock_read(int sock, void *buf, size_t size);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;

    char message[BUFSIZE];
    char buf[BUFSIZE];
    char *ftp_cmd;
    char *ftp_arg;

    if (argc != 3)
    {
        printf("Usage : <IP><port>\n");
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("100 error : server connection error");

    chdir("client_files");

    while (true)
    {
        memset(message, 0, BUFSIZE);

        scanf("%[^\n]", message);
        getchar();

        if (!strcmp(message, "q"))
        { /* 'q' 입력 시 종료 */
            close(sock);
            exit(0);
        }
        // printf("scanf 값 : %s\n", message); // 디버깅용 코드
        ftp_cmd = strtok(message, " ");

        if (ftp_cmd == NULL)
            continue;

        /* 클라이언트에서의 명령어별 예외처리 및 동작 */
        if (!strcmp(ftp_cmd, "ls"))
        {
            if(strtok(NULL, " ") != NULL)
                printf("ls 명령어는 명령 인수를 사용할 수 없습니다!\n");
            else
            {
                write(sock, ftp_cmd, sizeof(ftp_cmd));
                sock_read(sock, buf, BUFSIZE);
            }
        }
        else if (!strcmp(ftp_cmd, "get"))
        {
            ftp_arg = strtok(NULL, " ");
            if(ftp_arg == NULL)
                printf("다운받을 파일명을 입력하세요!.\n");
            else
            {
                write(sock, ftp_cmd, sizeof(ftp_cmd));
                write(sock, ftp_arg, BUFSIZE);
                sock_read(sock, buf, BUFSIZE);
            }
        }
        else if (!strcmp(ftp_cmd, "put"))
        {
            ftp_arg = strtok(NULL, " ");
            if(ftp_arg == NULL)
                printf("업로드할 파일명을 입력하세요!.\n");
            else
            {
                write(sock, ftp_cmd, sizeof(ftp_cmd));
                write(sock, ftp_arg, BUFSIZE);
                sock_read(sock, buf, BUFSIZE);
            }
        }
        else
        {
            printf("지원하지 않는 명령어를 입력하였습니다. 다시 입력하세요!\n");
        }

    }


    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void sock_read(int sock, void *buf, size_t size)
{
    while (true)
    {
        read(sock, buf, BUFSIZE);

        if (!strcmp(buf, ""))
            break;
        printf("%s", buf);
    }
}