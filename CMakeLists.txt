cmake_minimum_required(VERSION 3.3)
project(HttpWebChat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        main.cpp
        ChatServer/chat_server.cpp
        ChatServer/chat_server.h
        ChatServer/json.cpp
        ChatServer/json.h
        HTTP/http_server.cpp
        HTTP/http_server.h
        HTTP/route_matcher.cpp
        HTTP/route_matcher.h
        HTTP/http_request.cpp
        HTTP/http_request.h
        HTTP/http_response.cpp
        HTTP/http_response.h
        HTTP/http_message.cpp
        HTTP/http_message.h
        HTTP/http_common.cpp
        HTTP/http_common.h
        TCPSocket/tcp_accept_socket.cpp
        TCPSocket/tcp_accept_socket.h
        TCPSocket/tcp_server_socket.cpp
        TCPSocket/tcp_server_socket.h
        TCPSocket/tcp_socket.cpp
        TCPSocket/tcp_socket.h
        poller.cpp
        poller.h
        resource.cpp
        resource.h
        common.cpp
        common.h)

set(RESOURCE_FILES
        Resources/index.html
        Resources/chat.css
        Resources/favicon.ico
        Resources/green_light.png
        Resources/red_light.png
        Resources/chat.js
        Resources/jquery.js
)

set(BINARY_RESOURCES "")
foreach(RESOURCE_FILE IN LISTS RESOURCE_FILES)
    string(REPLACE "/" ";" SEGMENTS ${RESOURCE_FILE})
    list(GET SEGMENTS 1 FILENAME)
    string(REPLACE "." "_" NAME ${FILENAME})
    string(CONCAT OUTPUT_FILENAME ${NAME} ".o")
    add_custom_command(OUTPUT ${OUTPUT_FILENAME}
            COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/Resources
                && ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILENAME} ${FILENAME}
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/${FILENAME})
    list(APPEND BINARY_RESOURCES ${OUTPUT_FILENAME})
endforeach()

add_executable(HttpWebChat ${SOURCE_FILES} ${BINARY_RESOURCES})

target_link_libraries(HttpWebChat)