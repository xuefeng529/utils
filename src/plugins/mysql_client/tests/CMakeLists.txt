AUX_SOURCE_DIRECTORY(. SRCS)
ADD_EXECUTABLE(mysql_client_test ${SRCS})
TARGET_LINK_LIBRARIES(mysql_client_test mysql_client)