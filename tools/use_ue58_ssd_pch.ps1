$ErrorActionPreference = 'Stop'
$trialRoot = [IO.Path]::GetFullPath('F:/coastline/LocalHost58/CoastalExploration')
$pchFolder = [IO.Path]::GetFullPath("$trialRoot/Intermediate/Build/Win64/x64/TP_ThirdPersonEditor/Development/UnrealEd")
$preservedFolder = "$pchFolder.hdd-preserved"
$ssdFolder = [IO.Path]::GetFullPath('C:/Users/Doc/AppData/Local/CoastalBuildCache/UE5.8.1/UnrealEd')
if (-not $pchFolder.StartsWith($trialRoot + [IO.Path]::DirectorySeparatorChar) -or
    -not $preservedFolder.StartsWith($trialRoot + [IO.Path]::DirectorySeparatorChar) -or
    -not $ssdFolder.StartsWith('C:\Users\Doc\AppData\Local\CoastalBuildCache\')) {
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
    if ((Get-Item -LiteralPath (Join-Path $ssdFolder $cacheFile.Name)).Length -ne $cacheFile.Length) {
        throw 'Cache copy size verification failed'
    }
}
Move-Item -LiteralPath $pchFolder -Destination $preservedFolder
try { New-Item -ItemType Junction -Path $pchFolder -Target $ssdFolder | Out-Null }
catch { Move-Item -LiteralPath $preservedFolder -Destination $pchFolder; throw }
@{ source = $pchFolder; target = $ssdFolder; preserved = $preservedFolder; files = $cacheFiles.Count;
   scope = 'Only generated UE5.8 trial PCH cache; no original host or content changes' } |
    ConvertTo-Json | Set-Content -LiteralPath 'F:/coastline/local-evidence/m3-ue58-ssd-pch.json'
Get-Item -LiteralPath $pchFolder | Select-Object FullName,LinkType,Target
