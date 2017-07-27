add_subdirectory(http)
add_subdirectory(inspect)
add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(net STATIC ${SRCS})
target_link_libraries(net base event.a event_openssl.a event_pthreads.a ssl)

set(HEADERS
  config.h
  Buffer.h
  Endian.h
  EventLoop.h
  EventLoopThread.h
  EventLoopThreadPool.h
  InetAddress.h
  TcpClient.h
  TcpConnection.h
  TcpServer.h
  Timer.h
  TimerQueue.h
  LengthHeaderCodec.h
  SslContext.h
  Ssl.h
  )
install(FILES ${HEADERS} DESTINATION include/net)
install(TARGETS net DESTINATION lib)
