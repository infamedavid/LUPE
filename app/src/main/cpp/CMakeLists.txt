cmake_minimum_required(VERSION 3.22.1)

project(lupe_audio LANGUAGES CXX)

add_library(
        lupe_audio
        SHARED
        AudioEngine.cpp
        NativeBridge.cpp
)

find_package(oboe REQUIRED CONFIG)

target_compile_features(lupe_audio PRIVATE cxx_std_17)

target_link_libraries(
        lupe_audio
        PRIVATE
        oboe::oboe
        log
        android
)
