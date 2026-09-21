param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ProjectFile
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath $ProjectFile).Path
$engine = (Resolve-Path -LiteralPath $EngineRoot).Path
$output = Join-Path (Split-Path $project) 'Saved/RuntimeDiagnostics'
New-Item -ItemType Directory -Force -Path $output | Out-Null
& (Join-Path $engine 'Engine/Build/BatchFiles/Build.bat') UnrealEditor Win64 Development "-Project=$project" '-Module=RuntimeDiagnostics' '-Module=RuntimeDiagnosticsMCP' -NoHotReloadFromIDE -NoXGE "-log=$output/Build.log"
if ($LASTEXITCODE -ne 0) { throw "RuntimeDiagnostics build failed: $LASTEXITCODE" }
# UBT's module-only build suppresses target metadata. Publish a manifest for the
# two DLLs just compiled against this exact editor, without rebuilding the engine.
$editorManifest = Get-Content -LiteralPath (Join-Path $engine 'Engine/Binaries/Win64/UnrealEditor.modules') -Raw | ConvertFrom-Json
$bin = Join-Path $PSScriptRoot 'Binaries/Win64'
foreach ($module in @('RuntimeDiagnostics', 'RuntimeDiagnosticsMCP')) {
    if (!(Test-Path -LiteralPath (Join-Path $bin "UnrealEditor-$module.dll"))) { throw "Missing compiled module $module" }
}
$manifest = [ordered]@{
    BuildId = $editorManifest.BuildId
    Modules = [ordered]@{
        RuntimeDiagnostics = 'UnrealEditor-RuntimeDiagnostics.dll'
        RuntimeDiagnosticsMCP = 'UnrealEditor-RuntimeDiagnosticsMCP.dll'
    }
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $bin 'UnrealEditor.modules') -Encoding UTF8
