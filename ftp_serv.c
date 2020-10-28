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
#define MAXUSERS 10

void *clnt_connection(void *arg);
void error_handling(char *message);

void end_write(int sock);

void ls_func(int sock, char *filename);
void get_func(int sock, char *filename);
void put_func(int sock, char *filename);

int clnt_number = 0;
int clnt_socks[MAXUSERS];
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
        if (clnt_number >= MAXUSERS)
        {
            write(clnt_sock, "접속자가 너무 많습니다.\n", BUFSIZE);
            write(clnt_sock, "접속을 종료합니다.\n", BUFSIZE);
            close(clnt_sock);
        }
        else
        {
            write(clnt_sock, "접속되었습니다.\n", BUFSIZE);
            end_write(clnt_sock);

            pthread_mutex_lock(&mutx);
            clnt_socks[clnt_number++] = clnt_sock;
            pthread_mutex_unlock(&mutx);
            pthread_create(&thread, NULL, clnt_connection, (void *)clnt_sock);
            printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
        }
    }
    return 0;
}

/** 클라이언트 접속시, 생성된 스레드가 동작할 기준이 되는 함수
 * @param   arg 생성된 스레드가 맡을 클라이언트 소켓 번호
 * @return  해당 스레드가 정상 종료되었는지에 대한 상태 코드
 */
void *clnt_connection(void *arg)
{
    int clnt_sock = (int)arg;
    char ftp_cmd[MAXSIZE];
    char ftp_arg[BUFSIZE];
    char buf[BUFSIZE];
    char tmp_name[MAXSIZE];
    int str_len = 0;

    struct stat file_info;
    char *file_data;
    int fd;
    int size = 0;

    // bool hash = true;
    // FILE *fp;

    sprintf(tmp_name, ".txt%d", clnt_sock);

    while ((str_len = read(clnt_sock, ftp_cmd, MAXSIZE)) != 0)
    {
        // cd, ls, pwd, *hash 구현 예정
        if (!strcmp(ftp_cmd, "ls")) // ls 명령 구현
        {
            // printf("ls 명령 실행.\n");
            sprintf(buf, "ls>%s", tmp_name);
            system(buf);

            ls_func(clnt_sock, tmp_name); // ls 명령 실행용 함수
        }
        else if (!strcmp(ftp_cmd, "get")) // get 명령 구현
        {
            // printf("get 명령 실행.\n");
            read(clnt_sock, ftp_arg, BUFSIZE);
            // printf("파일이름 : %s\n", ftp_arg);

            stat(ftp_arg, &file_info);
			fd = open(ftp_arg, O_RDONLY);
			size = file_info.st_size;
				
            if (fd == -1) // 파일이 없을때
            {
                size = 0;
                write(clnt_sock, &size, sizeof(int));
            }
            else // 파일 존재 시
            {
                write(clnt_sock, &size, sizeof(int));
                sendfile(clnt_sock, fd, NULL, size);
            }
        }
        else if (!strcmp(ftp_cmd, "put"))
        {
            // printf("put 명령 실행.\n");
            read(clnt_sock, &size, sizeof(int));

            if(!size) // 클라이언트로부터 받은 파일 크기가 0인경우
            {
                printf("클라이언트가 파일 전송에 실패했습니다!.\n");
                continue;
            }
            else
            {
                read(clnt_sock, ftp_arg, BUFSIZE);
                file_data = malloc(size);
                read(clnt_sock, file_data, size);
                fd = open(ftp_arg, O_CREAT | O_EXCL | O_WRONLY, 0666);
                write(fd, file_data, size);

                free(file_data);
                close(fd);
            }
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

/** 클라이언트에게 모든 메시지를 전송했을 때 호출하는 함수
 * @param   sock 통신이 끝났음을 전송할 클라이언트 소켓 번호
 */
void end_write(int sock)
{
    write(sock, "", 1);
}

/** 클라이언트의 요청을 받아 서버의 파일 목록을 전송하는 함수
 * @param   sock ls 명령의 결과를 전송할 클라이언트 소켓 번호
 * @param   filename ls 명령의 결과가 담겨있는 파일명
 */
void ls_func(int sock, char *filename)
{
    char buf[BUFSIZE];
    FILE *fp;

    fp = fopen(filename, "r");

    write(sock, "[파일 목록]\n", BUFSIZE);
    write(sock, "=====================\n", BUFSIZE);
    while (1)
    {
        strcpy(buf, "");
        fscanf(fp, "%[^\n]\n", buf);
        if (!strcmp(buf, ""))
            break;
        sprintf(buf, "%s\n", buf);

        write(sock, buf, BUFSIZE);
    }
    write(sock, "=====================\n", BUFSIZE);
    end_write(sock);

    fclose(fp);
    remove(filename);
}

/** 클라이언트의 요청을 받아 서버의 파일을 전송하는 함수
 * @param   sock 파일을 전송할 클라언트 소켓 번호
 * @param   filename 전송할 파일의 이름
 */
void get_func(int sock, char *filename)
{
}

/** 클라이언트의 요청을 받아 클라이언트로부터 파일을 전송받는 함수
 * @param   sock 파일을 전송받을 클라언트 소켓 번호
 * @param   filename 전송받을 파일의 이름
 */
void put_func(int sock, char *filename)
{
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