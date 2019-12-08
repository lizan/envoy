load("@rules_python//python:defs.bzl", "py_binary")
load("//tools/protosync:protosync.bzl", "protosync")

licenses(["notice"])  # Apache 2

py_binary(
    name = "tool",
    srcs = [
        "protosync.py",
    ],
    data = [
        "@envoy_api//bazel:external_proto_deps.bzl",
    ],
    main = "protosync.py",
    python_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        "//tools/api_proto_plugin:utils",
        "//tools/protoxform",
        "@rules_python//python/runfiles",
    ],
)

protosync(
    name = "protosync",
    targets = ["//tools/api_proto_plugin:default_type_db_target"],
)
