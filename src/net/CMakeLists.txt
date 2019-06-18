set(NET_SRCS
    Buffer.cc
    #Channel.cc
    #EventLoop.cc
    #Poller.cc
    #poller/DefaultPoller.cc
    #poller/PollPoller.cc
    #poller/EpollPoller.cc
    #Timer.cc
    #TimerQueue.cc
    #EventLoopThread.cc
    #InetAddress.cc
    #SocketsOps.cc
    #Socket.cc
    #Acceptor.cc
    #TcpConnection.cc
    #TcpServer.cc
    #EventLoopThreadPool.cc
    #Connector.cc
    #TcpClient.cc
    )


add_library(slack_net ${NET_SRCS})
target_link_libraries(slack_net slack_base slack_log)

add_subdirectory(tests)
#add_subdirectory(http)

#install(TARGETS muduo_net DESTINATION lib)
#set(HEADERS
#    EventLoop.h)
#install(FILES ${HEADERS} DESTINATION include/muduo/net)