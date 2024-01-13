set(TEST_TARGET conversion_test)

# Find GTK+ 3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

find_package(JPEG REQUIRED)

# Add your executable
add_executable(${TEST_TARGET} Test.cpp)
target_link_directories(${TEST_TARGET} PRIVATE ${GTK3_LIBRARY_DIRS})
target_include_directories(${TEST_TARGET} PRIVATE ${GTK3_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
target_link_libraries(${TEST_TARGET} PRIVATE ${GTK3_LIBRARIES} ${JPEG_LIBRARIES} ${LIB_TARGET})
target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
