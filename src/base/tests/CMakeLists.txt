add_executable(timestamp_unittest Timestamp_unittest.cc)
target_link_libraries(timestamp_unittest slack_base)

add_executable(atomic_unittest Atomic_unittest.cc)
target_link_libraries(atomic_unittest slack_base)

add_executable(exception_unittest Exception_unittest.cc)
target_link_libraries(exception_unittest slack_base)

add_executable(thread_unittest Thread_unittest.cc)
target_link_libraries(thread_unittest slack_base)

add_executable(thread_bench Thread_bench.cc)
target_link_libraries(thread_bench slack_base)

add_executable(mutex_unittest Mutex_unittest.cc)
target_link_libraries(mutex_unittest slack_base)

#add_executable(mutex_unittest Mutex_unittest.cc)
#target_link_libraries(mutex_unittest muduo_base)

#add_executable(blockingqueue_unittest BlockingQueue_unittest.cc)
#target_link_libraries(blockingqueue_unittest muduo_base)

#add_executable(blockingqueue_bench BlockingQueue_bench.cc)
#target_link_libraries(blockingqueue_bench muduo_base)

#add_executable(boundedblockingqueue_unittest BoundedBlockingQueue_unittest.cc)
#target_link_libraries(boundedblockingqueue_unittest muduo_base)

#add_executable(threadpool_unittest ThreadPool_unittest.cc)
#target_link_libraries(threadpool_unittest muduo_base)

#add_executable(singleton_unittest Singleton_unittest.cc)
#target_link_libraries(singleton_unittest muduo_base)

#add_executable(threadlocal_unittest ThreadLocal_unittest.cc)
#target_link_libraries(threadlocal_unittest muduo_base)

#add_executable(singletonthreadlocal_unittest SingletonThreadLocal_unittest.cc)
#target_link_libraries(singletonthreadlocal_unittest muduo_base)

#add_executable(threadlocalsingleton_unittest ThreadLocalSingleton_unittest.cc)
#target_link_libraries(threadlocalsingleton_unittest muduo_base)

#if(BOOSTTEST_LIBRARY)
#add_executable(logstream_unittest LogStream_unittest.cc)
#target_link_libraries(logstream_unittest muduo_base boost_unit_test_framework)
#endif()

#add_executable(logstream_bench LogStream_bench.cc)
#target_link_libraries(logstream_bench muduo_base)

#add_executable(logging_unittest Logging_unittest.cc)
#target_link_libraries(logging_unittest muduo_base)

#add_executable(fileutil_unittest FileUtil_unittest.cc)
#target_link_libraries(fileutil_unittest muduo_base)

#add_executable(processinfo_unittest ProcessInfo_unittest.cc)
#target_link_libraries(processinfo_unittest muduo_base)

#add_executable(logfile_unittest LogFile_unittest.cc)
#target_link_libraries(logfile_unittest muduo_base)

#add_executable(asynclogging_unittest AsyncLogging_unittest.cc)
#target_link_libraries(asynclogging_unittest muduo_base)
