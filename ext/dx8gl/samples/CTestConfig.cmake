# CTest configuration for dx8gl samples
set(CTEST_PROJECT_NAME "dx8gl-samples")
set(CTEST_NIGHTLY_START_TIME "00:00:00 EST")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "localhost")
set(CTEST_DROP_LOCATION "/submit.php?project=dx8gl-samples")
set(CTEST_DROP_SITE_CDASH TRUE)

# Set timeout for graphics tests (30 seconds per test)
set(CTEST_TEST_TIMEOUT 30)

# Enable verbose output for debugging
set(CTEST_OUTPUT_ON_FAILURE ON)