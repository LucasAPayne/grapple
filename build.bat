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
set debug_flags=/DGRAPPLE_DEBUG /Zi /Od /MTd
set release_flags=/O2

set linker_flags=/link /opt:ref /incremental:no /subsystem:windows /entry:mainCRTStartup
set libs=kernel32.lib user32.lib d3d11.lib dxguid.lib
set debug_libs=kernel32.lib user32.lib d3dx11d.lib dxguid.lib

if "%is_debug%"=="1" (
    set compiler_flags=%common_flags% %common_defs% %debug_flags%
) else (
    set compiler_flags=%common_flags% %common_defs% %release_flags%
)

@rem /Zpc packs matrices in column major order
@rem /Ges enables strict mode
set shader_common_flags=/nologo /WX /Zpc /Ges /Qstrip_reflect /Qstrip_priv

@rem /Od disables optimizations
set shader_debug_flags=/Od
set shader_release_flags=/O3 /Qstrip_debug

if "%is_debug%"=="1" (
    set shader_flags=%shader_common_flags% %shader_debug_flags%
) else (
    set shader_flags=%shader_common_flags% %shader_release_flags%
)

if not exist shaders\compiled mkdir shaders\compiled
fxc %shader_flags% /T vs_5_0 /E vs /Fh shaders/compiled/d3d11_vshader.h /Vn d3d11_vshader shaders/shader.hlsl
fxc %shader_flags% /T ps_5_0 /E ps /Fh shaders/compiled/d3d11_pshader.h /Vn d3d11_pshader shaders/shader.hlsl

if not exist build mkdir build
pushd build
cl %compiler_flags% /I.. ..\src\main.c %output_names% %linker_flags% %libs%
popd
