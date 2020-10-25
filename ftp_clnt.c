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

void *send_message(void *arg);
void *recv_message(void *arg);
void error_handling(char *message);

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_result;
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
        error_handling("connect() error");

    chdir("client_files");

    pthread_create(&snd_thread, NULL, send_message, (void *)sock);
    pthread_create(&rcv_thread, NULL, recv_message, (void *)sock);
    pthread_join(snd_thread, &thread_result);
    pthread_join(rcv_thread, &thread_result);
    close(sock);
    return 0;
}

void *send_message(void *arg) /* 메시지 전송 쓰레드 실행 함수 */
{
    int sock = (int)arg;
    char message[BUFSIZE];
    while (1)
    {
        // printf(" lsq: 종료\n");
        scanf("%[^\n]", message);
        getchar();
        if (!strcmp(message, "q"))
        { /* 'q' 입력 시 종료 */
            close(sock);
            exit(0);
        }
        write(sock, message, BUFSIZE);
    }
}

void *recv_message(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
    int sock = (int)arg;
    char message[BUFSIZE];
    int str_len;
    while (1)
    {
        str_len = read(sock, message, BUFSIZE);
        if (str_len == -1)
            return 1;
        message[str_len - 1] = 0;
        printf("%s\n", message);
    }
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}