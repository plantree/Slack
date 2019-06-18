set(LOG_SRCS 
    LogStream.cc
    Logging.cc
    LogFile.cc
    AsyncLogging.cc
    )

add_library(slack_log ${LOG_SRCS})
target_link_libraries(slack_log pthread)

#install(TARGETS slack_base DESTINATION lib)
#file(GLOB HEADERS "*.h")
#install(FILES ${HEADERS} DESTINATION include/slack/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
    add_subdirectory(tests)
endif()