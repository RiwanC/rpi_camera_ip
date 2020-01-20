cmake_minimum_required(VERSION 3.10)

#nom du projet
project(rpi_camera_ip)

#ajout executable server
add_executable(main_server Server/Main.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

#ajout executable client
add_executable(main_client Client/ihm.c)
target_link_libraries(main_client ${GTK3_LIBRARIES})