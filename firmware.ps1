param(
    [ValidateSet('ajuda', 'preparar', 'compilar', 'usb', 'ota', 'portas')]
    [string]$Acao = 'ajuda',
    [string]$Porta,
    [string]$Cli = $env:ARDUINO_CLI,
    [ValidateRange(1, 32)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$runtimeRoot = Join-Path $projectRoot '.arduino'
$buildPath = Join-Path $projectRoot 'build-cli'
$outputPath = Join-Path $projectRoot 'output'
$fqbn = 'esp32:esp32:XIAO_ESP32C6:PartitionScheme=default,FlashSize=4M,CDCOnBoot=cdc,CPUFreq=160'
$maximumSize = 1966080

if ($Acao -eq 'ajuda') {
    Write-Output @'
Uso: .\firmware.cmd ACAO [-Porta COM5 ou IP] [-Cli caminho\arduino-cli.exe]
  preparar  Instala ESP32 3.3.12 e bibliotecas em .arduino dentro do projeto.
  compilar  Gera firmware e particoes OTA de 1.875 MiB x2, sem usar a IDE.
  usb       Compila e grava firmware, bootloader e particoes pela porta COM.
  ota       Compila e grava pela rede, sem senha (exige primeira gravacao USB).
  portas    Lista portas disponiveis.
Instale Arduino CLI separadamente e adicione ao PATH, coloque em .tools,
ou informe -Cli / variavel ARDUINO_CLI. Nenhuma acao usa a configuracao da IDE.
'@
    exit 0
}

if (!$Cli) {
    $localCli = Join-Path $projectRoot '.tools\arduino-cli.exe'
    $foundCli = Get-Command arduino-cli.exe -ErrorAction SilentlyContinue
    if (Test-Path -LiteralPath $localCli) { $Cli = $localCli }
    elseif ($foundCli) { $Cli = $foundCli.Source }
    else { throw 'Arduino CLI nao encontrado. Instale o executavel em .tools\arduino-cli.exe ou informe -Cli.' }
}
if (($Acao -eq 'usb' -or $Acao -eq 'ota') -and !$Porta) {
    throw 'Informe -Porta COM5 para USB ou -Porta 192.168.1.50 para OTA.'
}
if ($Acao -eq 'usb' -and $Porta -notmatch '^COM[0-9]+$') { throw 'USB requer porta COM, por exemplo COM5.' }
if ($Acao -eq 'ota' -and $Porta -notmatch '^[a-zA-Z0-9][a-zA-Z0-9.:-]*$') { throw 'OTA requer IP ou nome de rede, sem protocolo http.' }

New-Item -ItemType Directory -Path $runtimeRoot -Force | Out-Null
$configPath = Join-Path $runtimeRoot 'arduino-cli.yaml'
function YamlPath([string]$path) { return "'" + $path.Replace('\', '/').Replace("'", "''") + "'" }
$config = @"
board_manager:
  additional_urls:
    - https://espressif.github.io/arduino-esp32/package_esp32_index.json
directories:
  data: $(YamlPath (Join-Path $runtimeRoot 'data'))
  downloads: $(YamlPath (Join-Path $runtimeRoot 'downloads'))
  user: $(YamlPath (Join-Path $runtimeRoot 'user'))
"@
[IO.File]::WriteAllText($configPath, $config, [Text.UTF8Encoding]::new($false))
function Invoke-Arduino([string[]]$Arguments) {
    & $Cli --config-file $configPath @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Arduino CLI terminou com codigo $LASTEXITCODE." }
}

if ($Acao -eq 'preparar') {
    Invoke-Arduino -Arguments @('core', 'update-index')
    Invoke-Arduino -Arguments @('core', 'install', 'esp32:esp32@3.3.12')
    Invoke-Arduino -Arguments @('lib', 'update-index')
    Invoke-Arduino -Arguments @('lib', 'install', 'ArduinoJson@7.4.2', 'TinyGPSPlus@1.0.3', 'Adafruit SSD1306@2.5.9', 'Adafruit GFX Library@1.11.9')
    Write-Output 'Ambiente independente preparado. Execute firmware.cmd compilar ou firmware.cmd usb -Porta COM5.'
    exit 0
}
if ($Acao -eq 'portas') {
    Invoke-Arduino -Arguments @('board', 'list')
    exit 0
}
if (!(Test-Path -LiteralPath (Join-Path $runtimeRoot 'data\packages\esp32\hardware\esp32\3.3.12\boards.txt'))) {
    throw 'Execute firmware.cmd preparar antes de compilar ou gravar.'
}
# A tabela do sketch e copiada pelo hook do core ESP32. O limite de tamanho
# tambem e definido explicitamente, sem depender de boards.local.txt ou menus.
Invoke-Arduino -Arguments @('compile', '--fqbn', $fqbn, '--jobs', "$Jobs",
    '--build-path', $buildPath,
    '--build-property', 'build.partitions=default',
    '--build-property', "upload.maximum_size=$maximumSize", $projectRoot)
$binaryPath = Join-Path $buildPath 'XIAO_ESP32C6.ino.bin'
$bytes = (Get-Item -LiteralPath $binaryPath).Length
if ($bytes -gt $maximumSize) { throw "Firmware com $bytes bytes excede a particao de $maximumSize bytes." }
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
foreach ($name in @('XIAO_ESP32C6.ino.bin', 'XIAO_ESP32C6.ino.bootloader.bin', 'XIAO_ESP32C6.ino.partitions.bin', 'partitions.csv')) {
    Copy-Item -LiteralPath (Join-Path $buildPath $name) -Destination (Join-Path $outputPath $name) -Force
}
Write-Output "Firmware: $bytes / $maximumSize bytes. Arquivos gerados em $outputPath."
if ($Acao -eq 'usb') {
    Invoke-Arduino -Arguments @('upload', '--fqbn', $fqbn, '--port', $Porta,
        '--protocol', 'serial', '--input-dir', $buildPath)
} elseif ($Acao -eq 'ota') {
    Invoke-Arduino -Arguments @('upload', '--fqbn', $fqbn, '--port', $Porta,
        '--protocol', 'network', '--upload-field', 'password=',
        '--upload-property', 'upload.port.properties.port=3232', '--input-dir', $buildPath)
}
