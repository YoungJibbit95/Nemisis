param(
    [ValidateSet("all", "target", "prop", "props", "player", "arms", "soldier", "character", "characters", "ar", "smg", "sidearm", "weapon", "weapons", "arena", "environment", "environments")]
    [string]$Only = "all",

    [string]$BlenderPath = ""
)

$ErrorActionPreference = "Stop"

function Find-Blender {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (Test-Path -LiteralPath $ExplicitPath) {
            return (Resolve-Path -LiteralPath $ExplicitPath).Path
        }
        throw "BlenderPath does not exist: $ExplicitPath"
    }

    $fromPath = Get-Command blender -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $scriptDrive = Split-Path -Qualifier $PSScriptRoot
    $programFiles = @(
        "$env:ProgramFiles\Blender Foundation",
        "${env:ProgramFiles(x86)}\Blender Foundation",
        "$scriptDrive\Program Files\Blender Foundation",
        "$scriptDrive\Program Files (x86)\Blender Foundation"
    ) | Select-Object -Unique | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    foreach ($root in $programFiles) {
        $candidate = Get-ChildItem -LiteralPath $root -Recurse -Filter blender.exe -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }

    throw "Could not find Blender. Install Blender and add its folder to PATH, or pass -BlenderPath 'C:\Path\To\blender.exe'."
}

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")
$script = Join-Path $PSScriptRoot "make_dev_primitives.py"
$blender = Find-Blender -ExplicitPath $BlenderPath

Write-Host "Using Blender: $blender"
Write-Host "Generating Nemisis dev primitives: $Only"

Push-Location $repoRoot
try {
    & $blender --background --python $script -- --only $Only
    if ($LASTEXITCODE -ne 0) {
        throw "Blender exited with code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
