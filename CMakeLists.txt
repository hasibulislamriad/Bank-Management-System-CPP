cmake_minimum_required(VERSION 3.20)
project(BankManagementSystem VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(bank_system
    app/main_refactored.cpp
    src/Database.cpp
    src/Security.cpp
    src/BankService.cpp
)

target_include_directories(bank_system PRIVATE include)

target_compile_options(bank_system PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_libraries(bank_system PRIVATE SQLite::SQLite3 OpenSSL::Crypto)
