load("//tools/api_proto_plugin:plugin.bzl", "api_proto_plugin_aspect", "api_proto_plugin_impl")

def _protoxform_impl(target, ctx):
    return api_proto_plugin_impl(target, ctx, "proto", "protoxform", [".v2.proto", ".v3alpha.proto"])

# Bazel aspect (https://docs.bazel.build/versions/master/skylark/aspects.html)
# that can be invoked from the CLI to perform API transforms via //tools/protoxform for
# proto_library targets. Example use:
#
#   bazel build //api --aspects tools/protoxform/protoxform.bzl%protoxform_aspect \
#       --output_groups=proto
protoxform_aspect = api_proto_plugin_aspect("//tools/protoxform", _protoxform_impl, use_type_db = True)

def _run_impl(ctx):
    proto_deps = []
    for target in ctx.attr.targets:
        proto_deps.append(target[OutputGroupInfo].proto)

    args = []
    for deps in proto_deps:
        for dep in deps.to_list():
            if dep.owner.workspace_name in ctx.attr.proto_repositories:
                args.append("{}".format(dep.path))
    ctx.actions.write(ctx.outputs.proto_list, "\n".join(args))

_run_attrs = {
    "targets": attr.label_list(
        aspects = [protoxform_aspect],
        doc = "List of all proto_library target to be included.",
    ),
    "proto_repositories": attr.string_list(
        default = ["envoy_api"],
        allow_empty = False,
    ),
}

protoxform = rule(attrs = _run_attrs, outputs = {"proto_list": "%{name}.proto_list"}, implementation = _run_impl)
