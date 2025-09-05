CPMAddPackage(
    NAME cubeb
    GITHUB_REPOSITORY mozilla/cubeb
    GIT_TAG master
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTS OFF"
        "BUILD_RUST_LIBS OFF"
        "BUILD_TOOLS OFF"
        "BUNDLE_SPEEX OFF"
        "LAZY_LOAD_LIBS ON"
        "USE_SANITIZERS OFF"
        "USE_STATIC_MSVC_RUNTIME OFF"
)
