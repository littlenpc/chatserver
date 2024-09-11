# 定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)

# 指定生成可执行文件
add_executable(ChatClient ${SRC_LIST})
# 指定可执行文件链接时需要依赖的库文件
# 在客户端有读写两个线程
# 一是读线程：专门接收chat server服务器发送过来的消息响应的
# 二是写线程：接收用户的输入，发送相应的业务数据
target_link_libraries(ChatClient pthread)