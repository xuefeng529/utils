add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(http_lite STATIC ${SRCS})
target_link_libraries(http_lite base)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/plugins/http_lite)
install(TARGETS http_lite DESTINATION lib)
