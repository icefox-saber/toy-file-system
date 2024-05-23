# 设置包含头文件的路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 设置可执行文件的输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/step2)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR})

# 设置源文件目录和头文件目录
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib)
set(FS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/step2/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(FS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/step2/include)

file(GLOB SOURCES ${SOURCE_DIR}/*.c)
file(GLOB FS_SOURCES ${FS_SOURCE_DIR}/*.c)

# 列出所有的头文件

# 添加可执行文件 FC
add_executable(FC ${CMAKE_SOURCE_DIR}/step2/FC.c ${SOURCES})

# 添加头文件的路径到 FC 可执行文件的属性中
target_include_directories(FC PRIVATE ${INCLUDE_DIR})

# 添加可执行文件 FS
add_executable(FS ${CMAKE_SOURCE_DIR}/step2/FS.c ${SOURCES} ${FS_SOURCES})

# 添加头文件的路径到 FS 可执行文件的属性中
target_include_directories(FS PRIVATE ${INCLUDE_DIR} ${FS_INCLUDE_DIR})

# 添加可执行文件 FS
add_executable(test ${CMAKE_SOURCE_DIR}/step2/test.c ${SOURCES})

# 添加头文件的路径到 FS 可执行文件的属性中
target_include_directories(test  PRIVATE ${INCLUDE_DIR})
# 添加可执行文件 BDS_step2
add_executable(BDS_step2 ${CMAKE_SOURCE_DIR}/step2/BDS.c ${SOURCES})

# 添加头文件的路径到 BDS_step2 可执行文件的属性中
target_include_directories(BDS_step2 PRIVATE ${INCLUDE_DIR})
