#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/matrizRGB.h"
#include "math.h"

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
#define NUM_AMOSTRAS 50000   // Número de amostras para média

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
float calcular_resistencia(float adc_valor);
float ler_adc_com_media(void);
void replace_char(char *str, char find, char replace);

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
// Estrutura para retornar dados do resistor
// ==============================

typedef struct
{
    int primeiro_digito;
    int segundo_digito;
    int multiplicador;
    float valor_medido;
    float valor_teorico;
    float erro_percentual;
} DadosResistor;

DadosResistor detectar_valor_resistor(float resistencia);
void exibir_resistor_no_display(DadosResistor dados);

const float tabela_resistores_e24[] = {
    1.0, 1.1, 1.2, 1.3, 1.5, 1.6,
    1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
    3.3, 3.6, 3.9, 4.3, 4.7, 5.1,
    5.6, 6.2, 6.8, 7.5, 8.2, 9.1};

npColor_t cores_resistores[] = {
    COLOR_BLACK,
    COLOR_BROWN,
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_VIOLET,
    COLOR_GREY,
    COLOR_WHITE,
    COLOR_GOLD,
    COLOR_SILVER};

const char *cores[] = {
    "PRETO",    // 0
    "MARROM",   // 1
    "VERMELHO", // 2
    "LARANJA",  // 3
    "AMARELO",  // 4
    "VERDE",    // 5
    "AZUL",     // 6
    "VIOLETA",  // 7
    "CINZA",    // 8
    "BRANCO"    // 9
};
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

        DadosResistor valores = detectar_valor_resistor(r_x); // Detecta o valor do resistor

        // printf("Valor primeiro digito: %d\n", valores.primeiro_digito);
        // printf("Valor segundo digito: %d\n", valores.segundo_digito);
        // printf("Valor multiplicador: %d\n", valores.multiplicador);
        // printf("Valor medido: %.2f\n", valores.valor_medido);
        // printf("Valor teorico: %.2f\n", valores.valor_teorico);
        // printf("Erro percentual: %.2f%%\n", valores.erro_percentual);

        // printf("Valor adc: %.2f\n", adc_valor);

        if (tempo_atual - tempo_anterior >= 700)
        {
            exibir_resistor_no_display(valores); // Exibe os dados do resistor no display
        }
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
    printf("Valor do adc_max: %.2f\n", adc_read());

    float soma = 0.0f;
    for (int i = 0; i < NUM_AMOSTRAS; i++)
    {
        soma += adc_read();
        sleep_us(1);
    }

    return soma / NUM_AMOSTRAS;
}

float calcular_resistencia(float adc_valor)
{
    // Fórmula: R_x = R_conhecido * ADC_encontrado / (ADC_RESOLUTION - ADC_encontrado)
    return (R_CONHECIDO * adc_valor) / (ADC_RESOLUTION - adc_valor);
}

DadosResistor detectar_valor_resistor(float resistencia)
{
    DadosResistor dados;
    dados.valor_medido = resistencia;

    // Determinar potência (ordem de grandeza)
    float valor_normalizado = resistencia;
    int potencia = 0;

    // Normalizar para valor entre 1.0 e 99.9
    if (resistencia >= 100)
    {
        while (valor_normalizado >= 100)
        {
            valor_normalizado /= 10;
            potencia++;
        }
    }
    else if (resistencia < 10 && resistencia > 0)
    {
        while (valor_normalizado < 10)
        {
            valor_normalizado *= 10;
            potencia--;
        }
    }

    // Extrair primeiro e segundo dígitos
    dados.primeiro_digito = (int)valor_normalizado / 10;
    dados.segundo_digito = (int)valor_normalizado % 10;
    dados.multiplicador = potencia;

    // Buscar valor mais próximo na tabela E24
    float valor_normalizado_tabela = 0;
    float menor_diferenca = 100.0;

    for (int i = 0; i < 24; i++)
    {
        float diferenca = fabs(valor_normalizado - tabela_resistores_e24[i] * 10);
        if (diferenca < menor_diferenca)
        {
            menor_diferenca = diferenca;
            valor_normalizado_tabela = tabela_resistores_e24[i] * 10;
        }
    }

    // Recalcular os dígitos com o valor da tabela
    dados.primeiro_digito = (int)valor_normalizado_tabela / 10;
    dados.segundo_digito = (int)valor_normalizado_tabela % 10;

    // Calcular valor teórico com os dígitos e multiplicador
    dados.valor_teorico = (dados.primeiro_digito * 10 + dados.segundo_digito) * pow(10, dados.multiplicador);

    // Calcular erro percentual
    if (dados.valor_teorico != 0)
    {
        dados.erro_percentual = fabs(dados.valor_medido - dados.valor_teorico) / dados.valor_teorico * 100;
    }
    else
    {
        dados.erro_percentual = 0;
    }

    return dados;
}

void exibir_resistor_no_display(DadosResistor dados)
{
    int i = 0;
    char str_valor_medido[20];
    char str_valor_teorico[20];
    char str_erro[20];
    char buffer[20];

    // Formatar valores substituindo ponto por vírgula
    sprintf(buffer, "%1.0f", dados.valor_medido);
    replace_char(buffer, '.', ',');
    strcpy(str_valor_medido, buffer);

    sprintf(buffer, "%1.0f", dados.valor_teorico);
    replace_char(buffer, '.', ',');
    strcpy(str_valor_teorico, buffer);

    sprintf(buffer, "%.2f%%", dados.erro_percentual);
    replace_char(buffer, '.', ',');
    strcpy(str_erro, buffer);

    // Limpar o display
    ssd1306_fill(&ssd, false);

    // Desenhar valores
    ssd1306_draw_string(&ssd, "Medido:", 0, 0);
    ssd1306_draw_string(&ssd, str_valor_medido, 70, 0);

    ssd1306_draw_string(&ssd, "Teorico:", 0, 10);
    ssd1306_draw_string(&ssd, str_valor_teorico, 70, 10);

    ssd1306_draw_string(&ssd, "Erro:", 0, 20);
    ssd1306_draw_string(&ssd, str_erro, 70, 20);

    for (i = 0; i < 10; i++)
    {
        if (i == dados.primeiro_digito)
        {
            ssd1306_draw_string(&ssd, "FAIXA1:", 0, 35);
            ssd1306_draw_string(&ssd, cores[i], 60, 35);
            npSetRowIntensity(0, cores_resistores[i], 1);
        }
        if (i == dados.segundo_digito)
        {
            ssd1306_draw_string(&ssd, "FAIXA2:", 0, 45);
            ssd1306_draw_string(&ssd, cores[i], 60, 45);
            npSetRowIntensity(2, cores_resistores[i], 1);
        }

        if (i == dados.multiplicador)
        {
            ssd1306_draw_string(&ssd, "FAIXA3:", 0, 55);
            ssd1306_draw_string(&ssd, cores[i], 60, 55);
            npSetRowIntensity(4, cores_resistores[i], 1);
        }
    }

    // Enviar dados para o display
    ssd1306_send_data(&ssd);
}

// Função auxiliar para substituir caracteres
void replace_char(char *str, char find, char replace)
{
    for (char *p = str; *p; p++)
    {
        if (*p == find)
            *p = replace;
    }
}