#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/matrizRGB.h"

// ==============================
// Definições dos pinos e constantes
// ==============================
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_ADDR 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define BOTAO_A 5  // GPIO para botão A
#define BOTAO_B 6  // GPIO para botão B (BOOTSEL)

// ==============================
// Configurações do ohmímetro
// ==============================
#define R_CONHECIDO 10000   // Resistor de 10k ohm
#define ADC_VREF 3.31       // Tensão de referência do ADC
#define ADC_RESOLUTION 4095 // Resolução do ADC (12 bits)
#define NUM_AMOSTRAS 5000   // Número de amostras para média

// ==============================
// Variáveis globais
// ==============================
static ssd1306_t ssd;
volatile float r_x = 0.0; // Resistor desconhecido

// ==============================
// Protótipos de funções
// ==============================
void init_i2c(void);
void init_display(void);
void init_adc(void);
void init_buttons(void);
void atualizar_display(bool cor);
float calcular_resistencia(float adc_valor);
float ler_adc_com_media(void);

// ==============================
// Manipulador de interrupção para modo BOOTSEL

// ==============================
void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == BOTAO_B)
    {
        reset_usb_boot(0, 0);
    }
}

// ==============================
// Função principal
// ==============================
int main()
{
    stdio_init_all();

    init_i2c();
    init_display();
    init_adc();
    init_buttons();
    npInit(7); // Inicializa a matriz de LEDs

    // Configuração da interrupção para BOOTSEL
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    bool cor = true;
    uint32_t tempo_anterior = 0;
    int valor = 0;

    while (true)
    {
        uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

        // Leitura e cálculo da resistência
        float adc_valor = ler_adc_com_media();
        r_x = calcular_resistencia(adc_valor);

        // Atualiza display apenas a cada 700ms para evitar flickering
        if (tempo_atual - tempo_anterior >= 700)
        {
            tempo_anterior = tempo_atual;
            atualizar_display(cor);
        }

        // npSetColumn(valor, COLOR_RED);
        // npWrite();      // Atualiza a matriz de LEDs
        // sleep_ms(1000); // Aguarda 1ms
        // npClear();      // Limpa a matriz de LEDs

        // for (valor = 0; valor < 5; valor++)
        // {
        //     sleep_ms(1000); // Aguarda 1ms
        //     npClear();
        //     sleep_ms(10);
        //     npSetRow(valor, COLOR_BLUE);
        // }
    }
}

// ==============================
// Implementação das funções
// ==============================

void init_i2c(void)
{
    i2c_init(I2C_PORT, 400 * 1000); // Configura I2C para 400KHz

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void init_display(void)
{
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, I2C_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Limpa o display inicialmente
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void init_adc(void)
{
    adc_init();
    adc_gpio_init(ADC_PIN); // Configura ADC no pino 28
}

void init_buttons(void)
{
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
}

float ler_adc_com_media(void)
{
    adc_select_input(2); // Seleciona o ADC 2 (pino 28)

    float soma = 0.0f;
    for (int i = 0; i < NUM_AMOSTRAS; i++)
    {
        soma += adc_read();
        sleep_us(100);
    }

    return soma / NUM_AMOSTRAS;
}

float calcular_resistencia(float adc_valor)
{
    // Fórmula: R_x = R_conhecido * ADC_encontrado / (ADC_RESOLUTION - ADC_encontrado)
    return (R_CONHECIDO * adc_valor) / (ADC_RESOLUTION - adc_valor);
}

void atualizar_display(bool cor)
{
    char str_adc[10];
    char str_resistencia[10];

    // Converte os valores para strings
    sprintf(str_adc, "%1.0f", ler_adc_com_media());
    sprintf(str_resistencia, "%1.0f", r_x);

    // Atualiza o display
    ssd1306_fill(&ssd, !cor);

    // Interface gráfica
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
    ssd1306_line(&ssd, 3, 25, 123, 25, cor);      // Desenha uma linha horizontal
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);      // Desenha uma linha horizontal
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);      // Desenha uma linha vertical

    // Textos
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);
    ssd1306_draw_string(&ssd, "  Ohmimetro", 10, 28);
    ssd1306_draw_string(&ssd, "ADC", 13, 41);
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);
    ssd1306_draw_string(&ssd, str_adc, 8, 52);
    ssd1306_draw_string(&ssd, str_resistencia, 59, 52);

    // Atualiza o display
    ssd1306_send_data(&ssd);
}
