"""Bazel rules for DaveCC C++20 modules."""

DaveccModuleInfo = provider(
    doc = "A DaveCC module artifact and its transitive link inputs.",
    fields = {
        "module_name": "Logical module or header-unit name.",
        "bmi": "The DaveCC .dcm artifact.",
        "object": "The module unit object file.",
        "bmis": "Transitive .dcm files.",
        "mappings": "Transitive logical-name=artifact-path strings.",
        "objects": "Transitive module object files.",
    },
)

def _module_args(ctx, args):
    args.add("-target", ctx.attr.target)
    args.add("-std=c++20")
    args.add("-c")
    all_mappings = depset(
        transitive = [dep[DaveccModuleInfo].mappings for dep in ctx.attr.deps],
    ).to_list()
    for mapping in sorted(all_mappings):
        args.add("-fmodule-file", mapping)
    args.add_all(ctx.attr.copts)

def _davecc_module_impl(ctx):
    src = ctx.file.src
    bmi = ctx.actions.declare_file(ctx.label.name + ".dcm")
    obj = ctx.actions.declare_file(ctx.label.name + ".o")
    transitive_bmis = [dep[DaveccModuleInfo].bmis for dep in ctx.attr.deps]
    transitive_objects = [dep[DaveccModuleInfo].objects for dep in ctx.attr.deps]
    transitive_mappings = [
        dep[DaveccModuleInfo].mappings
        for dep in ctx.attr.deps
    ]

    args = ctx.actions.args()
    _module_args(ctx, args)
    if ctx.attr.header_unit:
        args.add("-fmodule-header")
        args.add("-fmodule-name", ctx.attr.module_name)
    args.add("-fmodule-output", bmi.path)
    args.add(src.path)
    args.add("-o", obj.path)

    input_bmis = depset(transitive = transitive_bmis)
    ctx.actions.run(
        executable = ctx.executable._davecc,
        arguments = [args],
        inputs = depset([src], transitive = [input_bmis]),
        outputs = [bmi, obj],
        mnemonic = "DaveccModule",
        progress_message = "Compiling DaveCC module %s" % ctx.attr.module_name,
    )

    bmis = depset([bmi], transitive = transitive_bmis)
    objects = depset([obj], transitive = transitive_objects)
    mappings = depset(
        [ctx.attr.module_name + "=" + bmi.path],
        transitive = transitive_mappings,
    )
    return [
        DefaultInfo(files = depset([bmi, obj])),
        DaveccModuleInfo(
            module_name = ctx.attr.module_name,
            bmi = bmi,
            object = obj,
            bmis = bmis,
            mappings = mappings,
            objects = objects,
        ),
    ]

davecc_module = rule(
    implementation = _davecc_module_impl,
    attrs = {
        "src": attr.label(
            allow_single_file = [".cppm", ".ixx", ".cpp", ".cc", ".h", ".hpp", ".hxx"],
            mandatory = True,
        ),
        "module_name": attr.string(mandatory = True),
        "deps": attr.label_list(providers = [DaveccModuleInfo]),
        "header_unit": attr.bool(default = False),
        "target": attr.string(default = "x86_64"),
        "copts": attr.string_list(),
        "_davecc": attr.label(
            default = Label("//:davecc"),
            executable = True,
            cfg = "exec",
        ),
    },
    doc = "Builds coordinated DaveCC .dcm and object artifacts.",
)

def _davecc_binary_impl(ctx):
    src = ctx.file.src
    obj = ctx.actions.declare_file(ctx.label.name + ".main.o")
    output = ctx.actions.declare_file(ctx.label.name)
    module_bmis = [dep[DaveccModuleInfo].bmis for dep in ctx.attr.modules]
    module_objects = [dep[DaveccModuleInfo].objects for dep in ctx.attr.modules]
    mappings = depset(
        transitive = [dep[DaveccModuleInfo].mappings for dep in ctx.attr.modules],
    )

    compile_args = ctx.actions.args()
    compile_args.add("-target", ctx.attr.target)
    compile_args.add("-std=c++20")
    compile_args.add("-c")
    for mapping in sorted(mappings.to_list()):
        compile_args.add("-fmodule-file", mapping)
    compile_args.add_all(ctx.attr.copts)
    compile_args.add(src.path)
    compile_args.add("-o", obj.path)
    ctx.actions.run(
        executable = ctx.executable._davecc,
        arguments = [compile_args],
        inputs = depset([src], transitive = module_bmis),
        outputs = [obj],
        mnemonic = "DaveccCompile",
        progress_message = "Compiling DaveCC source %s" % src.short_path,
    )

    link_inputs = depset(
        [obj] + ctx.files.link_inputs,
        transitive = module_objects,
    )
    link_args = ctx.actions.args()
    link_args.add("-target", ctx.attr.target)
    if ctx.attr.static:
        link_args.add("-static")
    link_args.add_all(link_inputs)
    link_args.add_all(ctx.attr.linkopts)
    link_args.add("-o", output.path)
    ctx.actions.run(
        executable = ctx.executable._davecc,
        arguments = [link_args],
        inputs = link_inputs,
        outputs = [output],
        mnemonic = "DaveccLink",
        progress_message = "Linking DaveCC binary %s" % ctx.label.name,
    )
    return [DefaultInfo(files = depset([output]), executable = output)]

davecc_binary = rule(
    implementation = _davecc_binary_impl,
    attrs = {
        "src": attr.label(
            allow_single_file = [".cpp", ".cc", ".cxx"],
            mandatory = True,
        ),
        "modules": attr.label_list(providers = [DaveccModuleInfo]),
        "link_inputs": attr.label_list(allow_files = True),
        "target": attr.string(default = "x86_64"),
        "static": attr.bool(default = True),
        "copts": attr.string_list(),
        "linkopts": attr.string_list(),
        "_davecc": attr.label(
            default = Label("//:davecc"),
            executable = True,
            cfg = "exec",
        ),
    },
    executable = True,
    doc = "Compiles and links one DaveCC source with module dependencies.",
)
