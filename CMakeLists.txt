
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(Borrow)

add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_sources(${PROJECT_NAME}
	PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/borrow_checker.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/borrow_ref_policy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/root_shared_ptr.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)
