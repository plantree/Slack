/*
 * @Author: py.wang 
 * @Date: 2019-05-25 08:41:02 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-25 08:54:53
 */
/*****************************************
 * 转换字节序
 *****************************************/
#ifndef NET_ENDIAN_H
#define NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace slack
{

namespace net
{

namespace sockets
{

// the inline assembler code makes type blur
// so we disable warnings for a while
#if __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

// 本地字节序到网络字节序，非标准
inline uint64_t hostToNetwork64(uint64_t host64)
{
    return htobe64(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
    return htobe32(host32);
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
    return htobe16(host16);
}

// 网络字节序到本地字节序
inline uint64_t networkToHost64(uint64_t net64)
{
    return be64toh(net64);
}

inline uint32_t networkToHost32(uint32_t net32)
{
    return be32toh(net32);
}

inline uint16_t networkToHost16(uint16_t net16)
{
    return be16toh(net16);
}

#if __GNUC_MINOR__ >= 6
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

}   // namespace sockets

}   // namespace net

}   // namespace slack

#endif  // NET_ENDIAN_H