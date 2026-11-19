param([Parameter(Mandatory=$true)][string]$TargetArch)
$ErrorActionPreference = 'Stop'
$compilerSetup = Join-Path $env:RUNNER_TEMP 'inno-6.4.3.exe'
Invoke-WebRequest 'https://github.com/jrsoftware/issrc/releases/download/is-6_4_3/innosetup-6.4.3.exe' -OutFile $compilerSetup
if ((Get-FileHash $compilerSetup -Algorithm SHA256).Hash -ne 'f3c42116542c4cc57263c5ba6c4feabfc49fe771f2f98a79d2f7628b8762723b') { throw 'Unexpected Inno Setup compiler digest' }
$compilerDir = Join-Path $env:RUNNER_TEMP 'inno-compiler'
$flags = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/CURRENTUSER',('/DIR="{0}"' -f $compilerDir))
$installed = Start-Process $compilerSetup -ArgumentList $flags -Wait -PassThru
if ($installed.ExitCode) { throw "Compiler setup failed $($installed.ExitCode)" }
& (Join-Path $compilerDir 'ISCC.exe') "/DTargetArch=$TargetArch" (Join-Path $PSScriptRoot 'Rocker2.iss')
if ($LASTEXITCODE) { throw 'ISCC failed' }
$setup = (Resolve-Path "installers/Rocker2-Setup-$TargetArch.exe").Path
$installDir = Join-Path $env:RUNNER_TEMP "instalace-řůě-$TargetArch"
$markerDir = Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Rocker 2/Skladby/Instalační zkouška'
New-Item -ItemType Directory -Path $markerDir -Force | Out-Null
$marker = Join-Path $markerDir 'zachovat.txt'
Set-Content -LiteralPath $marker -Value 'Zachovat skladbu'
$flags = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/DIR="{0}"' -f $installDir))
$result = Start-Process $setup -ArgumentList $flags -Wait -PassThru
if ($result.ExitCode) { throw 'App installation failed' }
$exe = Join-Path $installDir 'Rocker 2.exe'
if ((Get-FileHash -LiteralPath $exe).Hash -ne (Get-FileHash -LiteralPath 'pack/Rocker 2.exe').Hash) { throw 'Installed executable differs from the native build' }
$app = Start-Process $exe -PassThru
try {
    $deadline = (Get-Date).AddSeconds(40)
    do {
        Start-Sleep -Milliseconds 300
        $app.Refresh()
        if ($app.HasExited) { throw 'Installed application exited during startup' }
    } until ($app.MainWindowHandle -ne 0 -or (Get-Date) -ge $deadline)
    if ($app.MainWindowHandle -eq 0) { throw 'Installed application did not open a window' }
} finally {
    if (!$app.HasExited) {
        [void]$app.CloseMainWindow()
        if (!$app.WaitForExit(10000)) { $app.Kill(); $app.WaitForExit() }
    }
}
$result = Start-Process (Join-Path $installDir 'unins000.exe') -ArgumentList '/VERYSILENT','/NORESTART','/SUPPRESSMSGBOXES' -Wait -PassThru
if ($result.ExitCode -or (Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $marker)) { throw 'Uninstall or song preservation failed' }
