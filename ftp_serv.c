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
#define MAXSIZE 150
#define MAXUSERS 10
#define IPSIZE 20

#define LGDIR "/home/mylinux/anonymous_FTP/server_logs"
#define RED "\x1b[31m"
#define YELLO "\x1b[33m"
#define RESET_COLOR "\x1b[0m"

void *clnt_connection(void *arg);
void error_handling(char *message);

void end_write(int sock);

void ls_func(int sock, char *filename);
void get_func(int sock);
void put_func(int sock);
int get_mutx_no(char *filename);

void get_ipaddr(int sock, char *buf);
void get_now_time(struct tm *nt);
void write_log(char *message, char *logdir, bool flag);

int clnt_number = 0;
int clnt_socks[MAXUSERS];

int list_number = 0;
char mutx_lists[MAXUSERS][BUFSIZE];

pthread_mutex_t mutx;
pthread_mutex_t file_mutex[MAXUSERS];

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
    for (int i = 0; i < MAXUSERS; i++)
        if(pthread_mutex_init(&file_mutex[i], NULL))
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
            write(clnt_sock, "FTP 서버에 접속되었습니다.\n", BUFSIZE);
            end_write(clnt_sock);

            pthread_mutex_lock(&mutx);
            clnt_socks[clnt_number++] = clnt_sock;
            pthread_mutex_unlock(&mutx);
            pthread_create(&thread, NULL, clnt_connection, (void *)clnt_sock);
            printf("new client access. client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
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
    // 클라이언트 별 스레드 동작을 위한 변수들
    int clnt_sock = (int)arg;
    char ftp_cmd[MAXSIZE];
    char ftp_arg[BUFSIZE];
    char buf[BUFSIZE];
    char tmp_name[MAXSIZE];
    int str_len = 0;

    // log 파일 작성을 위한 변수들
    char clnt_ip[IPSIZE] = {0};
    char log_dir[MAXSIZE];
    char log_msg[400];

    sprintf(log_dir, LGDIR);
    get_ipaddr(clnt_sock, clnt_ip);
    // printf("%s\n", clnt_ip);

    sprintf(tmp_name, ".txt%d", clnt_sock);

    sprintf(log_msg, "client [%d] access. client ip : %s\n", clnt_sock, clnt_ip);
    write_log(log_msg, log_dir, true);

    while ((str_len = read(clnt_sock, ftp_cmd, MAXSIZE)) != 0)
    {
        // cd, ls, pwd, *hash 구현 예정
        if (!strcmp(ftp_cmd, "ls")) // ls 명령 구현
        {
            // printf("ls 명령 실행.\n");
            sprintf(log_msg, "client [%d] use [ls] command, ip : %s\n", clnt_sock, clnt_ip);
            write_log(log_msg, log_dir, true);

            sprintf(buf, "ls>%s", tmp_name);
            system(buf);

            ls_func(clnt_sock, tmp_name); // ls 명령 실행용 함수
        }
        else if (!strcmp(ftp_cmd, "get")) // get 명령 구현
        {
            sprintf(log_msg, "client [%d] use [get] command, ip : %s\n", clnt_sock, clnt_ip);
            write_log(log_msg, log_dir, true);

            sprintf(buf, "ls>%s", tmp_name);
            system(buf);

            ls_func(clnt_sock, tmp_name); // ls 명령 실행용 함수

            get_func(clnt_sock);
        }
        else if (!strcmp(ftp_cmd, "put"))
        {
            // printf("put 명령 실행.\n");
            sprintf(log_msg, "client [%d] use [put] command, ip : %s\n", clnt_sock, clnt_ip);
            write_log(log_msg, log_dir, true);

            sprintf(buf, "ls>%s", tmp_name);
            system(buf);

            ls_func(clnt_sock, tmp_name); // ls 명령 실행용 함수

            put_func(clnt_sock);
        }
    }

    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_number; i++)
    { /* 클라이언트 연결 종료 시 */
        sprintf(log_msg, "client [%d] disconnected , ip : %s\n", clnt_sock, clnt_ip);
        write_log(log_msg, log_dir, false);
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

    write(sock, "[서버 파일 목록]\n", BUFSIZE);
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
 */
void get_func(int sock)
{
    struct stat file_info;
    int fd;
    int size;

    char buf[BUFSIZE];

    char clnt_ip[IPSIZE] = {0};
    char log_dir[MAXSIZE];
    char log_msg[400];

    sprintf(log_dir, LGDIR);
    get_ipaddr(sock, clnt_ip);

    read(sock, buf, BUFSIZE);

    stat(buf, &file_info);
    fd = open(buf, O_RDONLY);
    size = file_info.st_size;

    if (fd == -1) // 파일이 없을때
    {
        printf("요청하는 파일이 없습니다. 파일 전송에 실패하였습니다.\n");
        size = 0;
        sprintf(log_msg, "client [%d] file [get] failed, ip : %s\n", sock, clnt_ip);
        write_log(log_msg, log_dir, true);

        write(sock, &size, sizeof(int));
    }
    else // 파일 존재 시
    {
        pthread_mutex_lock(&mutx);
        strcpy(mutx_lists[list_number++], buf);
        pthread_mutex_unlock(&mutx);

        pthread_mutex_lock(&file_mutex[get_mutx_no(buf)]);
        sleep(10);
        printf("파일을 전송합니다.\n");
        write(sock, &size, sizeof(int));
        sendfile(sock, fd, NULL, size);
        pthread_mutex_unlock(&file_mutex[get_mutx_no(buf)]);

        pthread_mutex_lock(&mutx);
        for(int i = 0; i < MAXUSERS; i++)
        {
            if (!strcmp(mutx_lists[i], buf))
            {
                for(; i < list_number - 1; i++)
                    strcpy(mutx_lists[i], mutx_lists[i+1]);

                break;
            }
        }
        list_number--;
        pthread_mutex_unlock(&mutx);
    }
}

/** 클라이언트의 요청을 받아 클라이언트로부터 파일을 전송받는 함수
 * @param   sock 파일을 전송받을 클라언트 소켓 번호
 */
void put_func(int sock)
{
    struct stat file_info;
    int fd;
    int size;
    char *file_data;

    char ftp_arg[BUFSIZE];
    char buf[BUFSIZE];

    char clnt_ip[IPSIZE] = {0};
    char log_dir[MAXSIZE];
    char log_msg[400];

    sprintf(log_dir, LGDIR);
    get_ipaddr(sock, clnt_ip);

    read(sock, &size, sizeof(int));

    if (!size) // 클라이언트로부터 받은 파일 크기가 0인경우
    {
        printf("클라이언트가 파일 전송에 실패했습니다!.\n");
        sprintf(log_msg, "client [%d] file [put] failed, ip : %s\n", sock, clnt_ip);
        write_log(log_msg, log_dir, true);
    }
    else
    {
        printf("클라이언트로부터 파일을 받았습니다.\n");
        read(sock, ftp_arg, BUFSIZE);

        file_data = malloc(size);
        read(sock, file_data, size);

        fd = open(ftp_arg, O_CREAT | O_EXCL | O_WRONLY, 0666);

        if(fd == -1)
        {
            sprintf(buf, "%s250 : File Existed\n%s", RED, RESET_COLOR);
            write(sock, buf, BUFSIZE);
            end_write(sock);
        }
        else
        {
            int put_size;
            write(fd, file_data, size);

            stat(ftp_arg, &file_info);
            put_size = file_info.st_size;

            if(size == put_size)
            {
                printf("파일을 정상적으로 전송받았습니다!\n");
                write(sock, "파일이 정상적으로 전송되었습니다!\n", BUFSIZE);
                end_write(sock);
            }
            else
            {
                printf( RED "비정상적인 파일 업로드!\n" RESET_COLOR);
                sprintf(buf, "%s파일이 정상적으로 전송되지 않았습니다!\n%s", RED, RESET_COLOR);
                end_write(sock);
            }
            
        }

        free(file_data);
        close(fd);
    }
}

/** 메시지를 받아 서버의 로그를 기록하는 함수
 * @param message 기록할 로그 메시지 문자열
 * @param logdir  로그파일이 기록될 디렉토리 경로
 * @param flag    mutex lock을 사용할것인지 아닌지에 대한 flag 변수
 */
void write_log(char *message, char *logdir, bool flag)
{
    FILE *log_file;
    char log_dir_name[MAXSIZE];
    char log_file_name[400];
    struct tm *log_time = (struct tm *)malloc(sizeof(struct tm));

    get_now_time(log_time);

    sprintf(log_dir_name, "%s/%d년 %d월", logdir, log_time->tm_year, log_time->tm_mon);
    mkdir(log_dir_name, 0755);

    sprintf(log_file_name, "%s/%d월 %d일.log", log_dir_name, log_time->tm_mon, log_time->tm_mday);

    if (flag)
        pthread_mutex_lock(&mutx);

    log_file = fopen(log_file_name, "a");

    fprintf(log_file, "[%d.%d.%d %d:%d:%d] %s", log_time->tm_year, log_time->tm_mon, log_time->tm_mday, log_time->tm_hour, log_time->tm_min, log_time->tm_sec, message);
    fclose(log_file);

    if (flag)
        pthread_mutex_unlock(&mutx);
}

/** 입력받은 소켓의 ip 주소를 문자열에 저장해주는 함수
 * @param   sock ip 주소를 알고싶은 소켓 번호
 * @param   buf ip 주소를 저장할 문자 배열의 시작 주소
 */
void get_ipaddr(int sock, char *buf)
{
    struct sockaddr_in sockAddr;
    int size;

    size = sizeof(sockAddr);
    memset(&sockAddr, 0, size);
    getpeername(sock, (struct sockaddr *)&sockAddr, &size);

    strcpy(buf, inet_ntoa(sockAddr.sin_addr));
}

/** mutex lock을 걸 mutex 배열의 인덱스를 반환하는 함수
 * @param   filename mutex lock으로 잠굴 파일의 이름
 * @return  mutex lock을 사용할 mutex 배열 인덱스값
 */
int get_mutx_no(char *filename)
{
    for(int i = 0; i < list_number; i++)
    {
        if(!strcmp(mutx_lists[i], filename))
            return i;
    }
}

/** 연결 시간에 대한 정보를 리턴하는 함수
 * @param nt : 시간을 기록하기위해 받아올 struct tm 구조체의 주소
 */
void get_now_time(struct tm *nt)
{
    time_t now_time;
    struct tm *t;

    time(&now_time);
    t = (struct tm *)localtime(&now_time);

    nt->tm_year = t->tm_year + 1900;
    nt->tm_mon = t->tm_mon + 1;
    nt->tm_mday = t->tm_mday;
    nt->tm_hour = t->tm_hour;
    nt->tm_min = t->tm_min;
    nt->tm_sec = t->tm_sec;
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