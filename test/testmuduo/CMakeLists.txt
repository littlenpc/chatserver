# cmake_minimum_required(VERSION 3.0) # 最小版本
# project(main) # 工程名

# 通过g++编译可执行文件：
# g++ -o
# 1. 可执行文件名(也可以是静态库.a或动态库.so) 
# 2. [-g(加这个的话表示生成的文件可以调试)，还有一些其他命令] 
# 3. .cpp文件 
# 4. -I/usr/include(头文件搜索路径) 
# 5. -L/usr/lib(库搜索路径) 
# 6. -i(静态库或动态库名，表示要链接的库)

# 配置编译选项 即2.位置
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) # 在变量CMAKE_CXX_FLAGS值的基础上加上-g选项，就可以调试了(或其他选项)

# 设置可执行文件最终的存储路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) # PROJECT_SOURCE_DIR --- 工程根目录


# 配置头文件搜索路径 即4.位置
# include_directories()
# 配置库文件搜索路径 即5.位置
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LIST muduo_server.cpp)

# 定义变量SRC_LIST，存储当前目录下的所有源文件。也可以说把.指定路径下的所有源文件名字放入变量SRC_LIST里面
# aux_source_directory(. SRC_LIST) 

# 表示生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST})
# add_library() # 生成库文件
# 表示server这个目标程序，需要连接muduo_net muduo_base pthread这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)