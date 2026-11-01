cmake_minimum_required(VERSION 3.22)

project(Chain7st VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE 8 se descarga automaticamente (no hace falta instalarlo)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Chain7st
    COMPANY_NAME               "Chain7"
    PLUGIN_MANUFACTURER_CODE   Chn7
    PLUGIN_CODE                C7st
    PRODUCT_NAME               "Chain7st"
    FORMATS                    VST3 Standalone
    VST3_CATEGORIES            Fx
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE)

target_sources(Chain7st PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/LookAndFeel.h
    Source/DSP.h)

target_compile_definitions(Chain7st PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Chain7st
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
