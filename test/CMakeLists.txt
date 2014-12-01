PROJECT(hello)
AUX_SOURCE_DIRECTORY(. SRC_FILES)
add_executable(hello ${SRC_FILES})
