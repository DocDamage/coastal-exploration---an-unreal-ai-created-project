$ErrorActionPreference = 'Stop'
$trialPathsJson = & python "$PSScriptRoot/ue58_paths.py"
if ($LASTEXITCODE -ne 0) { throw 'Unable to resolve the independent UE5.8 installation.' }
$trialPaths = $trialPathsJson | ConvertFrom-Json
$trialProject = $trialPaths.project
$trialEvidence = $trialPaths.evidence
$copyStatus = Get-Content -LiteralPath "$trialEvidence/m3-ue58-trial-copy.json" -Raw | ConvertFrom-Json
$buildStatus = Get-Content -LiteralPath "$trialEvidence/m3-ue58-editor-build.execution.json" -Raw | ConvertFrom-Json
if (-not $copyStatus.content_copy_complete -or $buildStatus.exit_code -ne 0) {
    throw 'The isolated copy and native build must complete before launching.'
}
$trialExistingEditors = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe'"
if ($trialExistingEditors | Where-Object {
    # Launcher may omit quotes for paths without spaces.
    $trialNormalizedCommand = $_.CommandLine.Replace('\', '/')
    $trialNormalizedProject = $trialProject.Replace('\', '/')
    $trialNormalizedCommand.IndexOf($trialNormalizedProject, [System.StringComparison]::OrdinalIgnoreCase) -ge 0
}) {
    throw 'The independent trial is already open in an editor/test process.'
}
if (Get-NetTCPConnection -LocalPort 30020 -State Listen -ErrorAction SilentlyContinue) {
    throw 'MCP port 30020 is already occupied; verify its project before launching another listener.'
}
# A Launcher-started project chooser has no project argument and does not own
# this project's listener. Leave that user-owned process alone.
# These settings apply only to this process and its child editor.
$env:TEMP = "$($trialPaths.workspace)/Cache/UE58/Temp"
$env:TMP = $env:TEMP
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', "$($trialPaths.workspace)/Cache/UE58/DDC", 'Process')
New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
$arguments = @(('"{0}"' -f $trialProject), '-DDC=InstalledNoZenLocalFallback')
Start-Process -FilePath "$($trialPaths.engine)/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList $arguments -WindowStyle Hidden -PassThru |
    Select-Object Id,ProcessName
