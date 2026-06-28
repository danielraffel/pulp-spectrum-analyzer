# Spectrum analysis engine (layer 1) — pure, headless-testable.

if(PULP_BUILD_TESTS)
    add_executable(spectrum-engine-test test_spectrum_engine.cpp)
    # pulp::signal provides the FFT (and links the platform math backend).
    target_link_libraries(spectrum-engine-test PRIVATE pulp::signal Catch2::Catch2WithMain)
    target_include_directories(spectrum-engine-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(spectrum-engine-test)
endif()
