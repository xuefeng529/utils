aux_source_directory(. SRCS)
add_library(kafkapapi STATIC ${SRCS})
target_link_libraries(kafkapapi base rdkafka)

set(HEADERS
kafkaPApi.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/kafka)
install(TARGETS kafkapapi DESTINATION lib)
