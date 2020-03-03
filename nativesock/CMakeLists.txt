# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


#INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/main/cpp/include)
#INCLUDE_DIRECTORIES(../curl_native/src/main/cpp/include/curl)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -march=armv7-a")


if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_definitions(-DENABLE_LOG)
endif ()

#add_definitions(-DENABLE_LOG)
add_definitions(-DNO_CHECK_FILE_DATA)

add_library( # Sets the name of the library.
        native_sock

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${CMAKE_SOURCE_DIR}/src/main/cpp/jni.cpp ${CMAKE_SOURCE_DIR}/src/main/cpp/transdata/Crc32.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)



target_link_libraries( # Specifies the target library.
        native_sock
        z
        ${log-lib})