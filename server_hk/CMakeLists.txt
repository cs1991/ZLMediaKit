project(HCSDK)
cmake_minimum_required(VERSION 3.1.3)
#使能 c++11
set(CMAKE_CXX_STANDARD 11)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	#添加头文件目录
	INCLUDE_DIRECTORIES(include/linux)
	link_directories(lib/linux/HCNetSDKCom lib/linux)


	file(GLOB HC_LIB_LIST lib/linux/*.so lib/linux/*.so.*)
else ()
	#添加头文件目录
	INCLUDE_DIRECTORIES(include/win64)
	link_directories(lib/win64)  #如果使用	link_directories（指定第三方库所在路径），target_link_libraries只需要给出库名字即可

	file(GLOB HC_LIB_LIST lib/win64/*.lib )
endif ()
file(GLOB HC_SRCS *.cpp  *.h)
message("-----------hk src::${HC_SRCS}")
add_library(HCSDK STATIC ${HC_SRCS})
list(APPEND HC_LIB_LIST ${HC_TOOL_LIB})
target_link_libraries(HCSDK ${HC_LIB_LIST})
message(${HC_LIB_LIST})
