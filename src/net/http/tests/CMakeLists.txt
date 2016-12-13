add_executable(HttpServer_test HttpServer_test.cc)
target_link_libraries(HttpServer_test http)

add_executable(HttpRequest HttpRequest.cc)
target_link_libraries(HttpRequest http)

