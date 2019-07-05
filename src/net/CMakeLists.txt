set(NET_SRCS
    InetAddress.cc
    SocketsOps.cc
    Socket.cc
    Buffer.cc
    Channel.cc
    EventLoop.cc
    Poller.cc
    poller/DefaultPoller.cc
    #poller/PollPoller.cc
    poller/EpollPoller.cc
    Timer.cc
    TimerQueue.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
    #Acceptor.cc
    #TcpConnection.cc
    #TcpServer.cc
    #Connector.cc
    #TcpClient.cc
    )

add_library(slack_net ${NET_SRCS})
target_link_libraries(slack_net slack_log slack_base)

add_subdirectory(tests)
#add_subdirectory(http)

#install(TARGETS muduo_net DESTINATION lib)
#set(HEADERS
#    EventLoop.h)
#install(FILES ${HEADERS} DESTINATION include/muduo/net)