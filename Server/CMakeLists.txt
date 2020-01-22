cmake_minimum_required(VERSION 3.10)

#nom du projet
project(rpi_camera_ip_server)

#set(CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_SYSTEM_PROCESSOR arm)
#set(CMAKE_VERBOSE_MAKEFILE ON)

#set(CMAKE_FIND_ROOT_PATH /root/buildroot-precompiled-2017.08/output/target)
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#include_directories(/root/buildroot-precompiled-2017.08/output/staging/usr/include)
#link_directories(/root/buildroot-precompiled-2017.08/output/target/usr/lib /root/buildroot-precompiled-2017.08/output/target/lib)
#set(CMAKE_SYSROOT /root/buildroot-precompiled-2017.08/output/target) #A modifier pour pointer vers rfs cible
#set(CMAKE_STAGING_PREFIX /root/buildroot-precompiled-2017.08/output/staging)

#set(CMAKE_C_COMPILER /root/buildroot-precompiled-2017.08/output/host/bin/arm-linux-gcc) #A modifier pour pointer vers le compilateur pour la raspberry pi

#ajout executable server
add_executable(main_server ../Server/Main.c ../Server/libCamera.c)
target_link_libraries(main_server pthread v4l2 jpeg) #permet d'inclure des -l__