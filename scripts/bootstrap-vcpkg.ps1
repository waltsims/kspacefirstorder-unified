Param(
  [string]$VcpkgRoot
)

$ErrorActionPreference = 'Stop'

if (-not $VcpkgRoot) {
  $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
  $repoRoot = Resolve-Path (Join-Path $scriptDir '..')
  $VcpkgRoot = Join-Path $repoRoot 'third-party\vcpkg'
}

Write-Host "Bootstrapping vcpkg at: $VcpkgRoot"
Write-Host "Bootstrapping vcpkg at: $VcpkgRoot"
& "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
if ($LASTEXITCODE -ne 0) {
  throw "vcpkg bootstrap failed with exit code $LASTEXITCODE"
}

Push-Location $repoRoot
try {
  Write-Host "Installing manifest dependencies (triplet autodetected by vcpkg)"
  & "$VcpkgRoot\vcpkg.exe" install
  if ($LASTEXITCODE -ne 0) {
    throw "vcpkg install failed with exit code $LASTEXITCODE"
  }
} finally {
  Pop-Location
}

Write-Host "Done. Set VCPKG_ROOT=$VcpkgRoot when configuring CMake if needed."

