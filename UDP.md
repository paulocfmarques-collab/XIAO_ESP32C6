# Comandos UDP

Envie um comando de texto em minúsculas por datagrama para a porta **4210**
da placa. Use o IP mostrado em REDE WI-FI ou `192.168.4.1` quando estiver
conectado ao portal XIAO_ESP32C6_CONFIG. O serviço acompanha as mudanças de IP
e reabre o socket ao recuperar a rede.

A resposta é um objeto JSON enviado ao IP e à porta de origem. Contém `ok`
e `comando`, além dos dados solicitados. Comandos desconhecidos, maiúsculas
e parâmetros extras retornam erro. Espaços nas pontas e quebras de linha
finais são aceitos. O limite é de 63 bytes por datagrama, antes de remover
os espaços. Nomes de comando e chaves JSON estão em minúsculas. Valores como
SSID, cidade e modelo da placa preservam sua grafia.

## Consultas

| Comando | Resposta |
| --- | --- |
| `ajuda` | Lista completa dos comandos aceitos e porta do serviço. |
| `ping` | Responde `pong`. |
| `status` | Resumo de GPS, Wi-Fi, OLED, LED, localidade, previsão, OTA e tempo ligado. |
| `gps` | Posição, satélites, HDOP, velocidade, rumo, altitude, data e hora, com validade. |
| `posicao` | Mesmo conjunto completo de dados GPS. |
| `relogio` | Mesmo conjunto GPS, incluindo hora local, data e fuso. |
| `satelites` | Mesmo conjunto GPS, incluindo satélites e estado do fix. |
| `movimento` | Mesmo conjunto GPS, incluindo velocidade em km/h, rumo em graus e altitude em metros. |
| `precisao` | Mesmo conjunto GPS, incluindo HDOP. |
| `wifi` | Conexão, SSID atual, IP, IP e estado do portal, MAC e RSSI. |
| `redes` | Os cinco slots de SSID cadastrados, sem expor senhas. |
| `localidade` | Resultado armazenado de cidade, estado e país, com disponibilidade e falha. |
| `tempo` | Previsão armazenada de três dias, idade da consulta e condição de dados anteriores. |
| `sistema` | Chip, CPU, tempo ligado, memória livre/mínima, flash e tamanho do firmware. |
| `ota` | Disponibilidade do ArduinoOTA, hostname, porta 3232 e indicação de ausência de senha. |
| `tela` | Disponibilidade do OLED, estado ligado e página selecionada. |
| `led` | Modo e estado atual do LED externo D8. |

Campos GPS inválidos retornam `null`. Latitude e longitude só são publicadas
com fix confiável e dados recebidos. Hora e data podem continuar disponíveis
pelo relógio sincronizado anteriormente com o GPS.

## Consultas pela internet

| Comando | Ação |
| --- | --- |
| `consultar_localidade` | Habilita a consulta de localidade durante 30 segundos e retorna o resultado armazenado. |
| `consultar_tempo` | Habilita a consulta de previsão durante 30 segundos e retorna o resultado armazenado. |

As consultas são assíncronas. Consulte depois com `localidade` ou `tempo`.
`solicitada` informa a janela ativa de solicitação, `consultando` indica uma
requisição em andamento e `falhou` informa falha anterior. O campo `anterior`
identifica resultado que requer atualização. A confirmação de solicitação
não significa que uma nova consulta já terminou.

Aplicam-se os mesmos requisitos de GPS confiável, hora sincronizada e internet,
cache de localidade de 500 m e de previsão de 30 minutos/5 km, além dos
intervalos mínimos de um e cinco minutos respectivamente. Se o intervalo
ainda não tiver terminado na janela de 30 segundos, envie o comando novamente
mais tarde. Repetir a solicitação renova essa janela. As consultas HTTPS
continuam sendo executadas uma por vez.

## Controle do OLED

| Comando | Ação |
| --- | --- |
| `tela_ligar` | Liga e desperta o OLED na página selecionada. |
| `tela_desligar` | Desliga o OLED. |
| `tela_proxima` | Liga o OLED e avança para a próxima página disponível. |
| `tela_relogio` | Abre o relógio quando essa página estiver disponível. |
| `tela_posicao` | Abre a posição com fix confiável. |
| `tela_movimento` | Abre movimento com fix confiável. |
| `tela_busca` | Abre a busca quando o GPS estiver buscando posição. |
| `tela_sem_dados` | Abre a página de ausência de dados quando essa condição estiver ativa. |
| `tela_rede` | Abre a página Wi-Fi. |
| `tela_localidade` | Abre a página de localidade. |
| `tela_tempo` | Abre a previsão. |

A seleção de página liga o painel, desperta a proteção de tela e encerra a
tela inicial. Páginas indisponíveis no estado atual do GPS retornam
`pagina_indisponivel_no_estado_atual_do_gps`. OLED não inicializado retorna
`oled_indisponivel`. O tempo normal para ativar a proteção de tela continua
sendo aplicado depois do comando. `pagina` descreve a página selecionada,
inclusive quando o painel está desligado ou na proteção de tela.

## LED e sistema

| Comando | Ação |
| --- | --- |
| `led_ligar` | Mantém o LED externo D8 ligado. |
| `led_desligar` | Mantém o LED externo D8 desligado. |
| `led_auto` | Restaura o modo automático do GPS, piscando na busca e aceso com fix. |
| `reiniciar` | Confirma a solicitação e agenda o reinício para aproximadamente um segundo depois. |

O modo manual do LED permanece até `led_auto` ou o reinício. O serviço UDP
não exige autenticação. Dispositivos com acesso à porta 4210 podem consultar
a posição e executar os controles, inclusive `reiniciar`.

## Envio pelo PowerShell

Altere o IP e o comando antes de executar. O exemplo aguarda uma resposta
por até três segundos e fecha o socket ao terminar.

```powershell
$destino = '192.168.1.67'
$comando = 'ajuda'
$cliente = [System.Net.Sockets.UdpClient]::new()
$cliente.Client.ReceiveTimeout = 3000
try {
    $cliente.Connect($destino, 4210)
    $bytes = [Text.Encoding]::UTF8.GetBytes($comando)
    [void]$cliente.Send($bytes, $bytes.Length)
    $origem = [Net.IPEndPoint]::new([Net.IPAddress]::Any, 0)
    $resposta = $cliente.Receive([ref]$origem)
    [Text.Encoding]::UTF8.GetString($resposta)
} finally {
    $cliente.Dispose()
}
```

UDP pode perder pacotes. Consultas e comandos de LED/tela por nome podem ser
repetidos. Para `tela_proxima`, repetir pode avançar mais de uma página. O
comando `reiniciar` responde antes do reinício, mas a confirmação também pode
se perder. Não há identificador de transação nem garantia de execução única.

Compilação, comunicação UDP e validação na placa ficam a cargo do usuário.
