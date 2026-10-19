<#
.SYNOPSIS
    Syncs the Nemisis workspace checkouts to the newest merged upstream state and
    rebuilds the CLion game executable.

.DESCRIPTION
    The workspace keeps one primary checkout per repository (Nemisis and
    Novacore-Engine) plus a number of feature worktrees. Work is published as
    pull requests and merged into main, so the primary checkouts are the ones
    that must carry every merged change.

    This script:
      1. fetches every remote for both repositories
      2. fast-forwards each primary checkout to its upstream branch when the
         checkout has no local commits of its own
      3. refuses to touch a checkout that has local commits or modifications to
         tracked files instead of discarding work; untracked files are preserved
         and reported but do not block a fast-forward
      4. optionally rebuilds nemisis_game in the CLion build directory

    Nothing is deleted. Local commits and modified tracked files abort the sync
    for that repository and are reported instead.

.PARAMETER BuildDirectory
    CMake build directory to refresh. Defaults to the CLion directory
    <workspace>/Nemisis/cmake-build-debug.

.PARAMETER SkipBuild
    Only update the sources; do not run CMake or build.

.PARAMETER Configure
    Re-run CMake configure before building. Required after CMakeLists changes.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\sync_workspace.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\sync_workspace.ps1 -Configure
#>
[CmdletBinding()]
param(
    [string]$BuildDirectory,
    [switch]$SkipBuild,
    [switch]$Configure
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$workspaceRoot = Split-Path -Parent $repositoryRoot
$workspaceRoot = (Resolve-Path $workspaceRoot).Path

$repositories = @(
    @{ Name = 'Nemisis';          Remote = 'origin'; Branch = 'main' },
    @{ Name = 'Novacore-Engine'; Remote = 'origin'; Branch = 'main' }
)

function Write-Step {
    param([string]$Message)
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Write-Note {
    param([string]$Message)
    Write-Host "    $Message"
}

function Invoke-Git {
    param(
        [string]$RepositoryPath,
        [Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments
    )

    $output = & git -C $RepositoryPath @Arguments 2>&1
    return [PSCustomObject]@{
        ExitCode = $LASTEXITCODE
        Output   = ($output | Out-String).Trim()
    }
}

function Sync-Repository {
    param(
        [string]$Name,
        [string]$Remote,
        [string]$Branch
    )

    $repositoryPath = Join-Path $workspaceRoot $Name
    if (-not (Test-Path (Join-Path $repositoryPath '.git'))) {
        Write-Step "$Name"
        Write-Note "not a git checkout, skipped"
        return $false
    }

    Write-Step "$Name"

    $fetch = Invoke-Git -RepositoryPath $repositoryPath -Arguments @('fetch', '--all', '--prune', '--tags')
    if ($fetch.ExitCode -ne 0) {
        Write-Note "fetch failed: $($fetch.Output)"
        return $false
    }

    $dirty = Invoke-Git -RepositoryPath $repositoryPath -Arguments @('status', '--porcelain', '--untracked-files=no')
    if ($dirty.Output) {
        Write-Note "local changes to tracked files present, leaving the checkout untouched:"
        $dirty.Output -split "`r?`n" | Where-Object { $_ } | ForEach-Object { Write-Note "  $_" }
        return $false
    }

    $untracked = Invoke-Git -RepositoryPath $repositoryPath -Arguments @('ls-files', '--others', '--exclude-standard')
    if ($untracked.Output) {
        $untrackedCount = ($untracked.Output -split "`r?`n" | Where-Object { $_ }).Count
        Write-Note "$untrackedCount untracked path(s) preserved (fast-forward leaves them untouched)"
    }

    $upstream = "$Remote/$Branch"
    $ahead = Invoke-Git -RepositoryPath $repositoryPath -Arguments @('rev-list', '--count', "$upstream..HEAD")
    if ([int]$ahead.Output -gt 0) {
        Write-Note "local commits exist ($($ahead.Output) ahead of $upstream), leaving the checkout untouched"
        return $false
    }

    $before = (Invoke-Git -RepositoryPath $repositoryPath -Arguments @('rev-parse', '--short', 'HEAD')).Output
    $merge = Invoke-Git -RepositoryPath $repositoryPath -Arguments @('merge', '--ff-only', $upstream)
    if ($merge.ExitCode -ne 0) {
        Write-Note "fast-forward failed: $($merge.Output)"
        return $false
    }

    $after = (Invoke-Git -RepositoryPath $repositoryPath -Arguments @('rev-parse', '--short', 'HEAD')).Output
    if ($before -eq $after) {
        Write-Note "already current at $after"
    }
    else {
        Write-Note "updated $before -> $after"
    }

    return $true
}

function Show-RemainingWork {
    param([string]$Name)

    $repositoryPath = Join-Path $workspaceRoot $Name
    $branches = Invoke-Git -RepositoryPath $repositoryPath -Arguments @(
        'for-each-ref', '--format=%(refname:short)', 'refs/heads'
    )
    if ($branches.ExitCode -ne 0) { return }

    foreach ($branch in ($branches.Output -split "`r?`n" | Where-Object { $_ })) {
        if ($branch -eq 'main') { continue }
        $ahead = Invoke-Git -RepositoryPath $repositoryPath -Arguments @(
            'rev-list', '--count', "origin/main..$branch"
        )
        if ([int]$ahead.Output -gt 0) {
            Write-Note "$Name branch $branch has $($ahead.Output) commit(s) not in main"
        }
    }
}

$synced = @()
foreach ($repository in $repositories) {
    if (Sync-Repository -Name $repository.Name -Remote $repository.Remote -Branch $repository.Branch) {
        $synced += $repository.Name
    }
}

foreach ($repository in $repositories) {
    Show-RemainingWork -Name $repository.Name
}

if ($SkipBuild) {
    Write-Step "build skipped"
    exit 0
}

if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $workspaceRoot 'Nemisis\cmake-build-debug'
}

$cmake = $null
$configured = Join-Path $BuildDirectory 'CMakeCache.txt'
if (Test-Path $configured) {
    $cache = Get-Content $configured
    $cmakeLine = $cache | Where-Object { $_ -match '^CMAKE_COMMAND:INTERNAL=' } | Select-Object -First 1
    if ($cmakeLine) {
        $candidate = $cmakeLine.Substring('CMAKE_COMMAND:INTERNAL='.Length)
        if (Test-Path $candidate) { $cmake = $candidate }
    }
}

if (-not $cmake) {
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCommand) { $cmake = $cmakeCommand.Source }
}

if (-not $cmake) {
    Write-Step "CMake not found"
    Write-Note "open the project in CLion once, or pass an existing -BuildDirectory"
    exit 1
}

Write-Step "CMake: $cmake"
Write-Step "build directory: $BuildDirectory"

if ($Configure -or -not (Test-Path $configured)) {
    $configureResult = & $cmake -S (Join-Path $workspaceRoot 'Nemisis') -B $BuildDirectory 2>&1
    $configureResult | Select-Object -Last 12 | ForEach-Object { Write-Note $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-Step "configure failed"
        exit $LASTEXITCODE
    }
}

$buildResult = & $cmake --build $BuildDirectory --target nemisis_game 2>&1
$buildResult | Select-Object -Last 12 | ForEach-Object { Write-Note $_ }
if ($LASTEXITCODE -ne 0) {
    Write-Step "build failed"
    exit $LASTEXITCODE
}

$executable = Join-Path $BuildDirectory 'nemisis_game.exe'
if (Test-Path $executable) {
    $stamp = (Get-Item $executable).LastWriteTime
    Write-Step "nemisis_game.exe rebuilt at $stamp"
}
else {
    Write-Step "build finished but nemisis_game.exe was not found"
    exit 1
}

Write-Step "done"
