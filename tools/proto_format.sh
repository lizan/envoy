#!/bin/bash

# Reformat API protos to canonical proto style using protoxform.

set -e

[[ "$1" == "check" || "$1" == "fix" ]] || (echo "Usage: $0 <check|fix>"; exit 1)

# Clean up any stale files in the API tree output. Bazel remembers valid cached
# files still.
rm -rf bazel-bin/external/envoy_api

# Find all source protos.
declare -r PROTO_TARGETS=$(bazel query "labels(srcs, labels(deps, @envoy_api//docs:protos))")

# This is for local RBE setup, should be no-op for builds without RBE setting in bazelrc files.
BAZEL_BUILD_OPTIONS+=" --remote_download_outputs=all --strategy=protoxform=sandboxed,local"

# TODO(htuch): This script started life by cloning docs/build.sh. It depends on
# the @envoy_api//docs:protos target in a few places as a result. This is not
# guaranteed to be the precise set of protos we want to format, but as a
# starting place it seems reasonable. In the future, we should change the logic
# here.
bazel build ${BAZEL_BUILD_OPTIONS} //tools:api_type_db
declare -x -r TYPE_DB_PATH="$(bazel info ${BAZEL_BUILD_OPTIONS} bazel-bin)/tools/api_type_db.pb_text"
bazel build ${BAZEL_BUILD_OPTIONS} @envoy_api//docs:protos --aspects \
  tools/protoxform/protoxform.bzl%protoxform_aspect --output_groups=proto --action_env=CPROFILE_ENABLED=1 \
  --action_env=TYPE_DB_PATH --host_force_python=PY3

./tools/proto_sync.py "$1" ${PROTO_TARGETS}
