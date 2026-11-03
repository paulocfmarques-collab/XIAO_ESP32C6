param(
    [string]$Esp32Package = 'C:\Users\paulo\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.12'
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath (Join-Path $Esp32Package 'boards.txt'))) {
    throw 'Pacote ESP32 nao encontrado. Informe -Esp32Package com a pasta da versao instalada.'
}
$partitionTarget = Join-Path $Esp32Package 'tools\partitions\xiao_gps_ota.csv'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'partitions.csv') -Destination $partitionTarget -Force
$boardsLocal = Join-Path $Esp32Package 'boards.local.txt'
$content = if (Test-Path -LiteralPath $boardsLocal) { [IO.File]::ReadAllText($boardsLocal) } else { '' }
$prefix = 'XIAO_ESP32C6.menu.PartitionScheme.xiao_gps_ota'
$lines = $content -split '\r?\n' | Where-Object { !$_.StartsWith($prefix + '=') -and !$_.StartsWith($prefix + '.') }
$entry = @'
XIAO_ESP32C6.menu.PartitionScheme.xiao_gps_ota=GPS OTA 4MB (1.875MB x2 / 128KB SPIFFS)
XIAO_ESP32C6.menu.PartitionScheme.xiao_gps_ota.build.partitions=xiao_gps_ota
XIAO_ESP32C6.menu.PartitionScheme.xiao_gps_ota.upload.maximum_size=1966080
'@
[IO.File]::WriteAllText($boardsLocal, (($lines -join "`r`n").TrimEnd() + "`r`n" + $entry + "`r`n"), [Text.UTF8Encoding]::new($false))
$boardsPath = Join-Path $Esp32Package 'boards.txt'
$boardsText = [IO.File]::ReadAllText($boardsPath)
$backupPath = Join-Path $Esp32Package 'boards.txt.before-xiao-gps-ota.bak'
if (!(Test-Path -LiteralPath $backupPath)) {
    Copy-Item -LiteralPath $boardsPath -Destination $backupPath
}
$boardsText = [regex]::Replace($boardsText,
    '(?m)^XIAO_ESP32C6\.menu\.PartitionScheme\.default=[^\r\n]*',
    'XIAO_ESP32C6.menu.PartitionScheme.default=GPS OTA 4MB (1.875MB x2 / 128KB SPIFFS)')
$boardsText = [regex]::Replace($boardsText,
    '(?m)^XIAO_ESP32C6\.menu\.PartitionScheme\.default\.build\.partitions=[^\r\n]*',
    'XIAO_ESP32C6.menu.PartitionScheme.default.build.partitions=xiao_gps_ota')
$sizeKey = 'XIAO_ESP32C6.menu.PartitionScheme.default.upload.maximum_size'
if ($boardsText -match '(?m)^XIAO_ESP32C6\.menu\.PartitionScheme\.default\.upload\.maximum_size=') {
    $boardsText = [regex]::Replace($boardsText,
        '(?m)^XIAO_ESP32C6\.menu\.PartitionScheme\.default\.upload\.maximum_size=[^\r\n]*',
        "$sizeKey=1966080")
} else {
    $boardsText += "`r`n$sizeKey=1966080`r`n"
}
[IO.File]::WriteAllText($boardsPath, $boardsText, [Text.UTF8Encoding]::new($false))
Write-Output 'Padrao XIAO atualizado: GPS OTA, limite 1966080 bytes. Feche todas as janelas da IDE e reabra. Grave por USB uma vez.'
