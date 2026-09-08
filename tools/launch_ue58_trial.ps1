$ErrorActionPreference = 'Stop'
$trialProject = 'F:/coastline/LocalHost58/CoastalExploration/CoastalExploration.uproject'
$trialEvidence = 'F:/coastline/local-evidence'
$copyStatus = Get-Content -LiteralPath "$trialEvidence/m3-ue58-trial-copy.json" -Raw | ConvertFrom-Json
$buildStatus = Get-Content -LiteralPath "$trialEvidence/m3-ue58-editor-build.execution.json" -Raw | ConvertFrom-Json
if (-not $copyStatus.content_copy_complete -or $buildStatus.exit_code -ne 0) {
    throw 'The isolated copy and native build must complete before launching.'
}
$trialExistingEditors = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe'"
if ($trialExistingEditors | Where-Object { $_.Name -eq 'UnrealEditor-Cmd.exe' -or $_.CommandLine -match '\.uproject' }) {
    throw 'Close the current project editor/test process before starting the trial.'
}
# A Launcher-started project chooser has no project argument and does not own
# this project's listener. Leave that user-owned process alone.
# These settings apply only to this process and its child editor.
$env:TEMP = 'F:/coastline/Cache/UE58/Temp'
$env:TMP = $env:TEMP
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', 'F:/coastline/Cache/UE58/DDC', 'Process')
$arguments = @(('"{0}"' -f $trialProject), '-DDC=InstalledNoZenLocalFallback')
Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru |
    Select-Object Id,ProcessName
