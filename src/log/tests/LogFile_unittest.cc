/*
 * @Author: py.wang 
 * @Date: 2019-05-16 08:36:31 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 08:41:49
 */

#include "src/log/LogFile.h"
#include "src/log/Logging.h"

#include <unistd.h>

std::unique_ptr<slack::LogFile> g_logfile;

void outputFunc(const char *msg, int len)
{
    g_logfile->append(msg, len);
}

void flushFunc()
{
    g_logfile->flush();
}

int main(int argc, char *argv[])
{
    char name[256] = {0};
    strncpy(name, argv[0], sizeof name - 1);
    printf("%s %s\n", name, ::basename(name));
    g_logfile.reset(new slack::LogFile(::basename(name), 200 * 1000));
    slack::Logger::setOutput(outputFunc);
    slack::Logger::setFlush(flushFunc);

    slack::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    for (int i = 0; i < 10000; ++i)
    {
        LOG_INFO << line << i;
        usleep(1000);
    }
}