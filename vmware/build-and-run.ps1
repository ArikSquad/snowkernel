param(
  [switch]$Build
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$proj = Split-Path -Parent $root

Push-Location $proj
try {
  if ($Build) {
    if (Get-Command make -ErrorAction SilentlyContinue) {
      make iso
    } else {
      Write-Error "GNU make not found. Build the ISO manually or run in WSL: 'make iso'"
    }
  }
  $iso = Join-Path $proj 'yapkernel.iso'
  if (!(Test-Path $iso)) { throw "ISO not found: $iso. Build it first (make iso)." }

  $vmx = Join-Path $root 'yapkernel.vmx'

  $vmrunCandidates = @(
    "$Env:ProgramFiles(x86)\VMware\VMware Workstation\vmrun.exe",
    "$Env:ProgramFiles\VMware\VMware Workstation\vmrun.exe",
    "$Env:ProgramFiles(x86)\VMware\VMware Player\vmrun.exe",
    "$Env:ProgramFiles\VMware\VMware Player\vmrun.exe"
  )
  $vmrun = $vmrunCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $vmrun) { throw "vmrun.exe not found. Install VMware Workstation/Player or add vmrun to PATH." }

  try {
    & $vmrun -T ws start $vmx nogui
  } catch {
    & $vmrun -T player start $vmx nogui
  }
}
finally {
  Pop-Location
}
