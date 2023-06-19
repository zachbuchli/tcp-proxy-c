cmake_minimum_required(VERSION 3.25)
project(server_client C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g3 -Og -Wall -Wextra -pedantic -DHAVE_INLINE -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE")

add_executable(server server.c tcp_proxy.c)
add_executable(client client.c)
add_executable(showip showip.c)
add_executable(echo_server echo_server.c tcp_proxy.c)
