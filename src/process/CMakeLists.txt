add_project(process STATIC
    image.c image.h
    scanner.c scanner.h
    FOLDER libs
)

target_include_directories(process PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(process PRIVATE klib)
