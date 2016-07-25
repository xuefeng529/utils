aux_source_directory(. SRCS)
add_executable(http_lite_test ${SRCS})
target_link_libraries(http_lite_test http_lite)