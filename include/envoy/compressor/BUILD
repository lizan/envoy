load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "compressor_interface",
    hdrs = ["compressor.h"],
    deps = [
        "//include/envoy/buffer:buffer_interface",
    ],
)
