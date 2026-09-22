$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$trainerExe = Join-Path $projectRoot 'build/train_expert.exe'
$runDir = Join-Path $projectRoot 'training/expert100k'
New-Item -ItemType Directory -Force -Path $runDir | Out-Null
if (Get-Process train_expert -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $trainerExe }) {
    throw 'Un entrainement est deja en cours.'
}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$process = Start-Process -FilePath $trainerExe -WorkingDirectory $projectRoot -ArgumentList @('training/expert100k/latest.policy','100000') -WindowStyle Hidden -RedirectStandardOutput (Join-Path $runDir "$stamp.log") -RedirectStandardError (Join-Path $runDir "$stamp.errors.log") -PassThru
$process.PriorityClass = 'BelowNormal'
$process.Id | Set-Content (Join-Path $runDir 'process.pid')
Write-Output "Entrainement demarre : PID $($process.Id), journal $runDir/$stamp.log"
