cmake_minimum_required(VERSION 2.8.9)
include(externalproject)

project(libfcgi)


##libcppa - actor framework
#externalproject_add(libcppa
#	GIT_REPOSITORY https://github.com/Neverlord/libcppa.git
#	GIT_TAG V0.7.1
#	CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
#	BUILD_COMMAND make -f <SOURCE_DIR>/Makefile -j8
#	INSTALL_COMMAND make -f <SOURCE_DIR>/Makefile install
#)
#externalproject_get_property(libcppa install_dir)
#set(cppa_install_dir ${install_dir})
#include_directories(${cppa_install_dir}/include)
#find_library(cppa_library cppa ${cppa_install_dir})

#boost
find_package(Boost 1.54 COMPONENTS program_options REQUIRED )

include_directories(cxx/include)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-std=c++11 -stdlib=libc++ -L/usr/local/lib -lboost_system -lboost_serialization")

set(libfcgi_srcs ${libfcgi_srcs} cxx/include/komm/fcgi.hpp)
set(libfcgi_srcs ${libfcgi_srcs} cxx/source/fcgi.cpp)

set(libfcgi_libs ${libfcgi_libs} libfcgi)
#set(libfcgi_libs ${libfcgi_libs} ${cppa_library})
set(libfcgi_libs ${libfcgi_libs} ${Boost_LIBRARIES})
message("XXX ${Boost_LIBRARIES}")
add_library(libfcgi ${libfcgi_srcs})
#add_dependencies(libfcgi libcppa)
add_executable(libfcgi_test cxx/source/libfcgi_test.cpp)
target_link_libraries(libfcgi_test ${libfcgi_libs})


