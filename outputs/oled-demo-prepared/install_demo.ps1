$ErrorActionPreference = 'Stop'
$demoStage = 'D:\stm32halproject\stm32f407vgt6\uart\outputs\oled-demo-prepared\iic_demo'
$demoTarget = 'D:\gk_rm_demo\iic_demo'
$manifestPath = Join-Path $PSScriptRoot 'install_manifest.json'
$entries = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$resolvedTarget = [IO.Path]::GetFullPath($demoTarget).TrimEnd('\')
if ($resolvedTarget -ne 'D:\gk_rm_demo\iic_demo') { throw 'Unexpected target directory' }

# All original and prepared files must still match the reviewed manifest.
# No files are deleted, and no directories outside the named project are written.
foreach ($entry in $entries) {
    $sourcePath = [IO.Path]::GetFullPath((Join-Path $demoStage $entry.path))
    $targetPath = [IO.Path]::GetFullPath((Join-Path $demoTarget $entry.path))
    if (-not $sourcePath.StartsWith($demoStage + '\', [StringComparison]::OrdinalIgnoreCase) -or
        -not $targetPath.StartsWith($resolvedTarget + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path outside project: $($entry.path)"
    }
    if ((Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.prepared_sha256) {
        throw "Prepared file changed after review: $($entry.path)"
    }
    if ($null -eq $entry.original_sha256) {
        if (Test-Path -LiteralPath $targetPath) { throw "New target file now exists: $($entry.path)" }
    } else {
        if (-not (Test-Path -LiteralPath $targetPath) -or
            (Get-FileHash -LiteralPath $targetPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.original_sha256) {
            throw "Original target changed since preparation: $($entry.path)"
        }
    }
}

$backupRoot = Join-Path $demoTarget ('Backups\oled-framework-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
foreach ($entry in $entries) {
    $sourcePath = Join-Path $demoStage $entry.path
    $targetPath = Join-Path $demoTarget $entry.path
    if ($null -ne $entry.original_sha256) {
        $backupPath = Join-Path $backupRoot $entry.path
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($backupPath)) | Out-Null
        Copy-Item -LiteralPath $targetPath -Destination $backupPath
    }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($targetPath)) | Out-Null
    Copy-Item -LiteralPath $sourcePath -Destination $targetPath -Force
}

foreach ($entry in $entries) {
    $targetPath = Join-Path $demoTarget $entry.path
    if ((Get-FileHash -LiteralPath $targetPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.prepared_sha256) {
        throw "Installed file verification failed: $($entry.path)"
    }
}
Write-Output "Installed $($entries.Count) files. Original files backed up at $backupRoot"

# Verify a fresh build in the requested directory, with no copied CMake cache.
$toolRoot = 'C:\Users\30710\AppData\Local\stm32cube\bundles'
$env:PATH = "$toolRoot\gnu-tools-for-stm32\14.3.1+st.2\bin;$toolRoot\ninja\1.13.1+st.1\bin;$toolRoot\cmake\4.0.1+st.3\bin;" + $env:PATH
Push-Location -LiteralPath $demoTarget
try {
    & cmake --preset Debug
    if ($LASTEXITCODE -ne 0) { throw 'Target CMake configure failed' }
    & cmake --build --preset Debug
    if ($LASTEXITCODE -ne 0) { throw 'Target build failed' }
} finally {
    Pop-Location
}
