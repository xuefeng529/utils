add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(http2 ${SRCS})
target_link_libraries(http2 net)

set(HEADERS
  HttpRequest.h
  HttpResponse.h
  HttpClient.h
  HttpServer.h
  )
install(FILES ${HEADERS} DESTINATION include/net/http)
install(TARGETS http2 DESTINATION lib)
