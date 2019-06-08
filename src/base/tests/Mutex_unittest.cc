/*
 * @Author: py.wang 
 * @Date: 2019-06-08 19:42:57 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-08 20:04:06
 */

#include "src/base/Mutex.h"

#include <pthread.h>
#include <stdio.h>

using namespace slack;

int counter = 0;
int lock_counter = 0;
MutexLock mutex;

// add 1e5
void *add_count(void *arg)
{
    for (int i = 0; i < 1e5; ++i)
    {
        ++counter;
    }
    return NULL;
}

void *add_lock_count(void *arg)
{
    for (int i = 0; i < 1e5; ++i)
    {
        MutexLockGuard lock(mutex);
        ++lock_counter;
    }
    return NULL;
}

int main()
{
    printf("No lock:\n");
    for (int i = 0; i < 10; ++i)
    {
        pthread_t p1, p2;
        pthread_create(&p1, NULL, add_count, NULL);
        pthread_create(&p2, NULL, add_count, NULL);
        pthread_join(p1, NULL);
        pthread_join(p2, NULL);
        printf("counter: %d\n", counter);
    }

    printf("Have lock:\n");
    for (int i = 0; i < 10; ++i)
    {
        pthread_t p1, p2;
        pthread_create(&p1, NULL, add_lock_count, NULL);
        pthread_create(&p2, NULL, add_lock_count, NULL);
        pthread_join(p1, NULL);
        pthread_join(p2, NULL);
        printf("counter: %d\n", lock_counter);
    }
}