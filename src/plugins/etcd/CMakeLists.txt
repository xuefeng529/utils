#add_subdirectory(tests)

add_library(etcd STATIC JsonHelper.cc LeaderSelector.cc)
target_link_libraries(etcd base)

add_executable(LeaderSelector_etcd_test LeaderSelector_test.cc)
target_link_libraries(LeaderSelector_etcd_test etcd curlpp base)

set(HEADERS
LeaderSelector.h
)

install(FILES ${HEADERS} DESTINATION include/plugins/etcd)
install(TARGETS etcd DESTINATION lib)
