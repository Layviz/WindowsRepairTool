[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$Installer
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
    throw 'vswhere.exe was not found. Install Visual Studio 2022 or its Build Tools with Desktop development with C++.'
}

$products = @('*')
if ($Installer) {
    $products = @(
        'Microsoft.VisualStudio.Product.Community',
        'Microsoft.VisualStudio.Product.Professional',
        'Microsoft.VisualStudio.Product.Enterprise'
    )
}
$vswhereArgs = @('-latest', '-version', '[17.0,18.0)', '-products') + $products + @(
    '-requires', 'Microsoft.Component.MSBuild', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
    '-property', 'installationPath'
)
$installation = & $vswhere @vswhereArgs
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio discovery failed with exit code $LASTEXITCODE."
}
if ([string]::IsNullOrWhiteSpace($installation)) {
    throw 'No suitable Visual Studio 2022 installation was found. Install Desktop development with C++ (MSVC v143 and a Windows SDK). Installer builds require full Visual Studio, not only Build Tools.'
}

$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path -LiteralPath $msbuild -PathType Leaf)) {
    throw "MSBuild was not found at '$msbuild'. Repair the Visual Studio 2022 installation."
}

if ($Installer) {
    $devenv = Join-Path $installation 'Common7\IDE\devenv.com'
    $disableOutOfProcBuild = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\VSI\DisableOutOfProcBuild\DisableOutOfProcBuild.exe'
    if (-not (Test-Path -LiteralPath $devenv -PathType Leaf) -or
        -not (Test-Path -LiteralPath $disableOutOfProcBuild -PathType Leaf)) {
        throw "Install Microsoft Visual Studio Installer Projects 2022 in '$installation'. MSBuild alone cannot build Setup.vdproj."
    }

    # Microsoft's per-user workaround is required for command-line .vdproj builds.
    Write-Host 'Configuring Installer Projects command-line builds for the current user...'
    & $disableOutOfProcBuild
    if ($LASTEXITCODE -ne 0) {
        throw "Installer Projects configuration failed with exit code $LASTEXITCODE."
    }
}

Push-Location $root
try {
    # Build the native projects directly so MSBuild never tries to load the .vdproj.
    $projects = @(
        'Windows Reparatur\Windows Reparatur.vcxproj',
        'WRTLauncher\WRTLauncher.vcxproj',
        'German Locale\German Locale.vcxproj'
    )
    foreach ($project in $projects) {
        Write-Host "Building $project ($Configuration|x64)..."
        & $msbuild $project /nologo /m /t:Build "/p:Configuration=$Configuration" /p:Platform=x64 "/p:OutDir=..\x64\$Configuration\"
        if ($LASTEXITCODE -ne 0) {
            throw "Building '$project' failed with exit code $LASTEXITCODE."
        }
    }

    if ($Installer) {
        # Setup is not enabled in the solution's x64 build mapping; select it explicitly.
        & $devenv 'Windows_Reparatur.sln' /Build "$Configuration|x64" /Project 'Setup\Setup.vdproj' /ProjectConfig $Configuration
        if ($LASTEXITCODE -ne 0) {
            throw "Installer build failed with exit code $LASTEXITCODE. See the build output for details."
        }
        $msi = Join-Path $root "Setup\$Configuration\Setup.msi"
        if (-not (Test-Path -LiteralPath $msi -PathType Leaf)) {
            throw "Installer build returned success but '$msi' was not generated."
        }
        Write-Host "Installer: $msi"
    }
    Write-Host "Native binaries: $root\x64\$Configuration"
}
finally {
    Pop-Location
}
