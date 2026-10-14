[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [switch] $BuildInstaller
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}

if ( $env:CI -eq $null ) {
    throw "Package-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "Packaging script requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Install-InnoSetup {
    $iscc = Get-Command iscc -ErrorAction SilentlyContinue
    if ( $iscc ) {
        return
    }

    $defaultIscc = "${Env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    if ( Test-Path -Path $defaultIscc ) {
        $env:Path = "$(Split-Path -Parent $defaultIscc);${env:Path}"
        return
    }

    Log-Information 'Installing Inno Setup via Chocolatey...'
    Invoke-External choco install innosetup -y --no-progress
    if ( Test-Path -Path $defaultIscc ) {
        $env:Path = "$(Split-Path -Parent $defaultIscc);${env:Path}"
    }

    if ( -not ( Get-Command iscc -ErrorAction SilentlyContinue ) ) {
        throw 'Inno Setup (iscc) was not found after installation.'
    }
}

function Package {
    trap {
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name
    $ProductVersion = $BuildSpec.version

    $OutputName = "${ProductName}-${ProductVersion}-windows-${Target}"

    $RemoveArgs = @{
        ErrorAction = 'SilentlyContinue'
        Path = @(
            "${ProjectRoot}/release/${ProductName}-*-windows-*.zip"
            "${ProjectRoot}/release/${ProductName}-*-windows-*.exe"
            "${ProjectRoot}/release/Package"
        )
    }

    Remove-Item @RemoveArgs

    Log-Group "Archiving ${ProductName}..."
    $CompressArgs = @{
        Path = (Get-ChildItem -Path "${ProjectRoot}/release/${Configuration}" -Exclude "${OutputName}*.*")
        CompressionLevel = 'Optimal'
        DestinationPath = "${ProjectRoot}/release/${OutputName}.zip"
        Verbose = ($Env:CI -ne $null)
    }
    Compress-Archive -Force @CompressArgs
    Log-Group

    if ( $BuildInstaller ) {
        Log-Group "Creating Windows installer for ${ProductName}..."

        $IsccFile = "${ProjectRoot}/build_${Target}/installer-Windows.generated.iss"
        if ( ! ( Test-Path -Path $IsccFile ) ) {
            throw 'InnoSetup install script not found. Run the CMake configure/build first.'
        }

        $PluginReleaseDir = "${ProjectRoot}/release/${Configuration}/${ProductName}"
        if ( ! ( Test-Path -Path $PluginReleaseDir ) ) {
            throw "Plugin release directory not found: ${PluginReleaseDir}"
        }

        Install-InnoSetup

        # Remap ProgramData-style layout to OBS application layout for the installer
        New-Item -ItemType Directory -Path "${ProjectRoot}/release/Package/obs-plugins" -Force | Out-Null
        New-Item -ItemType Directory -Path "${ProjectRoot}/release/Package/data/obs-plugins/${ProductName}" -Force | Out-Null
        Copy-Item -Path "${PluginReleaseDir}/bin/*" -Destination "${ProjectRoot}/release/Package/obs-plugins" -Recurse -Force
        Copy-Item -Path "${PluginReleaseDir}/data/*" -Destination "${ProjectRoot}/release/Package/data/obs-plugins/${ProductName}" -Recurse -Force

        Push-Location -Stack BuildTemp
        Ensure-Location -Path "${ProjectRoot}/release"
        Invoke-External iscc ${IsccFile} /O"${ProjectRoot}/release" /F"${OutputName}-Installer"
        Remove-Item -Path Package -Recurse -Force -ErrorAction SilentlyContinue
        Pop-Location -Stack BuildTemp

        Log-Group
    }
}

Package
