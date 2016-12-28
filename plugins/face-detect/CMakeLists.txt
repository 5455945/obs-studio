project(face-detect)
	
set(face-detect_inc_HEADERS
	inc/face-detect.h
	CvTextFreeType.h)
	
set(face-detect_SOURCES
    face-detect.def
	face-detect-plugin-main.c
	face-detect.cpp
	CvTextFreeType.cpp)

set(face-detect_HEADERS
	${face-detect_inc_HEADERS}
	)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(mylibopencv "${PROJECT_SOURCE_DIR}/lib/libx64/opencv/")
else()
    set(mylibopencv "${PROJECT_SOURCE_DIR}/lib/libx86/opencv/")
endif()
#message("${mylibopencv}")
link_directories("${mylibopencv}")

add_library(face-detect MODULE
	${face-detect_SOURCES}
	${face-detect_HEADERS})
	
include_directories(inc;inc/include)

find_package(Libcurl REQUIRED)
include_directories(${LIBCURL_INCLUDE_DIRS})
add_definitions(${LIBCURL_DEFINITIONS})

target_link_libraries(face-detect libobs 
freetype.lib
IlmImf.lib
ippicvmt.lib
libjasper.lib
libjpeg.lib
libpng.lib
libprotobuf.lib
libtiff.lib
libwebp.lib
opencv_aruco310.lib
opencv_bgsegm310.lib
opencv_bioinspired310.lib
opencv_calib3d310.lib
opencv_ccalib310.lib
opencv_core310.lib
opencv_datasets310.lib
opencv_dnn310.lib
opencv_dpm310.lib
opencv_face310.lib
opencv_features2d310.lib
opencv_flann310.lib
opencv_fuzzy310.lib
opencv_highgui310.lib
opencv_imgcodecs310.lib
opencv_imgproc310.lib
opencv_line_descriptor310.lib
opencv_ml310.lib
opencv_objdetect310.lib
opencv_optflow310.lib
opencv_photo310.lib
opencv_plot310.lib
opencv_reg310.lib
opencv_rgbd310.lib
opencv_saliency310.lib
opencv_shape310.lib
opencv_stereo310.lib
opencv_stitching310.lib
opencv_structured_light310.lib
opencv_superres310.lib
opencv_surface_matching310.lib
opencv_text310.lib
opencv_tracking310.lib
opencv_ts310.lib
opencv_video310.lib
opencv_videoio310.lib
opencv_videostab310.lib
opencv_xfeatures2d310.lib
opencv_ximgproc310.lib
opencv_xobjdetect310.lib
opencv_xphoto310.lib
zlib.lib)


install_obs_plugin_with_data(face-detect data)
