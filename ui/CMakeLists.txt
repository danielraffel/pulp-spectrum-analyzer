# Layer 4 (screenshot-testable slice) — spectrum panel rendered headlessly with
# the Skia raster backend and compared against checked-in baselines. Needs
# pulp::view (+ Skia) and pulp::signal (for the demo spectra). SKIPs cleanly
# where the Skia raster backend is unavailable.

if(PULP_BUILD_TESTS)
    add_executable(analyzer-ui-test test_analyzer_ui.cpp)
    target_link_libraries(analyzer-ui-test PRIVATE pulp::view pulp::signal Catch2::Catch2WithMain)
    target_include_directories(analyzer-ui-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(analyzer-ui-test PRIVATE ANALYZER_UI_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    catch_discover_tests(analyzer-ui-test)
endif()
