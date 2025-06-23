cmake_minimum_required(VERSION 3.10)
project(netconf_tracing)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LibSSH REQUIRED)

# Find libnetconf2 and libyang
pkg_check_modules(LIBNETCONF2 REQUIRED libnetconf2)
pkg_check_modules(LIBYANG REQUIRED libyang)

# Include directories
include_directories(${LIBNETCONF2_INCLUDE_DIRS})
include_directories(${LIBYANG_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# Add compiler flags
add_compile_options(${LIBNETCONF2_CFLAGS_OTHER})
add_compile_options(${LIBYANG_CFLAGS_OTHER})

# Server executable
add_executable(netconf_server
    src/server.c
    src/common.c
)

# Client executable
add_executable(netconf_client
    src/client.c
    src/common.c
)

# Link libraries for server
target_link_libraries(netconf_server
    ${LIBNETCONF2_LIBRARIES}
    ${LIBYANG_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSSH_LIBRARIES}
    pthread
    xml2
    xslt
)

# Link libraries for client
target_link_libraries(netconf_client
    ${LIBNETCONF2_LIBRARIES}
    ${LIBYANG_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBSSH_LIBRARIES}
    pthread
    xml2
    xslt
)

# Copy configuration files
file(COPY config DESTINATION ${CMAKE_BINARY_DIR}) 