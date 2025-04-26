#include <stdio.h>
#include "stdlib.h"
#include <math.h> // pow()

const float tabela_resistores_e24[] = {
    1.0, 1.1, 1.2, 1.3, 1.5, 1.6,
    1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
    3.3, 3.6, 3.9, 4.3, 4.7, 5.1,
    5.6, 6.2, 6.8, 7.5, 8.2, 9.1};

void detectar_valor_resistor(int resistencia)
{

    int digitos[20]; // Inicializa o primeiro dígito como 0
    int posicao = 0;
    int resistencia_copia = resistencia;
    while (resistencia_copia > 0)
    {
        digitos[posicao] = resistencia_copia % 10;  // Pega o último dígito
        resistencia_copia = resistencia_copia / 10; // Remove o último dígito
        posicao++;
    }
    printf("Número de dígitos: %d\n", posicao); // Imprime o número de dígitos

    int potencia = posicao - 1;
    int potencia_2_digitos = posicao - 2;

    if (potencia_2_digitos < 100)
    {
        potencia_2_digitos = 0;
    }

    printf("Potência: %d\n", potencia); // Imprime a potência

    for (int i = posicao - 1; i >= 0; i--)
    {
        printf("%d \n", digitos[i]); // Imprime os dígitos na ordem correta
    }
    int valor_teorico = 0;

    float erro = 0;
    float menor_erro = 1e9;
    int resistencia_teorica_atual = 0;

    if (resistencia < 100)
    {
        potencia = 0;
    }

    for (int i = 0; i < 24; i++)
    {
        resistencia_teorica_atual = round(tabela_resistores_e24[i] * pow(10, potencia)); // Calcula o valor teórico
        erro = abs(resistencia_teorica_atual - resistencia);                             // Calcula o erro absoluto
        if (erro < menor_erro)
        {
            menor_erro = erro;                         // Atualiza o menor erro
            valor_teorico = resistencia_teorica_atual; // Atualiza o valor teórico
        }
    }

    printf("Valor teórico: %d\n", valor_teorico);                                          // Imprime o valor teórico
    printf("Valor real: %d\n", resistencia);                                               // Imprime o valor real
    printf("erro: %.2f%%", abs(valor_teorico - resistencia) / (float)valor_teorico * 100); // Imprime o erro percentual
}
int main(void)
{
    system("chcp 65001 > NULL");
    int num = 123;

    /*
        Código simples para separar os dígitos de um número.
        Exemplo: 8967 -> dígitos armazenados em digitos[]
    */
    detectar_valor_resistor(num);

    return 0;
}
