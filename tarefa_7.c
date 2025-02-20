#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define LED_R_PIN 13 // LED Vermelho
#define LED_G_PIN 11 // LED Verde
#define LED_B_PIN 12 // LED Azul
#define BUTTON1_PIN 5
#define BUZZER_PIN 21    

#define WIFI_SSID "SSID" // Modificar o valor do SSID de rede
#define WIFI_PASS "SENHA" // Senha de rede

#define API_KEY "UXE2SR9OSHTIMJ3R" // Chave API ThingSpeak

#define TEMPO_BUTTON 2000   
#define TEMPO_DUPLO 500 
#define DEBOUNCE_TIME 50        

static uint32_t ultimo_toque = 0;
static uint32_t inicio_toque = 0;

static int count_button = 0;
static bool botao_press = false;
static bool checar_toques = false;

// Funções para o display SSD1306
void init_display() {
    i2c_init(i2c1, 400000);  // Definindo velocidade de clock do I2C
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    ssd1306_init(); // Inicializa o display
}

void display_message(char *message) {
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length); // Zera o buffer de renderização

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    int y = 0;
    ssd1306_draw_string(ssd, 5, y, message); // Desenha a string
    render_on_display(ssd, &frame_area); // Atualiza o display com a nova mensagem
}

// Inicializa PWM para o buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (100 * 4096)); // 100Hz
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

void beep(uint pin, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_gpio_level(pin, 2048); // 50% duty cycle
    sleep_ms(duration_ms);
    pwm_set_gpio_level(pin, 0);
    sleep_ms(100);
}

// Função para enviar dados ao ThingSpeak
void enviar_req(int field, int value) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        display_message("Erro PCB");
        return;
    }

    ip_addr_t server_ip;
    IP4_ADDR(&server_ip, 44, 205, 141, 64); // IP do ThingSpeak formatado

    err_t err = tcp_connect(pcb, &server_ip, 80, NULL);
    if (err != ERR_OK) {
        printf("Erro ao conectar ao ThingSpeak: %d\n", err);
        display_message("Erro TS.");
        return;
    }
    printf("Conectado ao TS\n");

    char request[256]; // Contruindo a requisição
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field%d=%d HTTP/1.1\r\n"
             "Host: api.thingspeak.com\r\n"
             "Connection: close\r\n"
             "\r\n",
             API_KEY, field, value); // Passando o valor de chave de API, field do canal e o valor gerado

    err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY); 
    if (err != ERR_OK) {
        printf("Erro ao enviar requisição: %d\n", err); // Tratamento de erros
        display_message("Erro req.");
        return;
    }
    tcp_output(pcb);

    printf("requisição enviada: field%d = %d\n", field, value);
}

// Função para conectar ao Wi-Fi
int connect_wifi() {
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        display_message("Erro wi-fi");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");
    display_message("Conectando...");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n"); // Tratamento de erros
        display_message("Erro conexao");
        return 1;
    } else {
        printf("wi-fi conectado\n");
        display_message("Wi-fi ok");
        sleep_ms(500);
    }

    return 0;
}

void ajuda(){
    printf("Toque único. Ajuda simples.\n");

    gpio_put(LED_G_PIN, 1);  // Acende o LED Verde
    enviar_req(1, 0);  // Envia ajuda básica
    display_message("Toque unico");
    sleep_ms(2000);
    display_message("Conectado ao TS");

    gpio_put(LED_G_PIN, 0);  // Desliga LED Verde
}

void urgente(){
    printf("Toque duplo. Ajuda urgente.\n");

    gpio_put(LED_B_PIN, 1);  // Acende o LED Azul
    enviar_req(1, 1);  // Envia ajuda grave
    display_message("Ajuda urgente");
    sleep_ms(2000);
    gpio_put(LED_B_PIN, 0);  // Desliga LED Azul
    display_message("Conectado ao TS");
    
}

void emergencia() {
    printf("Toque longo! Emergência!\n");
    enviar_req(1, 2);  // Envia alerta crítico
    display_message("Alerta Emergencia");

    // Ativa o buzzer e o LED vermelho juntos
    pwm_set_gpio_level(BUZZER_PIN, 2048); // 50% duty cycle
    gpio_put(LED_R_PIN, 1);  // Acende o LED Vermelho

    sleep_ms(4000); // Mantém por 4 segundos

    // Desativa o buzzer e o LED vermelho
    pwm_set_gpio_level(BUZZER_PIN, 0);
    gpio_put(LED_R_PIN, 0);

    display_message("Conectado ao TS");
}

// Função para monitorar o botão e LEDs
void monitor_buttons() {
    bool button_state = !gpio_get(BUTTON1_PIN); // Lê o estado do botão (inverso devido ao pull-up)
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

    if (button_state && !botao_press) {  // Se o botão foi pressionado
        if (tempo_atual - ultimo_toque > DEBOUNCE_TIME) {
            botao_press = true;
            inicio_toque = tempo_atual;
            count_button++;
        }
    }

    if (!button_state && botao_press) {  // Se o botão foi solto
        botao_press = false;
        uint32_t tempo_segurado = tempo_atual - inicio_toque; // Compara a janela de tempo do pressionamento do botão

    if (tempo_segurado >= TEMPO_BUTTON) { // Pressionamento longo
    emergencia(); // Chama a função referente ao comportamento de emergência 

    count_button = 0;
    checar_toques = false;

    return;
    }

        ultimo_toque = tempo_atual;

        if (count_button == 1) {
            checar_toques = true;  // Espera por um toque adicional
        } else if (count_button == 2) {
            urgente(); // Chama a função referente ao comportamento de alerta mais urgente

            count_button = 0;
            checar_toques = false;

        }
    }

    if (checar_toques && (tempo_atual - ultimo_toque > TEMPO_DUPLO)) {
        checar_toques = false;

        if (count_button == 1) {
            ajuda(); // Chama a função referente ao comportamento de pedido de ajuda menos urgente
        }

        count_button = 0;
    }
}

// Função para inicializar LEDs
void init_leds() {
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);  // Configura o LED vermelho como saída

    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);  // Configura o LED verde como saída

    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);  // Configura o LED azul como saída

    gpio_init(BUTTON1_PIN);
    gpio_set_dir(BUTTON1_PIN, GPIO_IN);  // Configura o botão como entrada
    gpio_pull_up(BUTTON1_PIN);  // Ativa o pull-up para o botão (se necessário)
    pwm_init_buzzer(BUZZER_PIN); // Init buzzer
}

int main() {
    stdio_init_all();
    sleep_ms(10000);
    printf("Iniciando aplicação\n");
    init_display();  // Inicializa o display
    display_message("Iniciando aplicação");
    

    if (connect_wifi()) {
        return 1;
    }

    init_leds();  // Inicializa os LEDs

    display_message("Conectado ao TS");

    // Loop principal
    while (true) {
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
        monitor_buttons();  // Atualiza o estado dos botões
        sleep_ms(100);      // Reduz o uso da CPU
    }

    cyw43_arch_deinit();  // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}
