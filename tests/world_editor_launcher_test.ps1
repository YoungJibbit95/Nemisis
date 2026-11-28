#Requires -Version 5.1
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Git {
    param([string[]]$Arguments)
    $result = & git.exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Fixture git failed: $($Arguments -join ' ')" }
    return (($result | Out-String).Trim())
}
function Commit {
    param([string]$Root, [string]$Message)
    $null = Git @("-C",$Root,"add",".")
    $null = Git @("-C",$Root,"-c","user.name=Launcher Contract","-c","user.email=fixture@example.invalid","commit","-m",$Message)
    return (Git @("-C",$Root,"rev-parse","HEAD"))
}
function Assert {
    param([bool]$Condition,[string]$Message)
    if (-not $Condition) { throw $Message }
}
$fixture = Join-Path ([IO.Path]::GetTempPath()) ("nova-launcher-" + [Guid]::NewGuid().ToString("N").Substring(0,10))
$null = New-Item -ItemType Directory -Path $fixture
$engine = Join-Path $fixture "engine"
$remote = Join-Path $fixture "origin.git"
$game = Join-Path $fixture "game"
$launcher = Join-Path $game "tools/Launch-WorldEditor.ps1"
$shell = (Get-Process -Id $PID).Path

function Launch-Check {
    param([string[]]$Extra, [bool]$ExpectSuccess, [string]$MessagePart)
    $savedPreference = $ErrorActionPreference
    try {
        # Capture the child shell's actual status and diagnostics, including
        # intentional refusal cases, without aborting the fixture prematurely.
        $ErrorActionPreference = "Continue"
        $result = & $shell -NoProfile -File $launcher -EngineRepository $engine -ValidateOnly @Extra 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedPreference
    }
    $message = ($result | Out-String)
    Write-Host $message
    Assert (($code -eq 0) -eq $ExpectSuccess) "Unexpected launcher exit code $code"
    Assert ($message.Contains($MessagePart)) "Missing expected launcher diagnostic: $MessagePart"
}
try {
    $null = Git @("init","--bare","--initial-branch=main",$remote)
    $null = Git @("init","--initial-branch=main",$engine)
    $null = New-Item -ItemType Directory -Path (Join-Path $engine "tools/world_editor") -Force
    Set-Content -LiteralPath (Join-Path $engine "tools/world_editor/main.cpp") -Value "// fixture only" -Encoding ascii
    $mainPin = Commit $engine "main engine fixture"
    $null = Git @("-C",$engine,"remote","add","origin",$remote)
    $null = Git @("-C",$engine,"push","origin","main")
    $null = Git @("-C",$engine,"checkout","-b","editor/test-candidate")
    Set-Content -LiteralPath (Join-Path $engine "tools/world_editor/main.cpp") -Value "// candidate fixture" -Encoding ascii
    $candidatePin = Commit $engine "unmerged candidate"
    $null = Git @("-C",$engine,"push","origin","editor/test-candidate")
    $null = Git @("-C",$engine,"checkout","main")
    # Original checkout contains an unpublished user file; launcher must not
    # reset, stash, remove or modify it while constructing the pinned worktree.
    Set-Content -LiteralPath (Join-Path $engine "user-local.txt") -Value "preserve me" -Encoding ascii
    $engineStatus = Git @("-C",$engine,"status","--porcelain")

    $null = Git @("init","--initial-branch=main",$game)
    foreach ($directory in @("tools","cmake","configs/maps","configs/assets")) {
        $null = New-Item -ItemType Directory -Path (Join-Path $game $directory) -Force
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "../tools/Launch-WorldEditor.ps1") -Destination $launcher
    Set-Content -LiteralPath (Join-Path $game ".gitignore") -Value "build/" -Encoding ascii
    Set-Content -LiteralPath (Join-Path $game "configs/maps/editor-room.world.json") -Value "{}" -Encoding ascii
    Set-Content -LiteralPath (Join-Path $game "configs/assets/nemisis_assets.json") -Value "{}" -Encoding ascii
    Set-Content -LiteralPath (Join-Path $game "cmake/novacore-revision.txt") -Value $mainPin -Encoding ascii
    Set-Content -LiteralPath (Join-Path $game "cmake/novacore-ui-revision.txt") -Value $mainPin -Encoding ascii
    $null = Commit $game "game fixture"

    Launch-Check @() $true "NovaCore: $mainPin"
    $pinnedMain = Join-Path $game ("build/world-editor-local/e" + $mainPin.Substring(0,12))
    Assert ((Git @("-C",$pinnedMain,"rev-parse","HEAD")) -ceq $mainPin) "Incorrect main worktree pin"
    Assert ((Git @("-C",$engine,"status","--porcelain")) -ceq $engineStatus) "Original engine modified"
    Assert ((Get-Content -LiteralPath (Join-Path $engine "user-local.txt") -Raw).Trim() -ceq "preserve me") "User file modified"

    Set-Content -LiteralPath (Join-Path $game "cmake/novacore-revision.txt") -Value $candidatePin -Encoding ascii
    Set-Content -LiteralPath (Join-Path $game "cmake/novacore-ui-revision.txt") -Value $candidatePin -Encoding ascii
    Launch-Check @() $false "not reachable"
    Launch-Check @("-CandidateRef","editor/test-candidate") $true "UNMERGED development candidate"
    $pinnedCandidate = Join-Path $game ("build/world-editor-local/e" + $candidatePin.Substring(0,12))
    Assert ((Git @("-C",$pinnedCandidate,"rev-parse","HEAD")) -ceq $candidatePin) "Incorrect candidate worktree pin"
    Launch-Check @("-EngineCheckout",$pinnedCandidate,"-CandidateRef","editor/test-candidate") $true "NovaCore: $candidatePin"
    Set-Content -LiteralPath (Join-Path $pinnedCandidate "local-dirty.txt") -Value "keep" -Encoding ascii
    Launch-Check @("-EngineCheckout",$pinnedCandidate,"-CandidateRef","editor/test-candidate") $false "local changes"

    Set-Content -LiteralPath (Join-Path $game "cmake/novacore-ui-revision.txt") -Value $mainPin -Encoding ascii
    Launch-Check @() $false "Both NovaCore engine pins"
    Assert ((Git @("-C",$engine,"rev-parse","HEAD")) -ceq $mainPin) "Original engine HEAD moved"
    Assert ((Git @("-C",$engine,"status","--porcelain")) -ceq $engineStatus) "Original engine was modified"
    Assert (-not (Get-ChildItem -LiteralPath (Join-Path $game "build/world-editor-local") -Recurse -Filter CMakeCache.txt)) "ValidateOnly invoked a build"
    Write-Host "Windows launcher revision/isolated-worktree safety contract passed"
} finally {
    # Only the unique temporary fixture, never a real project checkout.
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
