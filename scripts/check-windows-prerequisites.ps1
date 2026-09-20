$ErrorActionPreference = "Stop"
$commands = @("git", "cmake")
$missing = @()
foreach ($command in $commands) {
  if (-not (Get-Command $command -ErrorAction SilentlyContinue)) { $missing += $command }
}
if ($missing.Count -gt 0) {
  Write-Error ("Missing command(s): " + ($missing -join ", "))
}
Write-Host "Git:   $(& git --version)"
Write-Host "CMake: $(& cmake --version | Select-Object -First 1)"
if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
  Write-Host "MSVC compiler is available."
} else {
  Write-Warning "cl.exe is not on PATH. Use Developer PowerShell for Visual Studio 2022."
}
Write-Host "Bravelike prerequisites check completed."
