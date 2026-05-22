-- Copyright (C) 2026  ilobilo

set_project("resolution")
set_version("v0.1")

set_license("EUPL-1.2")

add_rules("mode.release")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "build" })

set_policy("package.install_locally", true)

set_policy("run.autobuild", true)

set_policy("build.c++.modules", true)
if is_plat("mingw") then
    set_policy("build.c++.modules.std", false)
else
    set_policy("build.c++.modules.std", true)
end

set_warnings("all")
set_optimize("fastest")

set_languages("c++23")
set_runtimes("c++_static")

if is_plat("mingw") then
    set_toolchains("mingw")
else -- linux
    set_toolchains("clang")
end

add_requires("magic_enum", "argparse")

target("resolve")
    set_kind("binary")

    add_packages("magic_enum", "argparse")

    add_files("source/**.cpp")
    add_files("source/**.cppm")

    if is_plat("mingw") then
        -- TODO: a workaround for std module. works on fedora 44 with mingw64-gcc-c++ installed
        local mingw_cxx_inc = "/usr/x86_64-w64-mingw32/sys-root/mingw/include/c++/bits"
        add_files(mingw_cxx_inc .. "/std.cc", mingw_cxx_inc .. "/std.compat.cc")
        add_syslinks("stdc++exp")
        add_ldflags("-static-libgcc", "-static-libstdc++", "-static", { force = true })
    end

    set_rundir("$(projectdir)")
