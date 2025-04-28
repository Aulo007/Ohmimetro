// matrizRGB.h
#ifndef MATRIZRGB_H
#define MATRIZRGB_H
#include <stdint.h>
#include "pico/stdlib.h"

#define LED_COUNT 25

// Definição do pixel/LED
typedef struct
{
    uint8_t G, R, B;
} npLED_t;

// Cores predefinidas
#define COLOR_RED     (npColor_t){255, 0, 0}
#define COLOR_GREEN   (npColor_t){0, 255, 0}
#define COLOR_BLUE    (npColor_t){0, 0, 255}
#define COLOR_WHITE   (npColor_t){255, 255, 255}
#define COLOR_BLACK   (npColor_t){0, 0, 0}
#define COLOR_YELLOW  (npColor_t){255, 255, 0}
#define COLOR_CYAN    (npColor_t){0, 255, 255}
#define COLOR_MAGENTA (npColor_t){255, 0, 255}
#define COLOR_PURPLE  (npColor_t){128, 0, 128}
#define COLOR_ORANGE  (npColor_t){255, 165, 0}

// Tipo para representar uma cor RGB
typedef struct {
    uint8_t r, g, b;
} npColor_t;

// Declaração de funções básicas
void npInit(uint8_t pin);
void npClear();
void npWrite();
void setMatrizDeLEDSComIntensidade(int matriz[5][5][3], double intensidadeR, double intensidadeG, double intensidadeB);
int getIndex(int x, int y);

// Novas funções adicionadas
void acenderLED(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void acenderLEDIntensidade(int x, int y, uint8_t r, uint8_t g, uint8_t b, float intensidade);
void acenderLEDCor(int x, int y, npColor_t cor);
void acenderLEDCorIntensidade(int x, int y, npColor_t cor, float intensidade);
void acenderTodaMatriz(uint8_t r, uint8_t g, uint8_t b);
void acenderTodaMatrizIntensidade(uint8_t r, uint8_t g, uint8_t b, float intensidade);
void acenderTodaMatrizCor(npColor_t cor);
void acenderLinha(int linha, uint8_t r, uint8_t g, uint8_t b);
void acenderColuna(int coluna, uint8_t r, uint8_t g, uint8_t b);
void acenderBorda(uint8_t r, uint8_t g, uint8_t b);
void acenderDiagonal(bool principal, uint8_t r, uint8_t g, uint8_t b);

// Funções de efeitos
void efeitoFade(npColor_t cor1, npColor_t cor2, int passos, int delay_ms);
void efeitoPiscagem(npColor_t cor, int vezes, int delay_ms);
void efeitoRotacao(npColor_t cor, int direcao, int vezes, int delay_ms);
void efeitoEspiral(npColor_t cor, int direcao, int delay_ms);
void efeitoOnda(npColor_t cor, int direcao, int vezes, int delay_ms);

// Funções de desenho
void desenharRectangulo(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
void desenharCirculo(int x_center, int y_center, int raio, uint8_t r, uint8_t g, uint8_t b);
void desenharCaractere(char c, uint8_t r, uint8_t g, uint8_t b);

// Utilitários
npColor_t criarCor(uint8_t r, uint8_t g, uint8_t b);
npColor_t misturarCores(npColor_t cor1, npColor_t cor2, float proporcao);
bool posicaoValida(int x, int y);

extern npLED_t leds[LED_COUNT]; // Torna a variável visível externamente
#endif // MATRIZRGB_H

// matrizRGB.c
#include "matrizRGB.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"
#include <math.h>

#define LED_COUNT 25 // Número de Leds na matriz 5x5

// Buffer de pixels global
npLED_t leds[LED_COUNT];
static PIO np_pio;
static uint sm;

// Inicialização da Matrix 5x5, na bitdoglab no pino 7
void npInit(uint8_t pin)
{
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    npClear();
}

// Função que seta, basicamente atribui uma cor a cada pino correspondente da matriz da placa
void setMatrizDeLEDSComIntensidade(int matriz[5][5][3], double intensidadeR, double intensidadeG, double intensidadeB)
{
    // Validação das intensidades
    intensidadeR = (intensidadeR < 0.0 || intensidadeR > 1.0) ? 1.0 : intensidadeR;
    intensidadeG = (intensidadeG < 0.0 || intensidadeG > 1.0) ? 1.0 : intensidadeG;
    intensidadeB = (intensidadeB < 0.0 || intensidadeB > 1.0) ? 1.0 : intensidadeB;

    // Loop para configurar os LEDs
    for (uint8_t linha = 0; linha < 5; linha++)
    {
        for (uint8_t coluna = 0; coluna < 5; coluna++)
        {
            // Calcula os valores RGB ajustados pela intensidade
            uint8_t r = (uint8_t)(float)(matriz[linha][coluna][0] * intensidadeR);
            uint8_t g = (uint8_t)(float)(matriz[linha][coluna][1] * intensidadeG);
            uint8_t b = (uint8_t)(float)(matriz[linha][coluna][2] * intensidadeB);

            uint index = getIndex(coluna, linha);

            // Configura o LED diretamente
            leds[index].R = r;
            leds[index].G = g;
            leds[index].B = b;
        }
    }

    npWrite(); // Função para ligar os leds setados.
}

void npWrite()
{
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

// Função para desligar os leds
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }

    npWrite();
}

// Função para obter a posição correta na matriz 5x5.
int getIndex(int x, int y)
{
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0)
    {
        return 24 - (y * 5 + x); // Linha par (esquerda para direita).
    }
    else
    {
        return 24 - (y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

// Verifica se uma posição é válida na matriz 5x5
bool posicaoValida(int x, int y)
{
    return (x >= 0 && x < 5 && y >= 0 && y < 5);
}

// Cria uma cor RGB
npColor_t criarCor(uint8_t r, uint8_t g, uint8_t b)
{
    npColor_t cor;
    cor.r = r;
    cor.g = g;
    cor.b = b;
    return cor;
}

// Mistura duas cores com base em uma proporção (0.0 a 1.0)
npColor_t misturarCores(npColor_t cor1, npColor_t cor2, float proporcao)
{
    // Limitar a proporção entre 0 e 1
    if (proporcao < 0.0f) proporcao = 0.0f;
    if (proporcao > 1.0f) proporcao = 1.0f;

    npColor_t resultado;
    resultado.r = (uint8_t)(cor1.r * (1.0f - proporcao) + cor2.r * proporcao);
    resultado.g = (uint8_t)(cor1.g * (1.0f - proporcao) + cor2.g * proporcao);
    resultado.b = (uint8_t)(cor1.b * (1.0f - proporcao) + cor2.b * proporcao);
    
    return resultado;
}

// Acende um único LED com cores RGB específicas
void acenderLED(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (posicaoValida(x, y))
    {
        uint index = getIndex(x, y);
        leds[index].R = r;
        leds[index].G = g;
        leds[index].B = b;
        npWrite();
    }
}

// Acende um único LED com cores RGB e intensidade específicas
void acenderLEDIntensidade(int x, int y, uint8_t r, uint8_t g, uint8_t b, float intensidade)
{
    if (posicaoValida(x, y))
    {
        // Limitar intensidade entre 0 e 1
        if (intensidade < 0.0f) intensidade = 0.0f;
        if (intensidade > 1.0f) intensidade = 1.0f;

        uint index = getIndex(x, y);
        leds[index].R = (uint8_t)(r * intensidade);
        leds[index].G = (uint8_t)(g * intensidade);
        leds[index].B = (uint8_t)(b * intensidade);
        npWrite();
    }
}


// Acende uma linha específica com uma cor
void acenderLinha(int linha, uint8_t r, uint8_t g, uint8_t b)
{
    if (linha >= 0 && linha < 5)
    {
        for (int x = 0; x < 5; x++)
        {
            uint index = getIndex(x, linha);
            leds[index].R = r;
            leds[index].G = g;
            leds[index].B = b;
        }
        npWrite();
    }
}

// Acende uma coluna específica com uma cor
void acenderColuna(int coluna, uint8_t r, uint8_t g, uint8_t b)
{
    if (coluna >= 0 && coluna < 5)
    {
        for (int y = 0; y < 5; y++)
        {
            uint index = getIndex(coluna, y);
            leds[index].R = r;
            leds[index].G = g;
            leds[index].B = b;
        }
        npWrite();
    }
}

// Acende a borda da matriz
void acenderBorda(uint8_t r, uint8_t g, uint8_t b)
{
    // Primeira e última linha
    for (int x = 0; x < 5; x++)
    {
        acenderLED(x, 0, r, g, b);
        acenderLED(x, 4, r, g, b);
    }
    // Primeira e última coluna (excluindo cantos já acesos)
    for (int y = 1; y < 4; y++)
    {
        acenderLED(0, y, r, g, b);
        acenderLED(4, y, r, g, b);
    }
    npWrite();
}

// Acende uma diagonal (principal ou secundária)
void acenderDiagonal(bool principal, uint8_t r, uint8_t g, uint8_t b)
{
    if (principal)
    {
        // Diagonal principal: do canto superior esquerdo ao inferior direito
        for (int i = 0; i < 5; i++)
        {
            acenderLED(i, i, r, g, b);
        }
    }
    else
    {
        // Diagonal secundária: do canto superior direito ao inferior esquerdo
        for (int i = 0; i < 5; i++)
        {
            acenderLED(4 - i, i, r, g, b);
        }
    }
    npWrite();
}

