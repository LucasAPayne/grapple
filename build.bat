@echo off

@rem Default to release build
set is_debug=0

@rem Check first argument
if /i "%1" EQU "debug" (
    set is_debug=1
    shift
)

@rem /Oi generates intrinsic functions
@rem /Fc displays the full path to files in error messages
@rem /wd4201 disables the warning about nameless structs/unions
set common_flags=/nologo /Oi /FC /WX /W4 /wd4201
set output_names=/Fograpple.obj /Fegrapple.exe /Fmgrapple.map
set common_defs=/D_CRT_SECURE_NO_WARNINGS /DVC_EXTRALEAN /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DGRAPPLE_WIN32

@rem /Zi generates a PDB
@rem /Od disables optimization
@rem /MTd uses the debug multithreaded C library
set debug_flags=/DGRAPPLE_DEBUG /Zi /Od /MTd /fsanitize=address

set linker_flags=/link /opt:ref /incremental:no /subsystem:windows /entry:mainCRTStartup
set libs=kernel32.lib user32.lib d3d11.lib dxguid.lib

if "%is_debug%"=="1" (
    set compiler_flags=%common_flags% %common_defs% %debug_flags%
) else (
    set compiler_flags=%common_flags% %common_defs%
)

if not exist build mkdir build
pushd build
cl %compiler_flags% ..\src\main.c %output_names% %linker_flags% %libs%
popd
