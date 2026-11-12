cmake_minimum_required(VERSION 3.22)

project(HA81A_Coulibaly_Edition VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HA81A_Coulibaly_Edition
    COMPANY_NAME "Coulibaly Audio"
    BUNDLE_ID com.coulibalyaudio.ha81aeq
    PLUGIN_MANUFACTURER_CODE Cbly
    PLUGIN_CODE H81A
    PRODUCT_NAME "HA-81A EQ - Coulibaly Edition"
    DESCRIPTION "Four-band analogue-inspired equalizer"
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(HA81A_Coulibaly_Edition)

target_sources(HA81A_Coulibaly_Edition PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_features(HA81A_Coulibaly_Edition PRIVATE cxx_std_17)
target_compile_definitions(HA81A_Coulibaly_Edition PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(HA81A_Coulibaly_Edition PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
