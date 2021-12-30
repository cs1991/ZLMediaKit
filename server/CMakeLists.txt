include_directories(../3rdpart)
file(GLOB jsoncpp_src_list ../3rdpart/jsoncpp/*.cpp ../3rdpart/jsoncpp/*.h )
add_library(jsoncpp STATIC ${jsoncpp_src_list})

# file(glob ...)将符合条件的文件组成的列表保存在变量中
file(GLOB MediaServer_src_list ./*.cpp  ./*.h)


#海康服务
if (ENABLE_HCSDK)
	message("----start build hcsdk")
    #ADD_SUBDIRECTORY(server_hk) #添加子目录
	#list(APPEND MediaServer_src_list ${HC_SRCS})
	list(APPEND LINK_LIB_LIST HCSDK)
	message("----end build hcsdk")
endif ()
message("--->LINK_LIB_LIST::${LINK_LIB_LIST}")

# 使用指定的源文件来生成目标可执行文件
add_executable(MediaServer ${MediaServer_src_list})

if(MSVC)
	set_target_properties(MediaServer PROPERTIES COMPILE_FLAGS  ${VS_FALGS} )
else()
# DESTINATION 用于指定安装路径，可以是绝对路径和相对路径，相对路径需要配合cmake_install_prefix变量来指定路径前缀
	install(TARGETS MediaServer DESTINATION ${INSTALL_PATH_EXECUTABLE})
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_link_libraries(MediaServer -Wl,--start-group jsoncpp ${LINK_LIB_LIST} -Wl,--end-group)
else ()
	target_link_libraries(MediaServer jsoncpp ${LINK_LIB_LIST})
endif ()
