# Open Developer PowerShell for Visual Studio with the C++ workload available.
$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$Compiler = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $Compiler) { throw 'cl.exe not found. Use Developer PowerShell with the C++ toolchain.' }
$Out = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
New-Item -ItemType Directory $Out | Out-Null
try {
    Push-Location $Out
    try {
        $Include = Join-Path $Root 'Plugins/CoastalFoundation/Source/CoastalFoundation/Public'
        foreach ($Source in Get-ChildItem (Join-Path $Root 'tests') -Filter '*_tests.cpp') {
            $Exe = Join-Path $Out ($Source.BaseName + '.exe')
            & $Compiler.Source /nologo /std:c++17 /EHsc /W4 /WX "/I$Include" $Source.FullName "/Fe:$Exe"
            if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $($Source.Name)" }
            & $Exe
            if ($LASTEXITCODE -ne 0) { throw "Test failure: $($Source.Name)" }
        }
    } finally { Pop-Location }
} finally {
    $ResolvedOut = [System.IO.Path]::GetFullPath($Out)
    $TempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd('\', '/')
    if (-not $ResolvedOut.StartsWith($TempRoot + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing cleanup outside the temporary directory.' }
    Remove-Item -LiteralPath $ResolvedOut -Recurse -Force -ErrorAction SilentlyContinue
}
