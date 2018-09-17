#add_subdirectory(tests)

#aux_source_directory(. SRCS)
add_library(redis_cli Client.cc)
target_link_libraries(redis_cli base hiredis)

set(HEADERS
  Client.h
  )
install(FILES ${HEADERS} DESTINATION include/plugins/redis)
install(TARGETS redis_cli DESTINATION lib)

add_executable(RedisClient_test Client_test.cc)
target_link_libraries(RedisClient_test redis_cli net base hiredis)
