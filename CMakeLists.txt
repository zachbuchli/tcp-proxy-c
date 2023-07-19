cmake_minimum_required(VERSION 3.8)
project(server_client C)

if (MSVC)
	message("Configure MSVC compiler flags!")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g3 -Og -Wall -Wextra -pedantic -DHAVE_INLINE -D_GNU_SOURCE")
endif()

# Create Executables

add_executable(server server.c tcp_proxy.c)
add_executable(client client.c)
add_executable(showip showip.c)
add_executable(echo_server echo_server.c tcp_proxy.c)

# Create libraries

add_library(logger STATIC logger.c)

# link libs

target_link_libraries(echo_server pthread)
target_link_libraries(echo_server logger)
target_link_libraries(server pthread)
