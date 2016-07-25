add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(ssdb_client STATIC ${SRCS})
target_link_libraries(ssdb_client base ssdb-client)

set(HEADERS
  SSDB_client.h
  ConnectionPool.h
  )
install(FILES ${HEADERS} DESTINATION include/plugins/ssdb_client)
install(TARGETS ssdb_client DESTINATION lib)
