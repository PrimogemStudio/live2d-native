set_languages("c++23")
add_rules("mode.debug")
add_rules("mode.release")
add_rules("mode.releasedbg")

target("live2d-native")
    set_kind("shared")
    set_exceptions("cxx")
    set_policy("check.auto_ignore_flags", false)
    add_cxflags("-Wall -Werror")
    add_cflags("-Wall -Werror")
    add_files("src/**.cpp", "src/**.cppm")
    add_headerfiles("src/**.hpp", "src/**.h")
    add_sysincludedirs("include")
    set_policy("build.c++.modules", true)
    if is_plat("windows") then
        add_syslinks("advapi32")
        add_sysincludedirs("include/platform/windows")
        add_syslinks("lib/windows/Live2DCubismCore.lib")
        add_defines("CSM_TARGET_WIN_GL=1")
    else
        if is_plat("linux") then
            add_syslinks("lib/linux/libLive2DCubismCore.a")
            add_defines("CSM_TARGET_LINUX_GL=1")
        elseif is_plat("macosx") then
            add_syslinks("lib/macos/libLive2DCubismCore.a")
            add_defines("CSM_TARGET_MAC_GL=1")
        end
        add_sysincludedirs("include/platform/unix")
    end
