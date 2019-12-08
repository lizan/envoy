load("@rules_proto//proto:defs.bzl", "ProtoInfo")
load("//tools/protoxform:protoxform.bzl", "protoxform_aspect")

def _protosync_impl(ctx):
    proto_sources = []
    proto_deps = []
    for target in ctx.attr.targets:
        proto_sources.extend(target[ProtoInfo].transitive_sources.to_list())
        proto_sources.extend(target[ProtoInfo].direct_sources)
        proto_deps.append(target[OutputGroupInfo].proto)

    proto_files = []
    for deps in proto_deps:
        for dep in deps.to_list():
            if dep.owner.workspace_name in ctx.attr.proto_repositories:
                proto_files.append(dep)

    runfiles = ctx.runfiles(proto_files + [ctx.outputs.proto_list])
    proto_sources = [
        label.owner
        for label in proto_sources
        if label.owner.workspace_name in ctx.attr.proto_repositories
    ]
    executable = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(ctx.outputs.proto_list, "".join(["{}\n".format(label) for label in proto_sources]))

    ctx.actions.write(
        executable,
        "\n".join(
            [
                "#!/usr/bin/env bash",
                "",
                "",
                # "cd $1",
                "echo {} --mode=$2 \\".format(ctx.expand_location("fooo", [ctx.attr._proto_sync])),
            ] +
            ["  {} \\".format(label) for label in proto_sources],
        ),
        is_executable = True,
    )

    return [
        DefaultInfo(
            executable = executable,
            runfiles = runfiles,
        ),
    ]

_sync_attrs = {
    "_proto_sync": attr.label(
        default = Label("//tools/protosync:tool"),
        executable = True,
        cfg = "target",
    ),
    "targets": attr.label_list(
        aspects = [protoxform_aspect],
        doc = "List of all proto_library target to be included.",
    ),
    "proto_repositories": attr.string_list(
        default = ["envoy_api"],
        allow_empty = False,
    ),
}

protosync = rule(
    attrs = _sync_attrs,
    outputs = {"proto_list": "%{name}.proto_list"},
    implementation = _protosync_impl,
    executable = True,
)
