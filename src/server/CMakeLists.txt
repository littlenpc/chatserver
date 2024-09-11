
# 定义变量SRC_LIST，存储当前目录下的所有源文件。也可以说把.指定路径下的所有源文件名字放入变量SRC_LIST里面
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 指定可生成文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 指定可执行文件连接时需要依赖的文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread hiredis)