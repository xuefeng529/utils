#aux_source_directory(. srcs)
add_executable(Thread_test Thread_test.cc)
target_link_libraries(Thread_test base)

add_executable(ThreadPool_test ThreadPool_test.cc)
target_link_libraries(ThreadPool_test base)

add_executable(Config_test Config_test.cc)
target_link_libraries(Config_test base)

add_executable(UniqueIdGenerator_test UniqueIdGenerator_test.cc)
target_link_libraries(UniqueIdGenerator_test base)

add_executable(any_test any_test.cc)

add_executable(crc32c_test crc32c_test.cc)
target_link_libraries(crc32c_test base)

add_executable(Timestamp_test Timestamp_test.cc)
target_link_libraries(Timestamp_test base)

add_executable(TimeZone_test TimeZone_test.cc)
target_link_libraries(TimeZone_test base)

add_executable(StringUtil_test StringUtil_test.cc)
target_link_libraries(StringUtil_test base)

add_executable(murmur3_hash_test murmur3_hash_test.cc)
target_link_libraries(murmur3_hash_test base)

add_executable(FileUtil_test FileUtil_test.cc)
target_link_libraries(FileUtil_test base)

add_executable(daemon_test daemon_test.cc)
target_link_libraries(daemon_test base)

add_executable(AsyncLogging_test AsyncLogging_test.cc)
target_link_libraries(AsyncLogging_test base)

add_executable(base_Buffer_test Buffer_test.cc)
target_link_libraries(base_Buffer_test base)

add_executable(Regex_test Regex_test.cc)
target_link_libraries(Regex_test base)