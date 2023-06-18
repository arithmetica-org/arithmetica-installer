cmake_minimum_required(VERSION 3.0.0)

project(arithmetica_installer)

add_executable(arithmetica_installer main.cpp)

target_link_directories(arithmetica_installer PRIVATE "lib/curl/x64 Release/lib")

target_link_libraries(arithmetica_installer PRIVATE libcurl_a)
target_link_libraries(arithmetica_installer PRIVATE Ws2_32)
target_link_libraries(arithmetica_installer PRIVATE Normaliz)
target_link_libraries(arithmetica_installer PRIVATE Wldap32)
target_link_libraries(arithmetica_installer PRIVATE Crypt32)
target_link_libraries(arithmetica_installer PRIVATE advapi32)

target_include_directories(arithmetica_installer PRIVATE "lib/curl/x64 Release/include")