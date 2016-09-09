add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(mysql_client STATIC ${SRCS})
target_link_libraries(mysql_client base mysqlclient)

set(HEADERS
Row.h
Query.h
Connection.h
ConnectionPool.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/mysql_client)
install(TARGETS mysql_client DESTINATION lib)
