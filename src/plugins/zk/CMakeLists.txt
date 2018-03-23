add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(zk STATIC ${SRCS})
target_link_libraries(zk base zookeeper_mt)

set(HEADERS
Client.h
LeaderSelector.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/zk)
install(TARGETS zk DESTINATION lib)
