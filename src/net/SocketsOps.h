/*
 * @Author: py.wang 
 * @Date: 2019-05-25 09:19:08 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:12:28
 */

#ifndef MUDUO_NET_SOCKETSOPS_H
#define MUDUO_NET_SOCKETSOPS_H

#include <arpa/inet.h>

namespace muduo
{

namespace net
{

namespace sockets
{
/****************************************
 * helper routine
 ******************************************/

// create a non-blocking socket file descriptor
int createNonBlockingOrDie();

int connect(int sockfd, const struct sockaddr_in &addr);
void bindOrDie(int sockfd, const struct sockaddr_in &addr);
void listenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in *addr);
ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);
void close(int sockfd);
void shutDownWrite(int sockfd);

void toIpPort(char *buf, size_t size, const struct sockaddr_in &addr);
void toIp(char *buf, size_t size, const struct sockaddr_in &addr);
void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);

int getSocketError(int sockfd);

struct sockaddr_in getLocalAddr(int sockfd);
struct sockaddr_in getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);

}   // namespace sockets

}   // namespace net

}   // namespace muduo

#endif  // MUDUO_NET_SOCKETSOPS_H
