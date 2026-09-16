enable_language(ASM_NASM)

add_executable(test_scanner test_scanner.c)
target_include_directories(test_scanner PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_scanner PRIVATE process)
add_test(NAME scanner COMMAND test_scanner)

add_executable(test_vdf test_vdf.c)
target_include_directories(test_vdf PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_vdf PRIVATE steam)
add_test(NAME vdf COMMAND test_vdf)

add_executable(test_autoloot test_autoloot.c test_autoloot.asm
    ${CMAKE_SOURCE_DIR}/src/mods/autoloot/exec_buttom_action_param_filter.asm)
add_test(NAME autoloot_registers COMMAND test_autoloot)

add_executable(test_steam_api test_steam_api.c)
target_include_directories(test_steam_api PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_steam_api PRIVATE steam)
add_test(NAME steam_api_unavailable COMMAND test_steam_api)
