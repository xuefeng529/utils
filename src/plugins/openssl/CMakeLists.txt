aux_source_directory(. SRCS)
add_library(openssl STATIC ${SRCS})
target_link_libraries(openssl base crypto)

set(HEADERS
Md5.h
Crypto.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/openssl)
install(TARGETS openssl DESTINATION lib)

add_executable(Md5_test tests/Md5_test.cc)
target_link_libraries(Md5_test openssl)

add_executable(Crypto_test tests/Crypto_test.cc)
target_link_libraries(Crypto_test openssl)
