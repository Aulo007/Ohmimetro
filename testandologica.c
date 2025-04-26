#include <stdio.h>

int main(void)
{

    int num = 3300;
    int copia = num;
    int posicao = 0;
    int digitos[20];

    /*
        Código simples para separar os digitos, como ele funciona, pois bem:
        Pegemos o número 8967 como exemplo, o resto da divisão dele por 10 é 7, por 100 é 67 e por ai vai.
        Quando divido 9867 por 10, e faço o resto dele, tenho 6,7. Como armazeno em um número inteiro, a parte
        decimal é descartada.

    */

    while (copia > 0)
    {
        digitos[posicao] = copia % 10; // Armazena o dígito na posição correta
        copia = copia / 10;
        posicao++;
    }

    int potecia = posicao - 2;

    for (int i = posicao - 1; i >= 0; i--)
    {
        printf("%d \n", digitos[i]);         // Imprime os dígitos na ordem correta
        printf("A potencia é: %d", potecia); // Imprime a potência na qual deve ser multiplicada pelo último número
    }
}