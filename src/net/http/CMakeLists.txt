add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(http ${SRCS})
target_link_libraries(http net ssl)

set(HEADERS
  http_parser.h
  HttpRequest.h
  HttpResponse.h
  HttpClient.h
  HttpServer.h
  )
install(FILES ${HEADERS} DESTINATION include/net/http)
install(TARGETS http DESTINATION lib)
