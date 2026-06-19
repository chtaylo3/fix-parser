<#
.SYNOPSIS
  Assemble FIX Parser plugin packages for Notepad++ (x64 and x86/Win32).

.DESCRIPTION
  For each architecture this emits TWO zips from identical content:

    1. Root-layout (the Notepad++ norm, e.g. FixParser_0.1.0_x64.zip)
       FixParser.dll + dictionaries\ + docs at the ZIP ROOT. This is what the
       Plugins Admin / nppPluginList validator expects (it locates the DLL by
       exact path) and is also the standard manual-install download.

    2. Drop-in folder (e.g. FixParser_0.1.0_x64_portable.zip)
       The same payload wrapped in a top-level "FixParser\" folder, so a tester
       can extract it straight into a portable Notepad++ "plugins\" directory.

  Architecture tokens follow the Notepad++ convention: x64 stays "x64"; the
  32-bit build is labelled "Win32" (as ComparePlus / NppJSONViewer do), not x86.

  The DLLs and dictionaries must already be built/configured via:

      cmake --build --preset vs2022-release     --target fixparser_plugin   # x64
      cmake --preset vs2022-x86
      cmake --build --preset vs2022-x86-release --target fixparser_plugin    # x86

.PARAMETER Version
  Version label used in the zip name, e.g. 0.1.0.

.PARAMETER X64BuildDir
  Build directory containing the x64 plugin (expects <dir>\Release\FixParser.dll
  and <dir>\dictionaries). Defaults to the local vs2022 preset output. In CI use
  the version-agnostic Ninja preset dir, e.g. build\ci.

.PARAMETER X86BuildDir
  Build directory containing the x86 plugin. Defaults to vs2022-x86; in CI use
  build\ci-x86.
#>
param(
  [string]$Version = "0.1.0",
  [string]$X64BuildDir,
  [string]$X86BuildDir
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist '_stage'

if (-not $X64BuildDir) { $X64BuildDir = "$root\build\vs2022" }
if (-not $X86BuildDir) { $X86BuildDir = "$root\build\vs2022-x86" }

# arch name -> (built DLL path, dictionaries dir, Notepad++ zip-name token).
# Both the VS and Ninja-Multi-Config generators emit <dir>\Release\FixParser.dll.
# The 32-bit token is "Win32" to match the Notepad++ plugin naming convention.
$targets = @(
  @{ Arch = 'x64'; Token = 'x64';   Dll = "$X64BuildDir\Release\FixParser.dll"; Dict = "$X64BuildDir\dictionaries" },
  @{ Arch = 'x86'; Token = 'Win32'; Dll = "$X86BuildDir\Release\FixParser.dll"; Dict = "$X86BuildDir\dictionaries" }
)

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
New-Item -ItemType Directory -Path $dist  -Force | Out-Null

Add-Type -AssemblyName System.IO.Compression          # ZipArchive, ZipArchiveMode
Add-Type -AssemblyName System.IO.Compression.FileSystem # ZipFileExtensions

# Build a zip from $SourceDir, writing entries with forward-slash separators (the
# ZIP spec separator that the Linux-based nppPluginList validator and stock unzip
# tools expect -- PowerShell's Compress-Archive emits backslashes). $Prefix is
# prepended to every entry: "" yields a root layout (DLL at the zip root), while
# "FixParser/" yields the drop-in folder layout.
function New-Zip([string]$SourceDir, [string]$Prefix, [string]$Zip) {
  if (Test-Path $Zip) { Remove-Item $Zip -Force }
  $base = (Resolve-Path $SourceDir).Path.TrimEnd('\')
  $fs = [System.IO.File]::Open($Zip, [System.IO.FileMode]::CreateNew)
  $archive = New-Object System.IO.Compression.ZipArchive($fs, [System.IO.Compression.ZipArchiveMode]::Create)
  try {
    foreach ($f in Get-ChildItem -Path $SourceDir -Recurse -File | Sort-Object FullName) {
      $rel = $f.FullName.Substring($base.Length + 1).Replace('\', '/')
      [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $archive, $f.FullName, "$Prefix$rel",
        [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
  } finally {
    $archive.Dispose(); $fs.Dispose()
  }
  # SHA-256 is printed lowercase because that is the form nppPluginList expects in
  # the manifest "id" field.
  $sha = (Get-FileHash $Zip -Algorithm SHA256).Hash.ToLower()
  "{0}  ({1:N0} bytes)`n  SHA-256: {2}" -f (Split-Path $Zip -Leaf), (Get-Item $Zip).Length, $sha
}

foreach ($t in $targets) {
  $arch  = $t.Arch
  $token = $t.Token
  if (-not (Test-Path $t.Dll))  { throw "Missing DLL for $arch : $($t.Dll) - build it first." }
  if (-not (Test-Path $t.Dict)) { throw "Missing dictionaries for $arch : $($t.Dict) - configure first." }

  # Stage the plugin payload once as plugins\FixParser\ ; both zip layouts reuse it.
  $plugDir = Join-Path $stage "$token\FixParser"
  $dictDir = Join-Path $plugDir 'dictionaries'
  New-Item -ItemType Directory -Path $dictDir -Force | Out-Null

  Copy-Item $t.Dll (Join-Path $plugDir 'FixParser.dll') -Force
  Copy-Item (Join-Path $t.Dict '*') $dictDir -Recurse -Force

  Copy-Item "$root\LICENSE"                 (Join-Path $plugDir 'LICENSE.txt')             -Force
  Copy-Item "$root\THIRD_PARTY_NOTICES.md"  (Join-Path $plugDir 'THIRD_PARTY_NOTICES.md') -Force

  $installTxt = @"
FixParser - Notepad++ plugin ($Version, $token)

REQUIREMENTS
  This is the $token build. It MUST match your Notepad++ architecture.
  Check yours in Notepad++:  ? (Help) menu -> Debug Info -> "Build ... (64-bit)" or "(32-bit)".
    64-bit Notepad++  -> use the x64 package
    32-bit Notepad++  -> use the Win32 package
  A mismatched build will silently fail to load.

INSTALL
  Easiest: Notepad++ -> Plugins -> Plugins Admin -> search "FixParser" -> Install.

  Manual (root-layout zip, FixParser_$Version`_$token.zip):
    1. Close Notepad++.
    2. Create a folder named "FixParser" in your Notepad++ plugins directory and
       extract this zip's contents into it, so you end up with:
         ...\plugins\FixParser\FixParser.dll
       Plugins directory:
         Installed 64-bit : C:\Program Files\Notepad++\plugins\
         Installed 32-bit : C:\Program Files (x86)\Notepad++\plugins\
         Portable         : <your-Notepad++-folder>\plugins\
    3. Start Notepad++. The plugin appears under  Plugins -> FixParser.

  Manual (drop-in zip, FixParser_$Version`_$token`_portable.zip):
    This zip already contains the "FixParser" folder -- just extract it directly
    into your plugins directory, then start Notepad++.

USAGE
  Open a FIX capture, then  Plugins -> FixParser -> Pretty-print FIX log.
  Hover a FIX line for a quickview; double-click a line to open the dockable panel.

NOTES
  - Self-contained: no extra runtime DLLs required (static CRT + pugixml).
  - Dictionaries live in the "dictionaries" subfolder beside the DLL; keep them together.
  - Files must be opened in UTF-8 or ANSI encoding (convert UTF-16 first).

PROJECT / AUTHOR
  Chris Taylor (github.com/chtaylo3)
  https://github.com/chtaylo3/fix-parser

LICENSE
  GPL-2.0. See LICENSE.txt and THIRD_PARTY_NOTICES.md (bundled dictionaries:
  dictionaries\NOTICE.txt).
"@
  Set-Content -Path (Join-Path $plugDir 'INSTALL.txt') -Value $installTxt -Encoding utf8

  # 1) Root-layout zip (the Notepad++ norm): DLL + data at the zip root.
  New-Zip -SourceDir $plugDir -Prefix "" -Zip (Join-Path $dist "FixParser_$Version`_$token.zip")

  # 2) Drop-in zip: the FixParser/ folder wrapped at the zip root.
  New-Zip -SourceDir $plugDir -Prefix "FixParser/" -Zip (Join-Path $dist "FixParser_$Version`_$token`_portable.zip")
}

Remove-Item $stage -Recurse -Force
"`nPackages written to: $dist"
