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

add_executable(Hash_test Hash_test.cc)
target_link_libraries(Hash_test base)

add_executable(FileUtil_test FileUtil_test.cc)
target_link_libraries(FileUtil_test base)

add_executable(daemon_test daemon_test.cc)
target_link_libraries(daemon_test base)