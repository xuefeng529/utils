ADD_EXECUTABLE(ZkClient_test Client_test.cc)
TARGET_LINK_LIBRARIES(ZkClient_test zk)

ADD_EXECUTABLE(LeaderSelector_test LeaderSelector_test.cc)
TARGET_LINK_LIBRARIES(LeaderSelector_test zk)
