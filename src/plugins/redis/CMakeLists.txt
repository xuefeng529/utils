add_library(redis_cli Client.cc)
target_link_libraries(redis_cli base hiredis)

set(HEADERS
	Client.h
)

install(FILES ${HEADERS} DESTINATION include/plugins/redis)
install(TARGETS redis_cli DESTINATION lib)

add_library(redis_cluster_cli ClusterClient.cc)
target_link_libraries(redis_cluster_cli base hiredis_vip.a)

set(CLUSTER_HEADERS
	ClusterClient.h
)

install(FILES ${CLUSTER_HEADERS} DESTINATION include/plugins/redis)
install(TARGETS redis_cluster_cli DESTINATION lib)

add_executable(RedisClient_test Client_test.cc)
target_link_libraries(RedisClient_test redis_cli net base)

add_executable(ClusterClient_test ClusterClient_test.cc)
target_link_libraries(ClusterClient_test redis_cluster_cli base)
