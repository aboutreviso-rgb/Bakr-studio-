cmake_minimum_required(VERSION 3.22)
project(BAKR_ANTHOLOGY VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE : dossier local (-DJUCE_LOCAL_PATH=...) sinon téléchargé automatiquement
if(DEFINED JUCE_LOCAL_PATH)
    add_subdirectory(${JUCE_LOCAL_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/BakrBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/BakrUI.cpp)

function(bakr_plugin TARGET PRODUCT CODE SOURCE)
    juce_add_plugin(${TARGET}
        COMPANY_NAME "BAKR"
        PLUGIN_MANUFACTURER_CODE Bakr
        PLUGIN_CODE ${CODE}
        FORMATS VST3
        PRODUCT_NAME "${PRODUCT}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Dynamics)
    juce_generate_juce_header(${TARGET})
    target_sources(${TARGET} PRIVATE ${SOURCE} ${COMMON_SOURCES})
    target_compile_definitions(${TARGET} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(${TARGET}
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endfunction()

bakr_plugin(BAKR_AMBER_OPTO  "BAKR AMBER OPTO"  Bamo Plugins/AmberOpto.cpp)
bakr_plugin(BAKR_KARAT_STRIP "BAKR KARAT STRIP" Bkst Plugins/KaratStrip.cpp)
bakr_plugin(BAKR_VARIMU_ST   "BAKR VARIMU ST"   Bvms Plugins/VariMuSt.cpp)
bakr_plugin(BAKR_BUS4        "BAKR BUS4"        Bbu4 Plugins/Bus4.cpp)
bakr_plugin(BAKR_LINE_PRE    "BAKR LINE PRE"    Blpr Plugins/LinePre.cpp)
bakr_plugin(BAKR_BLUE_COMP   "BAKR BLUE COMP"   Bblc Plugins/BlueComp.cpp)

# BAKR Suite (nouvelle generation, look UAD)
bakr_plugin(BAKR_LEVELER     "BAKR LEVELER"     Blv2 Plugins/Leveler.cpp)
bakr_plugin(BAKR_FET         "BAKR FET"         Bft2 Plugins/Fet.cpp)
bakr_plugin(BAKR_CHANNEL     "BAKR CHANNEL"     Bch2 Plugins/Channel.cpp)
bakr_plugin(BAKR_TUBE        "BAKR TUBE"        Btb2 Plugins/Tube.cpp)
bakr_plugin(BAKR_MASTER      "BAKR MASTER"      Bms2 Plugins/Master.cpp)
bakr_plugin(BAKR_FATBOX      "BAKR FATBOX"      Bfbx Plugins/FatBox.cpp)
bakr_plugin(BAKR_TAK35_PRO   "BAKR TAK35 PRO"   Btk3 Plugins/TakPro.cpp)

# BAKR CALIBRE : calibration automatique des enceintes (HS8) — mesure + correction
juce_add_plugin(BAKR_CALIBRE
    COMPANY_NAME "BAKR"
    PLUGIN_MANUFACTURER_CODE Bakr
    PLUGIN_CODE Bcal
    FORMATS VST3
    PRODUCT_NAME "BAKR CALIBRE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer EQ)
juce_generate_juce_header(BAKR_CALIBRE)
target_sources(BAKR_CALIBRE PRIVATE Plugins/Calibre/Calibre.cpp ${COMMON_SOURCES})
target_compile_definitions(BAKR_CALIBRE PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(BAKR_CALIBRE
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# BAKR INSTINCT : assistant vocal qui écoute la voix et règle la chaîne
juce_add_plugin(BAKR_INSTINCT
    COMPANY_NAME "BAKR"
    PLUGIN_MANUFACTURER_CODE Bakr
    PLUGIN_CODE Bins
    FORMATS VST3
    PRODUCT_NAME "BAKR INSTINCT"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics EQ)
juce_generate_juce_header(BAKR_INSTINCT)
target_sources(BAKR_INSTINCT PRIVATE Plugins/Instinct/Instinct.cpp ${COMMON_SOURCES})
target_compile_options(BAKR_INSTINCT PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(BAKR_INSTINCT PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(BAKR_INSTINCT
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# BAKR CASQUE : calibration de casque + émulation d'enceintes
juce_add_plugin(BAKR_CASQUE
    COMPANY_NAME "BAKR"
    PLUGIN_MANUFACTURER_CODE Bakr
    PLUGIN_CODE Bcsq
    FORMATS VST3
    PRODUCT_NAME "BAKR CASQUE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Spatial EQ)
juce_generate_juce_header(BAKR_CASQUE)
target_sources(BAKR_CASQUE PRIVATE Plugins/Casque/Casque.cpp ${COMMON_SOURCES})
target_compile_options(BAKR_CASQUE PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(BAKR_CASQUE PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(BAKR_CASQUE
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Outil de test (captures d'écran + test audio), pas compilé pour Windows
option(BAKR_SNAP "Construire l'outil de capture" OFF)
if(BAKR_SNAP)
    foreach(P AmberOpto KaratStrip VariMuSt Bus4 LinePre BlueComp Leveler Fet Channel Tube Master FatBox TakPro)
        juce_add_console_app(Snap_${P} PRODUCT_NAME "Snap_${P}")
        juce_generate_juce_header(Snap_${P})
        target_sources(Snap_${P} PRIVATE Tools/Snap.cpp Plugins/${P}.cpp ${COMMON_SOURCES})
        target_compile_definitions(Snap_${P} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 SNAP_NAME="${P}" JucePlugin_Name="${P}")
        target_link_libraries(Snap_${P} PRIVATE juce::juce_audio_utils juce::juce_dsp)
    endforeach()
    juce_add_console_app(Test_Calibre PRODUCT_NAME "Test_Calibre")
    juce_generate_juce_header(Test_Calibre)
    target_sources(Test_Calibre PRIVATE Tools/TestCalibre.cpp Plugins/Calibre/Calibre.cpp ${COMMON_SOURCES})
    target_compile_definitions(Test_Calibre PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 CALIBRE_TEST=1 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="BAKR CALIBRE")
    target_link_libraries(Test_Calibre PRIVATE juce::juce_audio_utils juce::juce_dsp)
    juce_add_console_app(Test_Instinct PRODUCT_NAME "Test_Instinct")
    juce_generate_juce_header(Test_Instinct)
    target_sources(Test_Instinct PRIVATE Tools/TestInstinct.cpp Plugins/Instinct/Instinct.cpp ${COMMON_SOURCES})
    target_compile_definitions(Test_Instinct PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 INSTINCT_TEST=1 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="BAKR INSTINCT")
    target_link_libraries(Test_Instinct PRIVATE juce::juce_audio_utils juce::juce_dsp)
    juce_add_console_app(Test_Casque PRODUCT_NAME "Test_Casque")
    juce_generate_juce_header(Test_Casque)
    target_sources(Test_Casque PRIVATE Tools/TestCasque.cpp Plugins/Casque/Casque.cpp ${COMMON_SOURCES})
    target_compile_definitions(Test_Casque PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="BAKR CASQUE")
    target_link_libraries(Test_Casque PRIVATE juce::juce_audio_utils juce::juce_dsp)
endif()
