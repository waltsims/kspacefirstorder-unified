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
& "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics

Write-Host "Installing manifest dependencies (triplet autodetected by vcpkg)"
& "$VcpkgRoot\vcpkg.exe" install

Write-Host "Done. Set VCPKG_ROOT=$VcpkgRoot when configuring CMake if needed."

