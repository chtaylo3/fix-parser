<#
.SYNOPSIS
  Provision the local Windows toolchain to match the GitHub Actions runner.

.DESCRIPTION
  CMake finds a toolchain; it does not install one. This script installs the
  host prerequisites that our CI workflows assume are present on the runner
  image, so `cmake --preset ci` / `--preset sanitize` behave the same locally:

    * LLVM             - clang-tidy ("tidy" preset) and clang-cl ("fuzz" preset).
    * Ninja            - generator used by the ci / sanitize / tidy / fuzz presets.
    * CMake (>= 3.21)  - build-system generator.

  It is idempotent: anything already present is reported and skipped. Visual
  Studio (MSVC + Windows SDK) and vcpkg are NOT installed here -- VS is a large
  interactive install, and vcpkg lives wherever you cloned it (VCPKG_ROOT).

.NOTES
  Run from a normal PowerShell prompt. winget may prompt for elevation per package.
#>
[CmdletBinding()]
param(
  # Skip the winget installs and only report what is present.
  [switch]$CheckOnly
)

$ErrorActionPreference = 'Stop'

# Locate a tool on PATH or at a known fallback path. Returns the path or $null.
function Find-Tool($exe, $fallback) {
  $c = Get-Command $exe -ErrorAction SilentlyContinue
  if ($c) { return $c.Source }
  if ($fallback -and (Test-Path $fallback)) { return $fallback }
  return $null
}

function Install-WingetPackage($id, $exe, $fallback) {
  $found = Find-Tool $exe $fallback
  if ($found) { Write-Host "  [ok]      $id ($found)"; return }
  if ($CheckOnly) { Write-Host "  [missing] $id (run without -CheckOnly to install)"; return }
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    throw "winget is not available; install '$id' manually (https://github.com/microsoft/winget-cli)."
  }
  Write-Host "  installing $id ..."
  # Don't trust winget's exit code (it returns non-zero for 'already installed' /
  # 'no upgrade'); verify the tool is present afterwards instead.
  winget install --exact --id $id --accept-package-agreements --accept-source-agreements --disable-interactivity | Out-Null
  $found = Find-Tool $exe $fallback
  if (-not $found) { throw "Failed to install $id; install it manually." }
  Write-Host "  [ok]      $id ($found)"
}

Write-Host "== fix-parser dev-environment setup ==`n"

$llvmBin = Join-Path $env:ProgramFiles 'LLVM\bin'

Write-Host "Toolchain prerequisites:"
Install-WingetPackage 'LLVM.LLVM'         'clang-cl' (Join-Path $llvmBin 'clang-cl.exe')                  # clang-tidy + clang-cl (fuzz)
Install-WingetPackage 'Ninja-build.Ninja' 'ninja'    $null                                                # ci / sanitize generator
Install-WingetPackage 'Kitware.CMake'     'cmake'    (Join-Path $env:ProgramFiles 'CMake\bin\cmake.exe')   # build-system generator

# LLVM's installer may not refresh PATH in the current shell; surface its bin dir.
if ((Test-Path (Join-Path $llvmBin 'clang-cl.exe')) -and -not (Get-Command clang-cl -ErrorAction SilentlyContinue)) {
  Write-Host "`n  note: clang-cl is at $llvmBin but not on PATH in this shell."
  Write-Host "        Open a new terminal, or run: `$env:PATH = '$llvmBin;' + `$env:PATH"
}

Write-Host "`nEnvironment checks (provided by you / Visual Studio, not installed here):"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
  $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if ($vs) { Write-Host "  [ok]      Visual Studio (VC tools) -> $vs" }
  else     { Write-Host "  [missing] Visual Studio C++ workload (MSVC + Windows SDK)" }
} else {
  Write-Host "  [missing] Visual Studio Installer / vswhere"
}
if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake")) {
  Write-Host "  [ok]      vcpkg -> $env:VCPKG_ROOT"
} else {
  Write-Host "  [missing] VCPKG_ROOT not set / not a vcpkg checkout (see README)"
}

Write-Host "`nDone. For the sanitize preset, build from a VS dev shell so cl/clang-cl/ninja are on PATH:"
Write-Host "  cmake --preset sanitize; cmake --build --preset sanitize; ctest --preset sanitize"
