aux_source_directory(. SRCS)
add_executable(SSDB_client_test ${SRCS})
target_link_libraries(SSDB_client_test ssdb-client)