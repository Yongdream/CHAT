aux_source_directory(. SRC_LIST)

# 可执行文件
add_executable(ChatClient ${SRC_LIST})

# 依赖库文件
target_link_libraries(ChatClient pthread)