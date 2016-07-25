aux_source_directory(. SRCS)
add_library(json STATIC ${SRCS})
target_link_libraries(json base)

set(HEADERS
assertions.h
autolink.h
config.h
features.h
forwards.h
json.h
reader.h
value.h
writer.h
)
install(FILES ${HEADERS} DESTINATION include/plugins/json)
install(TARGETS json DESTINATION lib)
