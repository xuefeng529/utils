aux_source_directory(. SRCS)
add_executable(consistent_hash_test ${SRCS})
target_link_libraries(consistent_hash_test consistent_hash)