#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$EngineRepository = "",
    [string]$EngineCheckout = "",
    [string]$Map = "configs/maps/editor-room.world.json",
    [string]$SaveAs = "",
    [string]$CandidateRef = "",
    [switch]$ValidateOnly,
    [switch]$BuildOnly
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}
function Read-Git {
    param([string]$Root, [string[]]$Arguments)
    $result = & git -C $Root @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git failed in $Root (exit $LASTEXITCODE)"
    }
    return (($result | Out-String).Trim())
}
function Resolve-ProjectPath {
    param([string]$Root, [string]$Value)
    if ([IO.Path]::IsPathRooted($Value)) {
        return [IO.Path]::GetFullPath($Value)
    }
    return [IO.Path]::GetFullPath((Join-Path $Root $Value))
}
function Assert-PinnedCheckout {
    param([string]$Root, [string]$Revision)
    $actualRoot = Read-Git $Root @("rev-parse", "--show-toplevel")
    if ((Resolve-Path -LiteralPath $actualRoot).Path -ne (Resolve-Path -LiteralPath $Root).Path) {
        throw "EngineCheckout must be the root of a real NovaCore Git checkout."
    }
    if ((Read-Git $Root @("rev-parse", "HEAD")) -cne $Revision) {
        throw "Engine checkout does not match the exact game pin $Revision. No checkout was changed."
    }
    if (Read-Git $Root @("status", "--porcelain", "--untracked-files=normal")) {
        throw "Engine checkout has local changes. Use the isolated pinned worktree instead."
    }
    if (-not (Test-Path -LiteralPath (Join-Path $Root "tools/world_editor/main.cpp") -PathType Leaf)) {
        throw "Pinned engine does not contain the native World Editor."
    }
}

$null = Get-Command git -ErrorAction Stop
$gameRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$gameRevision = Read-Git $gameRoot @("rev-parse", "HEAD")
$pin = (Get-Content -LiteralPath (Join-Path $gameRoot "cmake/novacore-revision.txt") -Raw).Trim()
$uiPin = (Get-Content -LiteralPath (Join-Path $gameRoot "cmake/novacore-ui-revision.txt") -Raw).Trim()
if ($pin -cnotmatch '^[0-9a-f]{40}$' -or $pin -cne $uiPin) {
    throw "Both NovaCore engine pins must be the same full lowercase commit SHA."
}
if ($gameRevision -cnotmatch '^[0-9a-f]{40}$') {
    throw "Could not resolve the exact Nemisis checkout."
}
$mapPath = Resolve-ProjectPath $gameRoot $Map
if (-not (Test-Path -LiteralPath $mapPath -PathType Leaf)) {
    throw "Map does not exist: $mapPath"
}
$catalog = Join-Path $gameRoot "configs/assets/nemisis_assets.json"
if (-not (Test-Path -LiteralPath $catalog -PathType Leaf)) {
    throw "Nemisis asset catalog is missing: $catalog"
}
$savePath = ""
if ($SaveAs) {
    $savePath = Resolve-ProjectPath $gameRoot $SaveAs
    if (-not (Test-Path -LiteralPath (Split-Path -Parent $savePath) -PathType Container)) {
        throw "Save As parent directory does not exist."
    }
}

$localRoot = Join-Path $gameRoot "build/world-editor-local"
$engineRoot = ""
if ($EngineCheckout) {
    $engineRoot = Resolve-ProjectPath $gameRoot $EngineCheckout
    Assert-PinnedCheckout $engineRoot $pin
    $engineRepo = $engineRoot
} else {
    if (-not $EngineRepository) { $EngineRepository = "../Novacore-Engine" }
    $engineRepo = (Resolve-Path -LiteralPath (Resolve-ProjectPath $gameRoot $EngineRepository)).Path
    $engineRoot = Join-Path $localRoot ("e" + $pin.Substring(0,12))
}

# The release path accepts only a pin reachable from the actual remote main.
# A separate explicit developer opt-in permits a named validation branch.
# Fetch writes Git metadata only; never check out, reset or stash user files.
$remoteRef = "refs/heads/main"
if ($CandidateRef) {
    if ($CandidateRef -cnotmatch '^editor/[A-Za-z0-9][A-Za-z0-9/_-]*$') {
        throw "CandidateRef must be an explicit editor/ branch name."
    }
    $remoteRef = "refs/heads/$CandidateRef"
    Write-Warning "Using an UNMERGED development candidate, not an integrated release."
}
Invoke-Checked git @("-C", $engineRepo, "fetch", "--no-tags", "origin", $remoteRef)
$reachable = Read-Git $engineRepo @("rev-parse", "FETCH_HEAD")
& git -C $engineRepo merge-base --is-ancestor $pin $reachable
if ($LASTEXITCODE -ne 0) {
    throw "Game pin $pin is not reachable from the requested remote revision $remoteRef. No local files were overwritten."
}
if (-not $EngineCheckout) {
    if (-not (Test-Path -LiteralPath $engineRoot)) {
        $null = New-Item -ItemType Directory -Path $localRoot -Force
        Invoke-Checked git @("-C", $engineRepo, "worktree", "add", "--detach", $engineRoot, $pin)
    }
    Assert-PinnedCheckout $engineRoot $pin
}

if (Read-Git $gameRoot @("status", "--porcelain", "--untracked-files=no")) {
    Write-Warning "Nemisis has tracked local changes. This local build includes those changes; its HEAD alone is not a clean-release identity."
}
Write-Host "Nemisis: $gameRevision"
Write-Host "NovaCore: $pin (reachable from $remoteRef)"
Write-Host "Engine checkout: $engineRoot"
Write-Host "Map: $mapPath"
if ($ValidateOnly) { return }

$null = Get-Command cmake -ErrorAction Stop
# Short directory keys avoid unnecessary Windows path-length pressure;
# the identity file prevents accidental reuse even if short keys collide.
$buildRoot = Join-Path $localRoot ("g" + $gameRevision.Substring(0,12) + "-e" + $pin.Substring(0,12))
$identityPath = Join-Path $buildRoot "world-editor-revisions.txt"
if (Test-Path -LiteralPath $buildRoot) {
    if (-not (Test-Path -LiteralPath $identityPath -PathType Leaf)) {
        throw "Existing isolated build directory has no verified revision identity; refusing reuse."
    }
    $identity = @(Get-Content -LiteralPath $identityPath)
    if ($identity.Count -ne 2 -or $identity[0] -cne $gameRevision -or $identity[1] -cne $pin) {
        throw "Existing isolated build directory belongs to different revisions."
    }
} else {
    $null = New-Item -ItemType Directory -Path $buildRoot -Force
    Set-Content -LiteralPath $identityPath -Value @($gameRevision,$pin) -Encoding ascii
}
# This independent CMake build never replaces existing playable Release output.
Invoke-Checked cmake @("-S", $gameRoot, "-B", $buildRoot, "-G", "Visual Studio 17 2022", "-A", "x64",
    "-DNOVACORE_ENGINE_ROOT=$engineRoot", "-DNOVACORE_ENABLE_SDL3=ON", "-DNOVACORE_ENABLE_VULKAN=ON",
    "-DNOVACORE_BUILD_WORLD_EDITOR=ON", "-DNOVACORE_REQUIRE_EDITOR_NATIVE=ON", "-DNEMISIS_COPY_RUNTIME_DATA=OFF")
Invoke-Checked cmake @("--build", $buildRoot, "--config", "Debug", "--target", "nemisis_game", "novacore_world_editor")
$gameExe = Join-Path $buildRoot "Debug/nemisis_game.exe"
$editorExe = Join-Path $buildRoot "Novacore-Engine/Debug/novacore_world_editor.exe"
foreach ($exe in @($gameExe, $editorExe)) {
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Expected executable missing: $exe" }
}
Write-Host "Game: $gameExe"
Write-Host "Editor: $editorExe"
if ($BuildOnly) { return }

# Only an explicit SaveAs authorizes overwriting an existing document. Opening
# the example does not implicitly authorize writing back to it.
$launchArguments = @("--map", $mapPath, "--game", $gameExe)
if ($savePath) { $launchArguments += @("--save-as", $savePath) }
Push-Location -LiteralPath $gameRoot
try {
    Invoke-Checked $editorExe $launchArguments
} finally {
    Pop-Location
}
