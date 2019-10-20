/* 
 * 提供对socket的读取一行操作
 */


#ifndef READN_H
#define READN_H
ssize_t sock_readline(int sockfd, void *buf, size_t maxlen);
#endif