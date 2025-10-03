$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
  throw "Docker is not installed or not on PATH. Install Docker Desktop and retry."
}

$image = 'yapkernel-build:latest'
$exists = (& docker images -q $image).Trim()
if (-not $exists) {
  & docker build -t $image $root
}

& docker run --rm -v "$root:/work" -w /work $image bash -lc "\
  set -e; \
  make CC='gcc -m32' LD='ld -m elf_i386' iso\
"
