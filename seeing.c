struct stat file_info;
    int fd;
    int size;

    char buf[BUFSIZE];
    char file_buf[BUFSIZE];

ssize_t str_len;

        // sleep(10); //mutex test용 코드
        printf( "파일을 전송합니다.\n");
        write(sock, &size, sizeof(int));

        while (0 < (str_len = read(fd, file_buf, BUFSIZE - 1)))
        {
            file_buf[str_len]  = '\0';
            write(sock, file_buf, BUFSIZE);
            memset(file_buf, 0, BUFSIZE);
        }
        end_write(sock);

        close(fd);
        for (int i = 0; i < list_number; i++)
        {
            if (!strcmp(mutx_lists[i], buf))
            {
                for (; i < list_number - 1; i++)
                    strcpy(mutx_lists[i], mutx_lists[i + 1]);

                break;
            }
        }