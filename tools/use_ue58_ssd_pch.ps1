$ErrorActionPreference = 'Stop'
$trialPathsJson = & python "$PSScriptRoot/ue58_paths.py"
if ($LASTEXITCODE -ne 0) { throw 'Unable to resolve the independent trial' }
$trialPaths = $trialPathsJson | ConvertFrom-Json
$trialRoot = [IO.Path]::GetDirectoryName($trialPaths.project)
$pchFolder = [IO.Path]::GetFullPath("$trialRoot/Intermediate/Build/Win64/x64/TP_ThirdPersonEditor/Development/UnrealEd")
$preservedFolder = "$pchFolder.hdd-preserved-0909"
$audioCacheRoot = [IO.Path]::GetFullPath((Join-Path $env:LOCALAPPDATA 'CoastalBuildCache'))
$ssdFolder = [IO.Path]::GetFullPath((Join-Path $audioCacheRoot 'UE5.8.2/UnrealEd'))
if (-not $pchFolder.StartsWith($trialRoot + [IO.Path]::DirectorySeparatorChar) -or
    -not $preservedFolder.StartsWith($trialRoot + [IO.Path]::DirectorySeparatorChar) -or
    -not $ssdFolder.StartsWith($audioCacheRoot + [IO.Path]::DirectorySeparatorChar)) {
    throw 'Unexpected cache paths'
}
if (Get-Process cl -ErrorAction SilentlyContinue) { throw 'Finish or stop compilation before moving its cache' }
if ((Get-Item -LiteralPath $pchFolder).LinkType) { throw 'Cache is already redirected; inspect before rerunning' }
if ((Test-Path -LiteralPath $preservedFolder) -or (Test-Path -LiteralPath $ssdFolder)) {
    throw 'Preserved/cache destination already exists; inspect before rerunning'
}
$cacheFiles = Get-ChildItem -LiteralPath $pchFolder -File
if ((Get-PSDrive C).Free -lt 8GB) { throw 'Insufficient free SSD space' }
New-Item -ItemType Directory -Path $ssdFolder | Out-Null
# Copy each verified file explicitly.
foreach ($cacheFile in $cacheFiles) {
    Copy-Item -LiteralPath $cacheFile.FullName -Destination (Join-Path $ssdFolder $cacheFile.Name)
}
foreach ($cacheFile in $cacheFiles) {
    if ((Get-FileHash -LiteralPath (Join-Path $ssdFolder $cacheFile.Name)).Hash -ne (Get-FileHash -LiteralPath $cacheFile.FullName).Hash) {
        throw 'Cache copy SHA256 verification failed'
    }
}
Move-Item -LiteralPath $pchFolder -Destination $preservedFolder
try { New-Item -ItemType Junction -Path $pchFolder -Target $ssdFolder | Out-Null }
catch { Move-Item -LiteralPath $preservedFolder -Destination $pchFolder; throw }
@{ source = $pchFolder; target = $ssdFolder; preserved = $preservedFolder; files = $cacheFiles.Count;
   scope = 'Only generated UE5.8 trial PCH cache; no original host or content changes' } |
    ConvertTo-Json | Set-Content -LiteralPath "$($trialPaths.evidence)/m3-ue58-ssd-pch-0909.json"
Get-Item -LiteralPath $pchFolder | Select-Object FullName,LinkType,Target
