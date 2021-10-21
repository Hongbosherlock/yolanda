#include "lib/common.h"

#define MAX_LINE 4096

char
rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

void child_run(int fd) {
    char outbuf[MAX_LINE + 1];
    size_t outbuf_used = 0;
    ssize_t result;

    while (1) {
        char ch;
        result = recv(fd, &ch, 1, 0);
        if (result == 0) {
            break;
        } else if (result == -1) {
            perror("read");
            break;
        }

        /* We do this test to keep the user from overflowing the buffer. */
        if (outbuf_used < sizeof(outbuf)) {
            outbuf[outbuf_used++] = rot13_char(ch);
        }

        if (ch == '\n') {
            send(fd, outbuf, outbuf_used, 0);
            outbuf_used = 0;
            continue;
        }
    }
}


void sigchld_handler(int sig) {
    //选项 WNOHANG 用来告诉内核，即使还有未终止的子进程也不要阻塞在 waitpid 上。
    while (waitpid(-1, 0, WNOHANG) > 0);
    return;
}

int main(int c, char **v) {
    int listener_fd = tcp_server_listen(SERV_PORT);
    signal(SIGCHLD, sigchld_handler);
    while (1) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        int fd = accept(listener_fd, (struct sockaddr *) &ss, &slen);
        if (fd < 0) {
            error(1, errno, "accept failed");
            exit(1);
        }

        if (fork() == 0) {
            //通过判断 fork 的返回值为 0，进入子进程处理逻辑。
            close(listener_fd);// 子进程不关心 listener_fd
            child_run(fd);
            exit(0);
        } else {
            //父进程不关心 accept 后的连接套接字
            close(fd);
        }
    }

    return 0;
}
/*
从父进程派生出的子进程，同时也会复制一份描述字，也就是说，连接套接字和监听套接字的引用计数都会被加 1，
调用 close 函数则会对引用计数进行减 1 操作，这样在套接字引用计数到 0 时，才可以将套接字资源回收。
*/