cmake_minimum_required(VERSION 2.8)

#SET(OPENCV_CPP_SAMPLES_REQUIRED_DEPS opencv_core opencv_flann opencv_imgproc
#    opencv_highgui opencv_ml opencv_video opencv_objdetect opencv_photo opencv_nonfree
#    opencv_features2d opencv_calib3d opencv_legacy opencv_contrib opencv_stitching opencv_videostab)

PROJECT( findContour C CXX)

#static linking, we have to set the package fining path to my own local opencv build.
# --------------------------------------------------
  # Widgets finds its own dependencies (QtGui and QtCore).
  find_package(Qt5Widgets)
#  find_package(Qt5Declarative)

  # The Qt5Widgets_INCLUDES also includes the include directories for
  # dependencies QtCore and QtGui
  include_directories(${Qt5Widgets_INCLUDES})

  # We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
  add_definitions(${Qt5Widgets_DEFINITIONS})
# --------------------------------------------------
#opencv should be built first!
set(OpenCV_DIR $ENV{OPENCV_OUT})
#otherwise it will use shared lib in /usr/local/share/openCV
#set(OpenCV_DIR "/home/thomas/build/misc/opencv/build")
set(OPENCV_SRC	$ENV{OPENCV_SRC})
set(OPENCV_OUT	$ENV{OPENCV_OUT})
message("++++OpenCV_DIR:${OpenCV_DIR}")
FIND_PACKAGE( OpenCV REQUIRED)
if(OpenCV_FOUND)
	message("found")
	message( "OpenCV_INCLUDE_DIRS:${OpenCV_INCLUDE_DIRS}" )
	message( "OpenCV_INCLUDES:${OpenCV_INCLUDES}")
	message( "OpenCV_LIBRARIES:${OpenCV_LIBRARIES}")
	message( "OpenCV_LIBS:${OpenCV_LIBS}")
	message( "OpenCV_DEFINITIONS:${OpenCV_DEFINITIONS}")
else()
	message(FATAL_ERROR "openCV not foud")
endif()

message("*** OpenCV_LIBS:${OpenCV_LIBS}")
message("*** OpenCV_LIBRARIES:${OpenCV_LIBRARIES}")


#openMP to avoid link error : omp_xxx error!
# undefined reference to `GOMP_loop_dynamic_start
find_package(OpenMP)
if(OPENMP_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

ADD_EXECUTABLE( findContour 
findContour.c )

qt5_use_modules(findContour Widgets  OpenGL Test Concurrent)

#dynamic or static link
TARGET_LINK_LIBRARIES( findContour ${OpenCV_LIBS} )
