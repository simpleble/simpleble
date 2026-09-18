param(
    [string]$DotNet = 'dotnet',
    [string]$CMake = 'cmake'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildDirectory = Join-Path $repoRoot 'build_simplesharpble_tests'

function Invoke-Checked {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE" }
}

if (-not $IsWindows) { throw 'Tests require Windows and MSVC.' }

# PLAIN builds need MSVC's UTF-8 and exception-unwinding flags explicitly.
Invoke-Checked $CMake @('-S', (Join-Path $repoRoot 'simplesharpble/test/native'), '-B', $buildDirectory,
    '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DSIMPLEBLE_PLAIN=ON',
    '-DBUILD_SHARED_LIBS=ON', '-DCMAKE_CXX_FLAGS=/utf-8 /EHsc')
Invoke-Checked $CMake @('--build', $buildDirectory, '--config', 'Release', '--parallel', '4')

$nativeDirectory = Join-Path $buildDirectory 'bin/Release'
Invoke-Checked $DotNet @('test', (Join-Path $repoRoot 'simplesharpble/test'),
    '-c', 'Release', "-p:NativeLibraryDirectory=$nativeDirectory")
