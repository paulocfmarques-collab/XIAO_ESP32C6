# Changelog — GPS XIAO ESP32-C6

Histórico das alterações do projeto, organizado da mais recente para a mais antiga.
As mudanças abaixo ainda não possuem uma versão de lançamento.

## 2026-11-04 — Assinatura do gerador de arquivos de argumentos

- Corrigido o parâmetro do callback ProjectTempFileMunge para `env`, conforme
  a assinatura reconhecida pelo SCons. O nome anterior, `build_env`, fazia
  o SCons tratar o objeto Python como texto de comando e falhar na sintaxe.
- Mantida a geração de arquivos de argumentos persistentes para o GCC.
- Compilação e gravação na COM23 pendentes de confirmação pelo usuário.

## 2026-11-04 — Arquivos de argumentos do GCC no Windows

- Adicionado pre-script do PlatformIO para preservar arquivos de resposta
  do GCC no diretório de build, com caminhos em barras `/` e nomes por alvo
  e conteúdo, em resposta ao erro `cannot find @...rsp: Invalid argument`.
- Adaptação aplicada aos ambientes USB e OTA no Windows, usando TempFileMunge
  do SCons e preservando o conteúdo dos argumentos gerados pela plataforma.
- README atualizado com diagnóstico e comando para repetir a compilação.
- Correção pendente de confirmação na compilação pelo usuário, conforme combinado.

## 2026-11-04 — Memória livre e controle da proteção de tela

- Comando sistema informa RAM total e percentual livre do heap, além de
  capacidade, bytes e percentual livres da partição de aplicação em execução.
- Adicionados protecao_tela, protecao_tela_habilitar e protecao_tela_desabilitar,
  totalizando 37 comandos UDP. Status e tela também informam essa configuração.
- Desabilitar a proteção restaura a página, respeitando o painel desligado
  manualmente. Habilitar reinicia a contagem de 60 segundos de inatividade.
- Configuração da proteção mantida em RAM até o reinício.
- README e UDP.md atualizados. Compilação e testes a cargo do usuário.

## 2026-11-04 — Respostas UDP em texto

- Respostas UDP passam a usar texto UTF-8 com campos por linha, listas
  numeradas e recuo para redes e dias de previsão.
- Confirmações usam `ok`, erros usam `erro`, estados usam `sim`/`nao`
  e dados ausentes usam `indisponivel`. O comando ping responde `pong`.
- Convertidos também erros de datagrama inválido e comando muito longo.
- README e UDP.md atualizados com formato e exemplos.
- Compilação e testes permanecem a cargo do usuário.

## 2026-11-04 — Comandos UDP em minúsculas

- Adicionado servidor UDP na porta 4210, disponível por Wi-Fi ou portal,
  com recuperação após mudança de IP e uma requisição atendida por loop.
- Adicionados 34 comandos em minúsculas com respostas JSON para consultas
  de GPS, rede, localidade, previsão, sistema, OTA, OLED, LED e reinício.
- OLED permite seleção remota de páginas disponíveis e LED possui modos
  ligado, desligado e automático. Reinício é agendado após a resposta.
- Solicitações de localidade e previsão usam os caches e intervalos existentes.
- Datagrama limitado a 63 bytes, com rejeição de comandos inválidos e NUL.
- README e UDP.md documentam comandos, respostas e exemplos.
- Compilação e testes na placa permanecem a cargo do usuário.

## 2026-11-03 — Migração para PlatformIO

- Adicionado platformio.ini com pioarduino 55.03.312-1 / Arduino ESP32 3.3.12,
  bibliotecas fixadas e ambientes de gravação USB e OTA sem senha.
- Preservados os fontes na raiz, CPU de 160 MHz, USB CDC e partitions.csv
  com duas aplicações OTA de 1.875 MiB.
- firmware.ps1 e os atalhos firmware.cmd/Makefile passam a usar PlatformIO,
  mantendo etapas, saída detalhada, validação de tamanho e exportação para output.
- Binários exportados agora usam os nomes firmware.bin, bootloader.bin e
  partitions.bin. README atualizado e .pio ignorado pelo Git.

## 2026-11-03 — Layout do painel web

- Painel mais largo com posição e previsão lado a lado em telas grandes,
  cartões por dia e formulário Wi-Fi agrupado por rede.
- Layout em coluna no celular, com campos rotulados e mais espaçamento.

## 2026-11-03 — Compilação e gravação independentes da IDE

- Adicionados firmware.cmd e firmware.ps1 com ações preparar, compilar,
  portas, usb e ota sem senha.
- Ambiente ESP32 3.3.12 e bibliotecas instalados em .arduino no projeto,
  sem depender da configuração da IDE ou de suas alterações em boards.txt.
- Compilação usa partitions.csv local e limite explícito de 1.966.080 bytes;
  gravações recompilam antes e param em caso de erro.
- Binários copiados para output após compilação bem-sucedida; README
  documenta preparação, USB inicial e atualizações OTA pela porta 3232.
- Diretórios de ferramentas e arquivos gerados adicionados ao .gitignore.
- Nenhuma compilação, gravação ou teste executado nesta alteração.

## 2026-11-03 — Correção do limite selecionado na IDE

- Consulta de metadados com Arduino CLI confirmou a opção GPS OTA, tabela
  xiao_gps_ota e limite 1.966.080 na instalação ativa, sem compilar o sketch.
- Documentado encerramento do serviço antigo da IDE caso persista o menu anterior.

- Instalador passa a ajustar também a opção padrão da XIAO_ESP32C6 em
  boards.txt para a tabela GPS OTA e limite de 1.966.080 bytes, com backup.
- A seleção anteriormente salva como default passa a usar as duas aplicações
  de 1.875 MiB após reabrir a IDE; demais esquemas permanecem disponíveis.
- README atualizado. Compilação e testes continuam a cargo do usuário.

## 2026-11-02 — Espaço de firmware para GPS, HTTPS e OTA

- Adicionada tabela de partições de 4 MB com duas aplicações OTA de 1.875 MiB,
  SPIFFS de 128 KiB e preservação da região NVS.
- Adicionado instalador da opção GPS OTA na IDE, com limite de 1.966.080 bytes
  e preservação das demais opções em boards.local.txt.
- Corrigida orientação do README: o esquema padrão de 1.2 MB já não comporta
  os 1.388.942 bytes informados; a nova tabela exige primeira gravação por USB.
- Compilação e testes permanecem a cargo do usuário.
- Documentada a reabertura completa da IDE caso o menu ainda exiba apenas
  as opções anteriores após instalar a partição GPS OTA.

## 2026-11-02 — Painel web de posição e previsão do tempo

- Adicionados posição GPS, satélites, HDOP, cidade, estado e país à página web,
  com atualização a cada três segundos por `/api/posicao` e link para mapa
  somente com posição confiável; formulário Wi-Fi preservado durante a atualização.
- Página aberta habilita consultas sem depender do OLED, com indicação de
  falhas, perda de comunicação e consultas anteriores.
- Adicionada tela PREVISAO DO TEMPO após LOCALIDADE, alternando três dias
  com condição, mínima/máxima e chance de chuva; mesmos dados no painel web.
- Previsão Open-Meteo por HTTPS nas coordenadas GPS, sem chave, com fuso
  automático, cache de 30 minutos/5 km e intervalo mínimo de cinco minutos.
- Consultas HTTPS serializadas em segundo plano para reduzir pico de memória.
- README atualizado; compilação e testes não executados, conforme combinado.

## 2026-11-02 — Consulta de localidade

- Adicionada tela LOCALIDADE após REDE WI-FI, com cidade/localidade, estado
  e país obtidos das coordenadas GPS via BigDataCloud, sem chave de API.
- Consultas HTTPS em tarefa separada, somente com tela ativa, Wi-Fi, fix
  confiável e hora sincronizada; cache de 500 m e intervalo mínimo de um minuto.
- Indicados ausência de rede/GPS, falhas e resultados de consultas anteriores.
- Nomes longos rolam na tela e acentos portugueses são simplificados.
- README documenta envio das coordenadas ao provedor e dependência ArduinoJson 7.
- Compilação e testes não executados, conforme preferência do usuário.

## 2026-11-02 — Página de rede no OLED

- Adicionada página REDE WI-FI à navegação pelo botão D9, com estado, SSID,
  IP e sinal em dBm; no portal, mostra a rede de configuração e seu endereço.
- Página acessível durante busca e ausência de dados GPS.
- Atualizados os indicadores de páginas para incluir a rede e acomodar quatro telas.
- Preservados proteção de tela e desligamento manual do OLED.
- Compilação e testes ficam por conta do usuário, conforme combinado.

## 2026-11-02 — OTA pela IDE do Arduino

- Adicionado ArduinoOTA sem senha, com hostname `xiao-esp32c6` e descoberta
  de porta de rede por mDNS quando conectado ao Wi-Fi cadastrado.
- Serviço encerrado ao perder a conexão e reiniciado quando a rede retorna.
- Registrados início, conclusão e erros de atualização no monitor serial.
- README atualizado com gravação inicial USB, seleção da porta de rede e
  seleção de **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)**, que já
  contém duas partições OTA no pacote ESP32 3.3.12 instalado, apesar do nome
  não mencionar OTA. Documentada a necessidade de USB para trocar a tabela.
- Compilação e testes ficam por conta do usuário; não executados nesta alteração.

## 2026-10-30

### Indicadores de busca GPS

- Adicionada barra animada de ida e volta abaixo do cabeçalho enquanto não há
  posição confiável, tanto na busca quanto na página de relógio.
- Mantida a piscada do LED D8 a cada 500 ms durante a busca e o LED aceso com
  posição válida e pelo menos três satélites.
- Dados de posição ou satélites com cinco segundos sem atualização deixam de
  indicar posição confiável, retomando os indicadores de busca.
- Preservados o desligamento manual do OLED e a proteção de tela com robôs.

### Relógio interno sincronizado pelo GPS

- Ajustado o relógio de sistema do ESP32 com data e hora UTC válidas e recentes
  recebidas juntas do GPS, com nova sincronização a cada minuto.
- Aplicado fuso local UTC-3 (UTC-2 quando a opção de horário de verão está ativa).
- Após a primeira sincronização, a tela usa o relógio interno e continua contando
  mesmo sem novas mensagens GPS. Ao reiniciar, aguarda nova sincronização.
- Registradas a primeira sincronização e as falhas no monitor serial.

### Liga/desliga do OLED pelo botão

- Pressão de 2 segundos em D9 desliga somente o painel OLED.
- O próximo toque liga o display sem avançar a página nem desligá-lo novamente
  se o botão continuar pressionado.
- Toques curtos avançam ao soltar; uma pressão longa não muda a página.
- GPS e LED permanecem ativos, e a proteção de tela não religa o painel desligado.

### Proteção de tela com robôs

- Ativada após 30 segundos sem tocar no botão D9, inclusive durante a busca
  ou ausência de dados do GPS.
- Adicionadas carinhas de robô que piscam, olham para os lados, sorriem,
  piscam um olho e mostram surpresa, desenhadas com Adafruit GFX e deslocamento pela tela.
- Removido `dim(true)`, que define o contraste como zero na biblioteca SSD1306
  e pode tornar a animação invisível; mantido o brilho normal durante a proteção.
- O primeiro toque restaura a página selecionada sem avançar;
  os próximos toques voltam a navegar normalmente.
- GPS e LED continuam ativos, sem pausas bloqueantes durante a animação.

### Diagramação gráfica do OLED

- Adicionados ícones de satélite, relógio, localização, velocidade e alerta.
- Organizadas as páginas com cabeçalho de estado FIX, barras de quantidade de
  satélites e rodapé com indicador de página e instrução do botão D9.
- Destacadas hora e velocidade em fonte maior, com coordenadas, data, altitude
  e precisão alinhadas em áreas próprias.
- Preservadas a orientação de 180 graus, a navegação manual e a sinalização do LED.

### OLED SSD1306 0,96"

- Substituído o Nokia 5110 pelo OLED SSD1306 128x64 via I2C, com SDA em D4,
  SCL em D5, VCC em 3V3 e GND comum. D0, D1 e D2 ficam livres.
- Trocada a dependência Adafruit PCD8544 por Adafruit SSD1306, mantendo Adafruit GFX.
- Adicionada detecção dos endereços 0x3C e 0x3D, com mensagens de falha no
  monitor serial sem interromper o GPS.
- Adaptadas as dimensões da tela para 21 caracteres por linha e oito linhas,
  preservando as páginas, o botão D9 e o LED externo D8.
- Removido o ajuste de contraste específico do Nokia 5110.

### LED externo e navegação por botão

- Transferido o LED de status para D8, ativo em HIGH: pisca a cada 500 ms
  durante a busca e permanece aceso com posição válida e pelo menos três satélites.
- Adicionado botão entre D9 e GND, com pull-up interno e debounce de 40 ms.
- Removida a alternância automática das telas; cada toque avança uma página.
- Preservadas as telas de busca e alerta quando não há dados ou posição confiável.
- Removida a pausa bloqueante do aviso serial de ausência de GPS, mantendo o
  intervalo de um segundo entre avisos sem impedir o botão e o LED.

### Substituição do display

- Substituído o LCD 16x2 I2C pelo Nokia 5110 (PCD8544), com resolução de
  84x48 pixels e comunicação SPI por software.
- Mantidas as telas de abertura, busca do GPS, relógio, posição, movimento e
  aviso de ausência de dados, adaptadas ao formato gráfico do novo display.
- Configurados D4 para MOSI, D5 para SCLK, D0 para D/C, D1 para CS e D2 para
  RESET; o GPS permanece em D6/D7.
- Atualizado o pinout e trocada a dependência de `hd44780` por Adafruit PCD8544
  e Adafruit GFX.
- Corrigida a correspondência entre CLK/DIN no firmware e as ligações do módulo.
- Ajustado o contraste inicial do Nokia 5110 para 60, valor que apresentou
  melhor legibilidade no módulo utilizado.

## 2026-10-29

### Diagnostico NMEA durante a busca

- Adicionado envio das sentencas NMEA brutas para o monitor serial a 115200 baud
  enquanto o GPS nao possui posicao valida com pelo menos tres satelites.
- Mantida a leitura e decodificacao do GPS durante a exibicao do NMEA.
- Concluida a sentenca NMEA em andamento antes de interromper o eco quando uma
  posicao e obtida, evitando linhas cortadas.
- Reservado o relatorio formatado para quando houver posicao valida com pelo
  menos tres satelites, sem misturar o relatorio com uma sentenca NMEA em andamento.
- Retomado o eco NMEA quando o criterio de posicao confiavel deixa de ser atendido.

## 2026-10-28

### Interface do display aprimorada

- Adicionados ícones personalizados de satélite, relógio, localização, altitude,
  velocidade e calendário.
- Adicionadas barras indicativas da quantidade de satélites e um caractere
  reservado para animações, utilizando os oito espaços de caracteres
  personalizados do LCD.
- Adicionada tela de abertura com o nome do projeto e barra de progresso de
  aproximadamente 2,5 segundos.
- Adicionada tela de busca com indicador animado, quantidade de satélites e
  tempo decorrido.
- Adicionado alerta animado para ausência de dados do GPS.
- Organizadas três páginas, alternadas a cada quatro segundos após obter
  posição válida com pelo menos três satélites:
  - **Relógio:** hora local, quantidade de satélites, barras de sinal, data local
    e deslocamento em relação ao UTC.
  - **Posição:** latitude e longitude com seis casas decimais, hemisférios e
    símbolo de grau.
  - **Movimento:** velocidade em km/h, direção cardinal, altitude em metros e
    classificação da precisão baseada no HDOP.
- Ocultada a direção cardinal quando a velocidade é inferior a 2 km/h.
- Configurada atualização da interface a cada 250 ms, enviando ao LCD somente
  as linhas alteradas.
- Ampliada a estrutura `GpsReading` com data, fuso, altitude, velocidade, direção,
  HDOP e estado de obtenção da posição.
- Corrigida a data local nas mudanças de dia causadas pelo fuso brasileiro,
  considerando transições de mês, ano e anos bissextos.

### Suporte ao LCD 16x2 I2C

- Criada a classe [GpsDisplay](GpsDisplay.h), com implementação em
  [GpsDisplay.cpp](GpsDisplay.cpp).
- Configurados os pinos **D4 para SDA** e **D5 para SCL**.
- Integrada a biblioteca **hd44780**, de Bill Perry, com detecção automática do
  endereço e do mapeamento do adaptador I2C.
- Adicionadas mensagens de inicialização, espera por posição e ausência de dados.
- Adicionado registro de falhas de inicialização do LCD na porta serial,
  mantendo o funcionamento do GPS sem o display.
- Integradas a inicialização e a atualização do LCD ao
  [sketch principal](XIAO_ESP32C6.ino).

### Organização orientada a objetos

- Encapsuladas a comunicação UART, a leitura do GPS e a impressão dos relatórios
  na classe [GpsManager](GpsManager.h).
- Encapsulado o controle do LED na classe [StatusLed](StatusLed.h).
- Simplificadas as funções `setup()` e `loop()` para coordenar os objetos.
- Mantidas as configurações de comunicação:
  - GPS na UART 1, a **9600 baud**, com **RX em D7** e **TX em D6**.
  - Monitor serial a **115200 baud**.
  - Fuso padrão **UTC-3**, com opção de **UTC-2** para horário de verão.
- Preservados o relatório serial, o LED aceso com posição válida e pelo menos
  três satélites, a sinalização intermitente durante a busca e o aviso de
  ausência de dados.

### Validação

- Compilação realizada com sucesso para a placa **XIAO ESP32-C6**
  (`esp32:esp32:XIAO_ESP32C6`) após a atualização visual.
- Nenhum erro ou aviso identificado nos arquivos do projeto nessa compilação.
- Validação visual e funcional no hardware ainda pendente.

## 2026-11-02 — Configuração Wi-Fi

- Adicionada página /config com visual do M5NanoC6 e cadastro de cinco redes.
- Credenciais persistidas em Preferences; senhas não retornam ao navegador.
- Rede XIAO_ESP32C6_CONFIG disponível quando nenhuma rede conecta.
- Tentativas sem espera bloqueante, recuperação a cada 10 minutos e reinício
  agendado após salvar, mantendo o processamento do GPS durante as tentativas.
