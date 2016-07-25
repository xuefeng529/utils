add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(inspect ${SRCS})
target_link_libraries(inspect http)

if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  set_target_properties(inspect PROPERTIES COMPILE_FLAGS "-DHAVE_TCMALLOC")
  target_link_libraries(inspect tcmalloc_and_profiler)
endif()

set(HEADERS
  Inspector.h
  )
install(FILES ${HEADERS} DESTINATION include/net/inspect)
install(TARGETS inspect DESTINATION lib)
