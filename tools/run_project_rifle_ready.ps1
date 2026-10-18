param(
    [string]$Executable = (Join-Path $PSScriptRoot '../build/phase3-vulkan/nemisis_game.exe'),
    [switch]$LookDown
)

$ErrorActionPreference = 'Stop'
$gamePath = (Resolve-Path -LiteralPath $Executable).Path
$repoPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Write-Host "Project developer Rifle Ready: $gamePath"
Write-Host "EXE SHA256: $((Get-FileHash -LiteralPath $gamePath -Algorithm SHA256).Hash)"
$gameArguments = @('--project-rifle-ready', '--vulkan')
if ($LookDown) { $gameArguments += '--project-fps-look-down' }
Push-Location -LiteralPath $repoPath
try {
    & $gamePath @gameArguments
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
