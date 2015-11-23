cmake_minimum_required(VERSION 2.8.4)

project(group-man)

#include(../cmake/mingw.cmake)
include(../cmake/darwin.cmake)
include(../cmake/utils.cmake)

set(COORD coordinator)
set(NTEST node-test)
set(COORD_SRCS 
    src/coordinator.c)
set(NTEST_SRCS
    test/node-test.c)
set(SRCS 
    src/gb-sender.c 
    src/gb-receiver.c
    src/node.c
    ../common/src/ansi-utils.c
    ../common/src/my-pjlib-utils.c
    ../object-pool/src/object-pool.c
    ../media-endpoint/src/endpoint.c)

set(GM_SRCS
    ${PROJECT_BINARY_DIR}/gen/gm-server.c 
    ${PROJECT_BINARY_DIR}/gen/gm-client.c)
set(GMC_SRCS
    ${PROJECT_BINARY_DIR}/gen/gmc-server.c 
    ${PROJECT_BINARY_DIR}/gen/gmc-client.c)
set(ADV_SRCS
    ${PROJECT_BINARY_DIR}/gen/adv-server.c 
    ${PROJECT_BINARY_DIR}/gen/adv-client.c)
set(GB_SRCS
    ${PROJECT_BINARY_DIR}/gen/gb-server.c
    ${PROJECT_BINARY_DIR}/gen/gb-client.c)

set(USERVER_DIR ../../userver)
set(UGENTOOL ${USERVER_DIR}/gen-tools/gen.awk)

set(GM_U    protocols/gm-proto.u)
set(GMC_U   protocols/gmc-proto.u)
set(ADV_U   protocols/adv-proto.u)
set(GB_U    protocols/gb-proto.u)

add_custom_command(
    OUTPUT ${GM_SRCS}
    COMMAND mkdir -p gen
    COMMAND awk -f ${UGENTOOL} $< ${USERVER_DIR}
    DEPENDS ${GM_U}
)
add_custom_command(
    OUTPUT ${GMC_SRCS}
    COMMAND mkdir -p gen
    COMMAND awk -f ${UGENTOOL} $< ${USERVER_DIR}
    DEPENDS ${GMC_U}
)
add_custom_command(
    OUTPUT ${ADV_SRCS}
    COMMAND mkdir -p gen
    COMMAND awk -f ${UGENTOOL} $< ${USERVER_DIR}
    DEPENDS ${ADV_U}
)
add_custom_command(
    OUTPUT ${GB_SRCS}
    COMMAND mkdir -p gen
    COMMAND awk -f ${UGENTOOL} $< ${USERVER_DIR}
    DEPENDS ${GB_U}
)

my_find_pjproject()
my_find_json_c()

add_definitions(${PJPROJECT_CFLAGS} ${JSON_C_CFLAGS})
include_directories(
    ${PROJECT_BINARY_DIR}/gen
    include
    ../common/include
    ../object-pool/include
    ../media-endpoint/include
    ../libut/include
    ${PJPROJECT_INCLUDE_DIRS}
    ${JSON_C_INCLUDE_DIRS}
)

add_executable(${NTEST} ${NTEST_SRCS} ${SRCS} ${GM_SRCS} ${GMC_SRCS} ${ADV_SRCS} ${GB_SRCS})
add_executable(${COORD} ${COORD_SRCS} ${SRCS} ${GM_SRCS} ${GMC_SRCS} ${ADV_SRCS} ${GB_SRCS})

my_target_link_pjproject(${NTEST})
my_target_link_pjproject(${COORD})

target_link_libraries(${NTEST} ${JSON_C_LDFLAGS})
target_link_libraries(${COORD} ${JSON_C_LDFLAGS})

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

install(TARGETS ${NTEST} ${COORD} RUNTIME DESTINATION .)

set(CPACK_GENERATOR "ZIP")

include(CPack)
