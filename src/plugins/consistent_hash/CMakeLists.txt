add_subdirectory(tests)

aux_source_directory(. SRCS)
add_library(consistent_hash STATIC ${SRCS})
target_link_libraries(consistent_hash base)

set(HEADERS
ConsistentHash.h
conhash.h
configure.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/consistent_hash)
install(TARGETS consistent_hash DESTINATION lib)
