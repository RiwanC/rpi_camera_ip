cmake_minimum_required(VERSION 3.10)

#nom du projet
project(rpi_camera_ip_server)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER /bin/gcc) #A modifier pour pointer vers le compilateur pour la raspberry pi

#ajout executable server
add_executable(main_server ../Server/Main.c)