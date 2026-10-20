cmake_minimum_required(VERSION 3.22)

# Windows: statikus futtatókörnyezet, hogy ne kelljen külön VC++ Redistributable a felhasználónál
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# macOS: univerzális (Intel + Apple Silicon) build
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

project(Shapetrator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(JUCE)

option(SHAPETRATOR_COPY_AFTER_BUILD "Build után másolja a VST3-at a rendszer VST3 mappájába" OFF)

juce_add_plugin(Shapetrator
    COMPANY_NAME                "Shapetrator"
    PLUGIN_MANUFACTURER_CODE    Shpt
    PLUGIN_CODE                 Shtr
    FORMATS                     VST3
    PRODUCT_NAME                "Shapetrator"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    VST3_CATEGORIES             Fx
    COPY_PLUGIN_AFTER_BUILD     ${SHAPETRATOR_COPY_AFTER_BUILD}
)

target_sources(Shapetrator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(Shapetrator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Shapetrator
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
