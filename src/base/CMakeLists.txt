set(BASE_SRCS 
    Timestamp.cc
    Exception.cc
    CurrentThread.cc
    Thread.cc
    CountDownLatch.cc
    Condition.cc
    #ThreadPool.cc
    #LogStream.cc
    #Logging.cc
    #FileUtil.cc
    #ProcessInfo.cc
    #LogFile.cc
    #AsyncLogging.cc
    )

add_library(slack_base ${BASE_SRCS})
target_link_libraries(slack_base pthread)

#install(TARGETS slack_base DESTINATION lib)
#file(GLOB HEADERS "*.h")
#install(FILES ${HEADERS} DESTINATION include/slack/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
    add_subdirectory(tests)
endif()