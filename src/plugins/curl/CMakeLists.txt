add_library(curlpp STATIC CurlInitializer HttpClient.cc)
target_link_libraries(curlpp base curl)

add_executable(CurlHttpClient_test HttpClient_test.cc)
target_link_libraries(CurlHttpClient_test base curlpp)

set(HEADERS
HttpClient.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/curl)
install(TARGETS curlpp DESTINATION lib)
