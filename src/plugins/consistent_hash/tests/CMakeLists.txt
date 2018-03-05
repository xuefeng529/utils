aux_source_directory(. SRCS)
add_executable(ConsistentHash_test ${SRCS})
target_link_libraries(ConsistentHash_test consistent_hash)