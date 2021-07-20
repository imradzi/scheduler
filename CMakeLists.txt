cmake_minimum_required(VERSION 3.17)
project(scheduler)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
