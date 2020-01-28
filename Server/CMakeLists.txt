cmake_minimum_required(VERSION 3.10)

#nom du projet
project(rpi_camera_ip_server)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_C_COMPILER /root/buildroot-precompiled-2017.08/output/host/bin/arm-linux-gcc) #A modifier pour pointer vers le compilateur pour la raspberry pi

set(CMAKE_FIND_ROOT_PATH /root/buildroot-precompiled-2017.08/output/target) #A modifier pour pointer vers le RFS distant
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#ajout executable serveur
add_executable(server src/Main.c src/libCamera.c)
target_link_libraries(server pthread v4l2 jpeg) #permet d'inclure des -l__
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../bin")