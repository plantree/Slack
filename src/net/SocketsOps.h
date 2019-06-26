/*
 * @Author: py.wang 
 * @Date: 2019-05-25 09:19:08 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:12:28
 */

#ifndef NET_SOCKETSOPS_H
#define NET_SOCKETSOPS_H

#include <arpa/inet.h>

namespace slack
{

namespace net
{

namespace sockets
{
/****************************************
 * helper routine
 ******************************************/

// create a non-blocking socket file descriptor
int createNonBlockingOrDie(sa_family_t family);

int connect(int sockfd, const struct sockaddr *addr);
void bindOrDie(int sockfd, const struct sockaddr *addr);
void listenOrDie(int sockfd);
// support ipv6
int accept(int sockfd, struct sockaddr_in6 *addr);

ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);

void close(int sockfd);
// 关闭写，依旧可读
void shutDownWrite(int sockfd);

// transformation
void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
void toIp(char *buf, size_t size, const struct sockaddr *addr);
void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr);
void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);

// socket opt
int getSocketError(int sockfd);

// some casts
const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr);
const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
struct sockaddr *sockaddr_cast(struct sockaddr_in6 *addr);
const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr);
const struct sockaddr_in6 *sockaddr_in6_cast(const struct sockaddr *addr);

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);

}   // namespace sockets

}   // namespace net

}   // namespace slack

#endif  // NET_SOCKETSOPS_H
