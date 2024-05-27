# 将当前目录下所有源文件名称放入SRC_LIST
aux_source_directory(. SRC_LIST)

# 生成一个共享的动态库
# add_library(mprpc SHARED ${SRC_LIST})

# muduo 库是静态库 基于此将该库生成为静态库 
add_library(mprpc ${SRC_LIST})

# 设置依赖库
# muduo_net是核心库 其他库（base）都依赖于该库 链接时要注意顺序
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)
