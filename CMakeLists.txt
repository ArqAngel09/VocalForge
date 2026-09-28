cmake_minimum_required(VERSION 3.22)
project(VocalForge VERSION 2.1.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 9.0.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VocalForge
    COMPANY_NAME "Angel Music Records"
    BUNDLE_ID "com.angelmusicrecords.amrvocalmix"
    PLUGIN_MANUFACTURER_CODE VFor
    PLUGIN_CODE Vc01
    FORMATS VST3 Standalone
    PRODUCT_NAME "AMR Vocal Mix"
    PLUGIN_NAME "AMR Vocal Mix"
    DESCRIPTION "Professional adaptive vocal mixing processor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx
)

juce_generate_juce_header(VocalForge)

target_sources(VocalForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/VocalPitchCorrector.cpp
    Source/VocalPitchCorrector.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/AMRLogo.h
)

target_compile_features(VocalForge PRIVATE cxx_std_17)
target_compile_definitions(VocalForge PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VocalForge PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
