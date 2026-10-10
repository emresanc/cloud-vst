cmake_minimum_required(VERSION 3.22)

project(Basement VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

option(BASEMENT_BUILD_PLUGIN "Build the JUCE plugin (downloads JUCE)" ON)
option(BASEMENT_BUILD_TESTS "Build engine tests and CLI" ON)

# --- Melody engine: plain C++, no JUCE ---------------------------------------
add_library(basement_engine STATIC
    engine/Theory.cpp
    engine/Styles.cpp
    engine/Generator.cpp
    engine/MidiWriter.cpp)
target_include_directories(basement_engine PUBLIC engine)
set_target_properties(basement_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(BASEMENT_BUILD_TESTS)
    add_executable(basement-cli tools/cli.cpp)
    target_link_libraries(basement-cli PRIVATE basement_engine)

    enable_testing()
    add_executable(engine_tests tests/engine_tests.cpp)
    target_link_libraries(engine_tests PRIVATE basement_engine)
    add_test(NAME engine_tests COMMAND engine_tests)
endif()

# --- Plugin -------------------------------------------------------------------
if(BASEMENT_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(Basement
        COMPANY_NAME "Basement Audio"
        BUNDLE_ID "com.basementaudio.basement"
        PLUGIN_MANUFACTURER_CODE Bsmt
        PLUGIN_CODE Bmlg
        PRODUCT_NAME "Basement"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Generator
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        FORMATS VST3 AU Standalone)

    target_sources(Basement PRIVATE
        plugin/PluginProcessor.cpp
        plugin/PluginEditor.cpp
        plugin/PianoRoll.cpp
        plugin/PreviewSynth.cpp)

    target_compile_definitions(Basement PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(Basement
        PRIVATE
            basement_engine
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

if(BASEMENT_BUILD_PLUGIN AND BASEMENT_BUILD_TESTS)
    add_executable(processor_tests tests/processor_tests.cpp)
    target_link_libraries(processor_tests PRIVATE Basement basement_engine juce::juce_audio_utils)
    target_compile_definitions(processor_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    add_test(NAME processor_tests COMMAND processor_tests)
endif()

if(BASEMENT_BUILD_PLUGIN AND BASEMENT_BUILD_TESTS)
    add_executable(basement-render tools/render.cpp plugin/PreviewSynth.cpp)
    target_link_libraries(basement-render PRIVATE basement_engine juce::juce_audio_formats juce::juce_audio_basics)
    target_compile_definitions(basement-render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
endif()
