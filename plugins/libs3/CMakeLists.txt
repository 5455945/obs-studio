project(libs3)

set(libs3_src_SOURCES
	src/acl.c
	src/bucket.c
	src/error_parser.c
	src/general.c
	src/libs3.def
	src/mingw_functions.c
	src/mingw_s3_functions.c
	src/multipart.c
	src/object.c
	src/request.c
	src/request_context.c
	src/response_headers_handler.c
	src/s3.c
	src/service.c
	src/service_access_logging.c
	src/simplexml.c
	src/testsimplexml.c
	src/util.c)

set(libs3_inc_wingw_sys_HEADERS
	inc/mingw/sys/select.h
	inc/mingw/sys/utsname.h)
	
set(libs3_inc_wingw_HEADERS
	inc/mingw/pthread.h)
	
set(libs3_inc_HEADERS
	inc/error_parser.h
	inc/getopt.h
	inc/iconv.h
	inc/libs3.h
	inc/request.h
	inc/request_context.h
	inc/response_headers_handler.h
	inc/simplexml.h
	inc/string_buffer.h
	inc/util.h)
	
set(libs3_SOURCES
	libs3-plugin-main.c
	send-data.c
	${libs3_src_SOURCES})

set(libs3_HEADERS
    ${libs3_inc_wingw_sys_HEADERS}
	${libs3_inc_wingw_HEADERS}
	${libs3_inc_HEADERS}
	)
	
source_group("Header Files\\inc\\mingw\\sys" FILES ${libs3_inc_wingw_sys_HEADERS})
source_group("Header Files\\inc\\mingw" FILES ${libs3_inc_wingw_HEADERS})
source_group("Header Files\\inc" FILES ${libs3_inc_HEADERS})
source_group("Source Files\\src" FILES ${libs3_src_SOURCES})


add_library(libs3 MODULE
	${libs3_SOURCES}
	${libs3_HEADERS})
	
include_directories(inc;inc/mingw;)

find_package(Libcurl REQUIRED)
include_directories(${LIBCURL_INCLUDE_DIRS})
add_definitions(${LIBCURL_DEFINITIONS})


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(mylibxml2 "${libs3_SOURCE_DIR}/lib/libx64/libxml2/libxml2.lib")
else()
	set(mylibxml2 "${libs3_SOURCE_DIR}/lib/libx86/libxml2/libxml2.lib")
endif()

target_link_libraries(libs3 libobs "${mylibxml2}" ${LIBCURL_LIBRARIES})


install_obs_plugin_with_data(libs3 data)
