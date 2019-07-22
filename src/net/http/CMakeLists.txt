set(http_SRCS
    HttpServer.cc
    HttpResponse.cc
    HttpContext.cc)

add_library(slack_http ${http_SRCS})
target_link_libraries(slack_http slack_net)

add_subdirectory(tests)