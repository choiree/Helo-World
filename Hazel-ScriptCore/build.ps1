# Hazel-ScriptCore Build Script
# Requires Visual Studio 2022 with .NET development tools

$sourceDir = "Source"
$outputDir = "../Hazelnut/Resources/Scripts"
$outputFile = "$outputDir/Hazel-ScriptCore.dll"

# Create output directory if it doesn't exist
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# Get all C# source files
$sourceFiles = Get-ChildItem -Path $sourceDir -Recurse -Filter "*.cs"

Write-Host "Compiling $($sourceFiles.Count) source files..."

# Use Visual Studio 2022's Roslyn compiler (supports C# 6+)
$cscPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\Roslyn\csc.exe"

if (-not (Test-Path $cscPath)) {
    Write-Error "Roslyn compiler not found at: $cscPath"
    Write-Error "Please install Visual Studio 2022 with .NET development tools."
    exit 1
}

$arguments = @(
    "/target:library",
    "/out:`"$outputFile`"",
    "/platform:x64",
    "/optimize-",
    "/debug",
    "/langversion:latest"
) + $sourceFiles.FullName

Write-Host "Running: $cscPath"

$process = Start-Process -FilePath $cscPath -ArgumentList $arguments -Wait -NoNewWindow -PassThru

if ($process.ExitCode -eq 0) {
    Write-Host "`nBuild successful! Output: $outputFile"
} else {
    Write-Error "Build failed with exit code $($process.ExitCode)"
    exit $process.ExitCode
}
