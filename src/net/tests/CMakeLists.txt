#aux_source_directory(. srcs)
add_executable(Buffer_test Buffer_test.cc)
target_link_libraries(Buffer_test net)

add_executable(EventLoop_test EventLoop_test.cc)
target_link_libraries(EventLoop_test net)

add_executable(EchoServer_test EchoServer_test.cc)
target_link_libraries(EchoServer_test net)

add_executable(EchoClient_test EchoClient_test.cc)
target_link_libraries(EchoClient_test net)

add_executable(EventLoopThread_test EventLoopThread_test.cc)
target_link_libraries(EventLoopThread_test net base)

add_executable(TimerQueue_test TimerQueue_test.cc)
target_link_libraries(TimerQueue_test net)

add_executable(CodecServer_test CodecServer_test.cc)
target_link_libraries(CodecServer_test net)

add_executable(CodecClient_test CodecClient_test.cc)
target_link_libraries(CodecClient_test net)

#add_executable(TcpClient_test TcpClient_test.cc)
#target_link_libraries(TcpClient_test net base)

#add_executable(ssl_server ssl_server.cc)
#target_link_libraries(ssl_server ssl)

#add_executable(ssl_client ssl_client.cc)
#target_link_libraries(ssl_client ssl)

add_executable(SslEchoServer_test SslEchoServer_test.cc)
target_link_libraries(SslEchoServer_test net base)

add_executable(SslEchoClient_test SslEchoClient_test.cc)
target_link_libraries(SslEchoClient_test net base)

add_executable(ApnsClient_test ApnsClient_test.cc)
target_link_libraries(ApnsClient_test net base)