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
target_link_libraries(EventLoopThread_test net)

add_executable(TimerQueue_test TimerQueue_test.cc)
target_link_libraries(TimerQueue_test net)