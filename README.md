# Tarefa final Embarcatech - Botão Auxiliar para Pessoas de Baixa Mobilidade

Este projeto consiste em um dispositivo IoT projetado para auxiliar pessoas com baixa mobilidade. Trata-se de um botão de ajuda que, ao ser pressionado, envia um sinal para o serviço ThingSpeak via Wi-Fi, com diferentes respostas do hardware dependendo do nível de urgência.

## Fluxograma do sistema

![Image](https://github.com/user-attachments/assets/7174a5f9-79c4-45d4-bb3c-e7d68bf4265e)

## Diagrama de aplicação

![Image](https://github.com/user-attachments/assets/29e507a8-1639-45e9-9be8-dff97a1747ae)

## Tecnologias Utilizadas

- Raspberry Pi Pico W
- Display OLED SSD1306 (pinos 14 e 15)
- LED RGB (pinos 11, 12 e 13)
- Buzzer (pino 21)
- Botão Push-Button (pino 5)
- pico/stdlib.h – Biblioteca utilizada para funções básicas de GPIO, temporização e entrada/saída;
- pico/cyw43_arch.h – Responsável por gerenciar o módulo Wi-Fi integrado do Raspberry Pi Pico W;
- lwip/tcp.h e lwip/pbuf.h – Usa TCP/IP utilizada para comunicação HTTP com o serviço do ThingSpeak;
- ssd1306.h – Biblioteca para controle do display OLED SSD1306 via comunicação I2C, permitindo exibição de mensagens no dispositivo;
- hardware/i2c.h – Gerencia a interface I2C para comunicação com o display oLED;
- hardware/pwm.h – Configura e controla o PWM do buzzer, permitindo gerar tons sonoros;
- hardware/clocks.h – Gerencia os clocks do sistema, necessários para configurar o PWM do buzzer corretamente;
- string.h, stdio.h, stdlib.h, ctype.h – Bibliotecas padrão do C utilizadas para manipulação de strings, entrada/saída e outras operações diversas.

## Como Funciona

1. **Inicialização**
   - Conecta-se ao Wi-Fi.
   - Configura LEDs, botão e display OLED.

2. **Monitoramento do Botão**
   - Detecta diferentes padrões de toque com debounce e comparação de valores.
   - Aciona LEDs e buzzer de acordo com a necessidade.
   - Envia os dados para a plataforma ThingSpeak.

## Funcionalidades

- **Toque Único**: Envia um pedido de ajuda simples e retorna feedback pelo LED RGB e display oLED
- **Toque Duplo**: Sinaliza uma situação mais urgente e retorna feedback pelo LED RGB e display oLED.
- **Pressionamento Longo**: Dispara um alerta de emergência e retorna feedback pelo LED RGB, display oLED e buzzer.

## Como Configurar

1. **Modificar Credenciais Wi-Fi:**
   No arquivo principal do projeto, altere os valores das constantes:
   ```c
   #define WIFI_SSID "SSID"
   #define WIFI_PASS "SENHA"
   ```

2. **Reinicie a placa em modo Bootsel e inicialize o projeto:**
    ```c
   picotool reboot -f -u
   ```