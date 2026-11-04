param(
    [ValidateSet('ajuda', 'preparar', 'compilar', 'usb', 'ota', 'portas')]
    [string]$Acao = 'ajuda',
    [string]$Porta,
    [string]$Pio = $env:PLATFORMIO_EXE,
    [ValidateRange(1, 32)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$outputPath = Join-Path $projectRoot 'output'
$environment = 'seeed_xiao_esp32c6'
if ($Acao -eq 'ota') { $environment = 'ota' }
$buildPath = Join-Path $projectRoot ".pio\build\$environment"
$maximumSize = 1966080

if ($Acao -eq 'ajuda') {
    Write-Output @'
Uso: .\firmware.cmd ACAO [-Porta COM5 ou IP] [-Pio caminho\platformio.exe] [-Jobs 4]
  preparar  Instala a plataforma e as bibliotecas definidas em platformio.ini.
  compilar  Mostra os comandos do compilador e gera o firmware OTA.
  usb       Compila e grava firmware, bootloader e particoes pela porta COM.
  ota       Compila e grava pela rede, sem senha (exige primeira gravacao USB).
  portas    Lista portas disponiveis.
Use PlatformIO no PATH, o Core instalado pela extensao VS Code, -Pio
ou a variavel PLATFORMIO_EXE. Nenhuma acao usa Arduino CLI ou a IDE.
'@
    exit 0
}

if (!$Pio) {
    $foundPio = Get-Command platformio.exe, pio.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    $localPio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe'
    if ($foundPio) { $Pio = $foundPio.Source }
    elseif (Test-Path -LiteralPath $localPio) { $Pio = $localPio }
    else { throw 'PlatformIO nao encontrado. Instale a extensao PlatformIO IDE ou informe -Pio.' }
}
if (($Acao -eq 'usb' -or $Acao -eq 'ota') -and !$Porta) {
    throw 'Informe -Porta COM5 para USB ou -Porta 192.168.1.50 para OTA.'
}
if ($Acao -eq 'usb' -and $Porta -notmatch '^COM[0-9]+$') { throw 'USB requer porta COM, por exemplo COM5.' }
if ($Acao -eq 'ota' -and $Porta -notmatch '^[a-zA-Z0-9][a-zA-Z0-9.:-]*$') { throw 'OTA requer IP ou nome de rede, sem protocolo http.' }

function Invoke-PlatformIO([string]$Etapa, [string[]]$Arguments) {
    Write-Host "`n==> $Etapa"
    & $Pio @Arguments
    if ($LASTEXITCODE -ne 0) { throw "PlatformIO terminou com codigo $LASTEXITCODE." }
    Write-Host "Concluido: $Etapa"
}

if ($Acao -eq 'preparar') {
    Invoke-PlatformIO -Etapa 'Instalando plataforma e bibliotecas' -Arguments @('pkg', 'install', '--project-dir', $projectRoot)
    Write-Output 'Ambiente preparado. Execute firmware.cmd compilar ou firmware.cmd usb -Porta COM5.'
    exit 0
}
if ($Acao -eq 'portas') {
    Invoke-PlatformIO -Etapa 'Procurando placas conectadas' -Arguments @('device', 'list')
    exit 0
}
Write-Host "Projeto: $projectRoot"
Write-Host "Acao: $Acao | Ambiente: $environment | Tarefas paralelas: $Jobs"
Invoke-PlatformIO -Etapa 'Compilando o firmware (saida detalhada)' -Arguments @('run',
    '--project-dir', $projectRoot, '--environment', $environment, '--jobs', "$Jobs", '--verbose')
Write-Host 'Validando tamanho do firmware...'
$binaryPath = Join-Path $buildPath 'firmware.bin'
$bytes = (Get-Item -LiteralPath $binaryPath).Length
if ($bytes -gt $maximumSize) { throw "Firmware com $bytes bytes excede a particao de $maximumSize bytes." }
Write-Host "Tamanho valido: $bytes / $maximumSize bytes."
Write-Host 'Copiando firmware, bootloader e particoes para a pasta output...'
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
foreach ($name in @('firmware.bin', 'bootloader.bin', 'partitions.bin')) {
    Copy-Item -LiteralPath (Join-Path $buildPath $name) -Destination (Join-Path $outputPath $name) -Force
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'partitions.csv') -Destination (Join-Path $outputPath 'partitions.csv') -Force
Write-Output "Firmware: $bytes / $maximumSize bytes. Arquivos gerados em $outputPath."
if ($Acao -eq 'usb' -or $Acao -eq 'ota') {
    Invoke-PlatformIO -Etapa "Gravando firmware ($Acao) em $Porta" -Arguments @('run',
        '--project-dir', $projectRoot, '--environment', $environment, '--target', 'upload',
        '--upload-port', $Porta, '--jobs', "$Jobs")
}
