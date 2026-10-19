cmake_minimum_required(VERSION 3.10)
project(unit_tests
    VERSION 0.1
    DESCRIPTION "Unit tests for the Passwords application"
    LANGUAGES C
)

#######################################################################
#                        Setup SDK & LedgerUT                         #
#######################################################################
if(NOT DEFINED ENV{BOLOS_SDK})
    message(FATAL_ERROR "BOLOS_SDK environment variable is not set. Please set it to the path of the Ledger Secure SDK.")
endif()

set(SDK_DIR $ENV{BOLOS_SDK})
include(${SDK_DIR}/cmake/LedgerUT.cmake)

#######################################################################
#             Setup app specific includes & compile defs              #
#######################################################################
set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
set(FAKES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fakes)

set(FAKE_SOURCES
    ${FAKES_DIR}/fake_sdk.c
    ${FAKES_DIR}/fake_vars.c
)

set(UT_INCLUDE_DIRS
    ${FAKES_DIR}
    ${APP_DIR}
    ${APP_DIR}/apdu_handlers
    ${APP_DIR}/ctaes
    ${APP_DIR}/ui
    ${UT_INCLUDE_DIRS_SDK}
)

# CMock checks for NULL the arguments that SDK headers declare nonnull
set(MOCK_COMPILE_OPTIONS -Wno-nonnull-compare)

set(UT_COMPILE_DEFS
    ${UT_COMPILE_DEFS_SDK}
    APPNAME="Passwords"
    APPVERSION="a.b.c"
    USE_CTAES
)

#######################################################################
#                           LedgerUT Setup                            #
#######################################################################
ledger_unit_tests_init()

# The app sources live in ../../src: ledger_unit_tests_find_sources() only looks in ../src
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${APP_DIR}/*.c)
list(FILTER APP_SOURCES EXCLUDE REGEX "/app_main\\.c$")

# Built only so that every app source has a .gcno file for the coverage baseline
ledger_unit_tests_build_app_sources_as_lib(
    SOURCES      ${APP_SOURCES}
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

# generate_coverage filters on ../src: regenerate the report restricted to the app sources
add_custom_command(TARGET generate_coverage POST_BUILD
    COMMAND ${LEDGER_UT_DIR}/gen_coverage.sh ${APP_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    VERBATIM
)

#######################################################################
#                        LedgerUT test suites                         #
#######################################################################
ledger_unit_tests_add_test(
    NAME         test_password_list
    SOURCES      ${APP_DIR}/password_list.c
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_hid_mapping
    SOURCES      ${APP_DIR}/hid_mapping.c
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_password
    SOURCES      ${APP_DIR}/password.c
                 ${APP_DIR}/metadata.c
                 ${FAKE_SOURCES}
    MOCK_HEADERS ${APP_DIR}/options.h
                 ${APP_DIR}/password_typing.h
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_dump_metadatas
    SOURCES      ${APP_DIR}/apdu_handlers/dump_metadatas.c
                 ${APP_DIR}/metadata.c
                 ${FAKE_SOURCES}
    MOCK_HEADERS ${APP_DIR}/ui/ui.h
                 ${SDK_DIR}/lib_standard_app/io.h
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_load_metadatas
    SOURCES      ${APP_DIR}/apdu_handlers/load_metadatas.c
                 ${APP_DIR}/metadata.c
                 ${FAKE_SOURCES}
    MOCK_HEADERS ${APP_DIR}/ui/ui.h
                 ${SDK_DIR}/lib_standard_app/io.h
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_options
    SOURCES      ${APP_DIR}/options.c
                 ${FAKE_SOURCES}
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_password_generation
    SOURCES      ${APP_DIR}/password_generation.c
                 ${APP_DIR}/ctr_drbg.c
                 ${APP_DIR}/ctaes/ctaes.c
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
)

ledger_unit_tests_add_test(
    NAME         test_password_typing
    SOURCES      ${APP_DIR}/password_typing.c
                 ${APP_DIR}/password_generation.c
                 ${APP_DIR}/password_path.c
                 ${APP_DIR}/hid_mapping.c
                 ${APP_DIR}/ctr_drbg.c
                 ${APP_DIR}/ctaes/ctaes.c
                 ${FAKE_SOURCES}
    MOCK_HEADERS ${SDK_DIR}/lib_cxng/include/lcx_sha256.h
                 ${SDK_DIR}/io/include/os_io_seph_cmd.h
                 ${SDK_DIR}/lib_stusb/include/usbd_ledger.h
    INCLUDE_DIRS ${UT_INCLUDE_DIRS}
    COMPILE_DEFS ${UT_COMPILE_DEFS}
    COMPILE_OPTIONS ${MOCK_COMPILE_OPTIONS}
)

# A signed shift overflow aborts the test
ledger_unit_tests_add_test(
    NAME            test_password_path
    SOURCES         ${APP_DIR}/password_path.c
    INCLUDE_DIRS    ${UT_INCLUDE_DIRS}
    COMPILE_DEFS    ${UT_COMPILE_DEFS}
    COMPILE_OPTIONS -fsanitize=undefined -fno-sanitize-recover=undefined
    LINK_OPTIONS    -fsanitize=undefined
)
