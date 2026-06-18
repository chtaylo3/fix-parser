<#
.SYNOPSIS
  Assemble sideload-ready FIX Parser plugin packages for Notepad++ (x64 and x86).

.DESCRIPTION
  Each package contains the self-contained plugin DLL plus its dictionaries, laid
  out as a `FixParser\` folder ready to drop into a Notepad++ `plugins\` directory.
  The DLLs and dictionaries must already be built/configured via:

      cmake --build --preset vs2022-release     --target fixparser_plugin   # x64
      cmake --preset vs2022-x86
      cmake --build --preset vs2022-x86-release --target fixparser_plugin    # x86

.PARAMETER Version
  Version label used in the zip name, e.g. 0.1.0-alpha.

.PARAMETER X64BuildDir
  Build directory containing the x64 plugin (expects <dir>\Release\FixParser.dll
  and <dir>\dictionaries). Defaults to the local vs2022 preset output. In CI use
  the version-agnostic Ninja preset dir, e.g. build\ci.

.PARAMETER X86BuildDir
  Build directory containing the x86 plugin. Defaults to vs2022-x86; in CI use
  build\ci-x86.
#>
param(
  [string]$Version = "0.1.0-alpha",
  [string]$X64BuildDir,
  [string]$X86BuildDir
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist '_stage'

if (-not $X64BuildDir) { $X64BuildDir = "$root\build\vs2022" }
if (-not $X86BuildDir) { $X86BuildDir = "$root\build\vs2022-x86" }

# arch name -> (built DLL path, dictionaries dir produced at configure time).
# Both the VS and Ninja-Multi-Config generators emit <dir>\Release\FixParser.dll.
$targets = @(
  @{ Arch = 'x64'; Dll = "$X64BuildDir\Release\FixParser.dll"; Dict = "$X64BuildDir\dictionaries" },
  @{ Arch = 'x86'; Dll = "$X86BuildDir\Release\FixParser.dll"; Dict = "$X86BuildDir\dictionaries" }
)

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
New-Item -ItemType Directory -Path $dist  -Force | Out-Null

foreach ($t in $targets) {
  $arch = $t.Arch
  if (-not (Test-Path $t.Dll))  { throw "Missing DLL for $arch : $($t.Dll) - build it first." }
  if (-not (Test-Path $t.Dict)) { throw "Missing dictionaries for $arch : $($t.Dict) - configure first." }

  # plugins\FixParser\ layout
  $plugDir = Join-Path $stage "$arch\FixParser"
  $dictDir = Join-Path $plugDir 'dictionaries'
  New-Item -ItemType Directory -Path $dictDir -Force | Out-Null

  Copy-Item $t.Dll (Join-Path $plugDir 'FixParser.dll') -Force
  Copy-Item (Join-Path $t.Dict '*') $dictDir -Recurse -Force

  # Docs travel inside the plugin folder so plugins\ root stays clean.
  Copy-Item "$root\LICENSE"                 (Join-Path $plugDir 'LICENSE.txt')          -Force
  Copy-Item "$root\THIRD_PARTY_NOTICES.md"  (Join-Path $plugDir 'THIRD_PARTY_NOTICES.md') -Force

  $installTxt = @"
FIX Parser - Notepad++ plugin ($Version, $arch) - ALPHA

REQUIREMENTS
  This is the $arch build. It MUST match your Notepad++ architecture.
  Check yours in Notepad++:  ? (Help) menu -> Debug Info -> "Build ... (64-bit)" or "(32-bit)".
    64-bit Notepad++  -> use the x64 package
    32-bit Notepad++  -> use the x86 package
  A mismatched build will silently fail to load.

INSTALL (sideload)
  1. Close Notepad++.
  2. Copy the "FixParser" folder (the one containing this file) into your
     Notepad++ plugins directory:
       Installed 64-bit : C:\Program Files\Notepad++\plugins\
       Installed 32-bit : C:\Program Files (x86)\Notepad++\plugins\
       Portable         : <your-Notepad++-folder>\plugins\
     Result: ...\plugins\FixParser\FixParser.dll
  3. Start Notepad++. The plugin appears under  Plugins -> FIX Parser.

USAGE
  Open a FIX capture, then  Plugins -> FIX Parser -> Pretty-print FIX log.
  Hover a FIX line for a quickview; double-click a line to open the dockable panel.

NOTES
  - Self-contained: no extra runtime DLLs required (static CRT + pugixml).
  - Dictionaries live in the "dictionaries" subfolder beside the DLL; keep them together.
  - Files must be opened in UTF-8 or ANSI encoding (convert UTF-16 first).

LICENSE
  GPL-2.0. See LICENSE.txt and THIRD_PARTY_NOTICES.md (bundled dictionaries:
  dictionaries\NOTICE.txt).
"@
  Set-Content -Path (Join-Path $plugDir 'INSTALL.txt') -Value $installTxt -Encoding utf8

  $zip = Join-Path $dist "FixParser-$Version-$arch.zip"
  if (Test-Path $zip) { Remove-Item $zip -Force }
  Compress-Archive -Path $plugDir -DestinationPath $zip -CompressionLevel Optimal
  $sha = (Get-FileHash $zip -Algorithm SHA256).Hash
  "{0}  ({1:N0} bytes)`n  SHA-256: {2}" -f (Split-Path $zip -Leaf), (Get-Item $zip).Length, $sha
}

Remove-Item $stage -Recurse -Force
"`nPackages written to: $dist"
