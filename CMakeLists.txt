cmake_minimum_required(VERSION 3.22)
project(SnobbyDelay VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(SnobbyDelay
    COMPANY_NAME "BoomHype Tech"
    PLUGIN_MANUFACTURER_CODE Bhyp
    PLUGIN_CODE Sndl
    FORMATS VST3
    PRODUCT_NAME "Snobby Delay"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Delay)

target_sources(SnobbyDelay PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(SnobbyDelay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SnobbyDelay PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)