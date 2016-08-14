function extDynamicLib(location, dllname, env)
    local configs =
    {
        {
            filter="platforms:*32",
            arch="x86"
        },
        {
            filter="platforms:*64",
            arch="x64"
        }
    }
    for key,config in pairs(configs) do
        filter (config.filter)
            local command = '{COPY} "$(SolutionDir)..\\' .. location:gsub('/', '\\') .. '\\' .. config.arch .. '\\' .. dllname .. '.dll" "$(OutputPath)"'
            postbuildcommands { command }
    end

    filter {}
end

function extStaticLib(location, libname)
    includedirs { location .. "/include" }

    filter "platforms:*32"
        libdirs { location .. "/lib/x86" }

    filter "platforms:*64"
        libdirs { location .. "/lib/x64" }

    filter {}

    links { "" .. libname }
end

function configure()
    configurations { "Debug", "Release" }
    platforms { "Win32", "x64" }

    filter "platforms:*32"
        system "Windows"
        architecture "x86"

    filter "platforms:*64"
        system "Windows"
        architecture "x86_64"

    filter {}
    
    flags { "ExtraWarnings", "FatalWarnings" }
end

function baseProject(kind_name, name)
    project(name)
        kind(kind_name)
        language "C++"
        configure()
end

function staticLibrary(name)
    baseProject("StaticLib", name)
end

function dynamicLibrary(name, file_filters)
    baseProject("DynamicLib", name)
end

function application(name, file_filters)
    baseProject("ConsoleApp", name)
end

workspace "DSEmu"
    location "build"
    startproject "DSEmu"
    configure()

    filter "configurations:Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        flags { "Symbols", "Optimize" }

    filter {}
        defines { "_CRT_SECURE_NO_WARNINGS" }

application "DSEmu"
    files
    {
        "DSEmu/**.h", "DSEmu/**.cpp",
    }

application "ArmCpuGen"
    files
    {
        "Tools/ArmCpuGen/**.h", "Tools/ArmCpuGen/**.cpp",
    }
