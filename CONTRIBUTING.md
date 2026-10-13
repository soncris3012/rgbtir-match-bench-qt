# Contributing

1. Build with C++17, Qt 6 and OpenCV; do not introduce Python runtime dependencies.
2. Keep algorithms outside the UI in `src/core/`.
3. A matcher implements `IFeatureMatcher` and returns paired `cv::Point2f` coordinates.
4. Add unit tests to `tests/TestCore.cpp` for pairing or metric changes.
5. Run `cmake --build build` and `ctest --test-dir build --output-on-failure` before opening a pull request.
6. Do not commit LLVIP/RoadScene images, downloaded weights or generated reports.

