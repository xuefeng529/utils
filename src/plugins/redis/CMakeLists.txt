#add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(redis_cli STATIC ${SRCS})
target_link_libraries(redis_cli base hiredis)

set(HEADERS
  Client.h
  )
install(FILES ${HEADERS} DESTINATION include/plugins/redis)
install(TARGETS redis_cli DESTINATION lib)
