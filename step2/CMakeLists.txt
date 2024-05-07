# 设置包含头文件的路径
include_directories(${CMAKE_SOURCE_DIR}/include)
# 设置可执行文件的输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/step2)    
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}) 
# 设置源文件目录和头文件目录
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
file(GLOB SOURCES ${SOURCE_DIR}/*.c)

# 列出所有的头文件
set(HEADERS ${INCLUDE_DIR}/tcp_buffer.h ${INCLUDE_DIR}/tcp_utils.h ${INCLUDE_DIR}/thpool.h)

# 添加可执行文件 BDS
# 添加可执行文件 BDC
add_executable(FC ${CMAKE_SOURCE_DIR}/step2/FC.c ${SOURCES})

# 添加头文件的路径到 BDC 可执行文件的属性中
target_include_directories(FC PRIVATE ${INCLUDE_DIR})

add_executable(FS ${CMAKE_SOURCE_DIR}/step2/FS.c ${SOURCES})

# 添加头文件的路径到 BDC 可执行文件的属性中
target_include_directories(FS PRIVATE ${INCLUDE_DIR})

add_executable(BDS_step2 ${CMAKE_SOURCE_DIR}/step2/BDS.c ${SOURCES})

# 添加头文件的路径到 BDS 可执行文件的属性中
target_include_directories(BDS PRIVATE ${INCLUDE_DIR})
