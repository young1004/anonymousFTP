#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFSIZE 100
#define MAXSIZE 30

void error_handling(char *message);

void sock_read(int sock);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;

    char message[BUFSIZE];
    char buf[BUFSIZE];
    char *ftp_cmd;
    char *ftp_arg;
    int str_len = 0;

    struct stat file_info;
    char *file_data;
    int fd;
    int size = 0;
    // FILE *fp;

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

    sock_read(sock);

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
            if (strtok(NULL, " ") != NULL)
                printf("ls 명령어는 명령 인수를 사용할 수 없습니다!\n");
            else
            {
                write(sock, ftp_cmd, MAXSIZE);
                sock_read(sock);
            }
        }
        else if (!strcmp(ftp_cmd, "get"))
        {
            ftp_arg = strtok(NULL, " ");
            if (ftp_arg == NULL)
                printf("다운받을 파일명을 입력하세요!.\n");
            else
            {
                // printf("cmd : %s\n", ftp_cmd);
                // printf("arg : %s\n", ftp_arg);
                write(sock, ftp_cmd, MAXSIZE);
                write(sock, ftp_arg, BUFSIZE);

                read(sock, &size, sizeof(int));

                if(!size) // size가 0이면
                    printf("200 error : file not found.\n");
                else
                {
                    file_data = malloc(size);
                    read(sock, file_data, size);
                    fd = open(ftp_arg, O_CREAT | O_EXCL | O_WRONLY, 0666);
                    write(fd, file_data, size);

                    free(file_data);
			        close(fd);
                }
                
            }
        }
        else if (!strcmp(ftp_cmd, "put"))
        {
            ftp_arg = strtok(NULL, " ");
            if (ftp_arg == NULL)
                printf("업로드할 파일명을 입력하세요!.\n");
            else if (false) // 클라이언트에 업로드할 파일이 없을 시 처리 구문
            {
            }
            else
            {
                write(sock, ftp_cmd, MAXSIZE);
                write(sock, ftp_arg, BUFSIZE);
                sock_read(sock);
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

/** 에러 발생시 예외처리를 위한 함수
 * @param   message 예외처리시 출력할 에러 메시지가 담긴 문자열의 시작 주소
 */
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

/** 서버의 전송이 끝날때까지 서버로부터 메시지를 읽는 함수
 * @param   sock 메시지를 읽을 서버의 소켓 번호
 */
void sock_read(int sock)
{
    char buf[BUFSIZE];

    while (true)
    {
        read(sock, buf, BUFSIZE);

        if (!strcmp(buf, "")) // 서버가 전송이 끝났음을 의미
            break;
        printf("%s", buf);

        if (!strcmp(buf, "접속을 종료합니다.\n")) // 서버 접속자가 너무 많을 시
        {
            close(sock);
            exit(0);
        }
    }
}