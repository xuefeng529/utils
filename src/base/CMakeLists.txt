add_subdirectory(tests)

add_definitions(-DENDIAN_BIG=1)
aux_source_directory(. SRCS)
add_library(base STATIC ${SRCS})
target_link_libraries(base pthread rt)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/base)
install(TARGETS base DESTINATION lib)
