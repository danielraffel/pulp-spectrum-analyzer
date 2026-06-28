# Spectrum analysis engine (layer 1) — pure, headless-testable.

if(PULP_BUILD_TESTS)
    # Layer 1 — analysis engine (needs pulp::signal for the FFT).
    add_executable(spectrum-engine-test test_spectrum_engine.cpp)
    target_link_libraries(spectrum-engine-test PRIVATE pulp::signal Catch2::Catch2WithMain)
    target_include_directories(spectrum-engine-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(spectrum-engine-test)

    # Layer 2 — diff model (pure; no Pulp libraries required).
    add_executable(spectrum-diff-test test_spectrum_diff.cpp)
    target_link_libraries(spectrum-diff-test PRIVATE Catch2::Catch2WithMain)
    target_include_directories(spectrum-diff-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(spectrum-diff-test)

    # Persistent UI state (pure; versioned (de)serializer with safe defaults).
    add_executable(analyzer-state-test test_analyzer_state.cpp)
    target_link_libraries(analyzer-state-test PRIVATE Catch2::Catch2WithMain)
    target_include_directories(analyzer-state-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(analyzer-state-test)

    # Layer 3 — lock-free RT publication (audio→worker→UI; FFT off the callback).
    add_executable(rt-publisher-test test_rt_publisher.cpp)
    target_link_libraries(rt-publisher-test PRIVATE pulp::signal pulp::runtime Catch2::Catch2WithMain)
    target_include_directories(rt-publisher-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(rt-publisher-test)
endif()
