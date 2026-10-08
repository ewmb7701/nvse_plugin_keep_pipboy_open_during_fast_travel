[CmdletBinding()]
param(
 [ValidateSet('Debug','Release')][string]$Configuration='Debug',
 [switch]$Test,
 [switch]$Install
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Install Visual Studio Desktop development with C++.' }
$msbuild=Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$project=Join-Path $projectRoot 'source\nvse_plugin_keep_pipboy_open_during_fast_travel.vcxproj'
& $msbuild $project /m /t:Build "/p:Configuration=$Configuration" /p:Platform=Win32 /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed.' }
if ($Test) {
 & $msbuild $project /m /t:Build "/p:Configuration=$Configuration" /p:Platform=Win32 /p:RunTests=true /v:minimal
 if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
 $testExe=Join-Path $projectRoot "target\$Configuration\tests\keep_pipboy_open_during_fast_travel_tests.exe"
 & $testExe
 if ($LASTEXITCODE -ne 0) { throw "Tests failed: $LASTEXITCODE" }
}
if ($Install) {
 $workspaceRoot=Split-Path -Parent (Split-Path -Parent $projectRoot)
 $pluginDir=Join-Path $workspaceRoot 'mods\Keep Pip-Boy Open During Fast Travel\nvse\plugins'
 if (!(Test-Path -LiteralPath $pluginDir -PathType Container)) { throw "Standalone mod not found: $pluginDir" }
 Copy-Item -LiteralPath (Join-Path $projectRoot "target\$Configuration\keep_pipboy_open_during_fast_travel.dll") -Destination $pluginDir
 $modDir=Split-Path -Parent (Split-Path -Parent $pluginDir)
 Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md'), (Join-Path $projectRoot 'TESTING.md') -Destination $modDir
}
