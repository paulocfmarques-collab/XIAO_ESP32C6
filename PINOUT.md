# Pinout — XIAO ESP32-C6 GPS

Mapa das conexões usadas pelo firmware para a placa **Seeed XIAO ESP32-C6**,
um receptor GPS UART e um display OLED 0,96" SSD1306 128x64 I2C.

## Conexões

| XIAO ESP32-C6 | Sinal | Conectar a | Observações |
|---|---|---|---|
| D7 | UART1 RX | TX do módulo GPS | Recebe os dados NMEA; 9600 baud, 8N1. |
| D6 | UART1 TX | RX do módulo GPS | Transmite para o GPS. |
| D4 (GPIO22) | I2C SDA | SDA do OLED | Dados I2C. |
| D5 (GPIO23) | I2C SCL | SCL do OLED | Clock I2C. |
| D8 (GPIO19) | Saída digital | Resistor em série e LED externo para GND | Pisca a cada 500 ms durante a busca; fica aceso com posição válida e pelo menos três satélites. |
| D9 (GPIO20) | Entrada com pull-up interno | Botão entre D9 e GND | Toque curto troca a tela; segurar por 2 s desliga o OLED; próximo toque liga. Debounce de 40 ms. |
| USB | Serial | Computador / monitor serial | Monitor serial configurado em 115200 baud. |

## Ligações por módulo

### GPS

```text
XIAO D7 (UART1 RX)  <-->  GPS TX
XIAO D6 (UART1 TX)  <-->  GPS RX
XIAO GND            <-->  GPS GND
```

Configure o GPS para **9600 baud, 8N1**. A alimentação do módulo deve seguir
as especificações do próprio GPS e da placa.

### OLED 0,96" SSD1306 128x64 I2C

```text
XIAO D4 (SDA)  <-->  OLED SDA
XIAO D5 (SCL)  <-->  OLED SCL
XIAO 3V3       <-->  OLED VCC
XIAO GND       <-->  OLED GND
```

Alimente o módulo com **3,3 V** e confira os nomes impressos dos quatro pinos:
a ordem física pode variar. O OLED não possui backlight nem conexões BL, CS,
D/C ou RESET externas neste módulo. Remova as ligações do Nokia 5110;
D0, D1 e D2 ficam livres.

O firmware procura o display nos endereços I2C **0x3C** e **0x3D** e registra
o endereço encontrado no monitor serial. Se não encontrar o display, registra
um erro e mantém o funcionamento do GPS, do LED e do botão.

Instale as bibliotecas **Adafruit SSD1306** e **Adafruit GFX** pelo Gerenciador
de Bibliotecas do Arduino; aceite também as dependências solicitadas.

## Notas

### LED e botão

```text
XIAO D8  -->  resistor limitador  -->  anodo LED / catodo  -->  GND
XIAO D9  -->  botao normalmente aberto  -->  GND
```

- As telas só mudam pelo botão: relógio, posição e movimento com posição confiável.
- O toque curto avança ao soltar o botão. Segurar por 2 segundos desliga somente
  o OLED, sem avançar a página. O próximo toque liga sem avançar, mesmo se mantido
  pressionado; GPS e LED continuam ativos com a tela desligada.
- Após 30 segundos sem tocar no botão, aparecem carinhas de robô animadas,
  desenhadas com Adafruit GFX, com brilho normal e movimento pela tela. O primeiro toque sai da proteção
  sem avançar a página; GPS e LED continuam funcionando.
- Durante a busca, o botão alterna entre busca e relógio se o horário já estiver
  disponível. Sem horário, permanece na busca. Abertura e alerta de ausência de
  dados têm prioridade sobre a navegação.
- D0 a D9 são os nomes dos pinos usados pelo sketch para a XIAO ESP32-C6,
  não os números GPIO: D8 corresponde ao GPIO19 e D9 ao GPIO20.
- GPS e OLED devem compartilhar o GND com a placa.
- O LED de status agora é externo em D8; o LED integrado não é controlado.
