#include "usart.h"
#include <stm32g031xx.h>
#include <string.h>
#include "timer.h"

//================================================================
// 1. DEFINITIONS DE L'UART
//================================================================

//Initialise USART1 sur D0 (TX) et D1 (RX) à 9600 bauds.
void UART_Init(void) {
    // 1. Activer les horloges (Port B et USART1)
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;		//gpio b
    RCC->APBENR2 |= RCC_APBENR2_USART1EN; 	// USART1

    // 2. Configurer PB6 (TX) et PB7 (RX) D0/D1 en mode alternate
    GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
    GPIOB->MODER |= (2 << GPIO_MODER_MODE6_Pos) | (2 << GPIO_MODER_MODE7_Pos);

    // Choisir AF0 (USART1) pour PB6 et PB7
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6_Msk | GPIO_AFRL_AFSEL7_Msk);

    // 3. Configurer USART1
    USART1->CR1 &= ~USART_CR1_UE;			//desactive uart pour config
    USART1->CR1 &= ~USART_CR1_M;			// mode 8bits de données
    USART1->CR2 &= ~USART_CR2_STOP;			//1 bit de stop

    // ---- BAUD RATE  16MHz ----
    USART1->BRR = 1667; 					// (16,000,000 / 9600)

    // Activer Transmetteur (TE), Récepteur (RE) et l'UART (UE)
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART1->CR1 |= USART_CR1_UE;
}

//Envoie un seul caractère sur l'UART.
void UART_envoie_car(char c) {
    while (!(USART1->ISR & USART_ISR_TXE_TXFNF)) {}		//attente pour ne pas saturer le tampon
    USART1->TDR = c;
}

//Envoie une chaîne de caractères sur l'UART.
void UART_envoie_chaine(const char* str) {
    while (*str) {
        UART_envoie_car(*str++);
    }
}

//Lit les données de l'UART jusqu'à ce qu'un timeout soit atteint.
void UART_LireReponse_AvecDelai(char* buffer, int buffer_taille, uint32_t timeout_ms) {
    char c;
    int i = 0;
    uint32_t start_time = SYSTICK_Get(); 					// Récupère le temps actuel

    memset(buffer, 0, buffer_taille); 						// Vider le buffer rempli avec des 0
    														//sinon vieilles données qui empeche la suite
    while (i < (buffer_taille - 1)) {					 		//boucle tant que tableau pas rempli et place libre pour caractere de fin
        // Vérifier si un caractère est arrivé
        if (USART1->ISR & USART_ISR_RXNE_RXFNE) {			//si flag leve alors on stock dans une variable et on avance
            c = (char)(USART1->RDR & 0xFF);
            buffer[i++] = c;
        }
        // Vérifier le timeout
        if ((SYSTICK_Get() - start_time) > timeout_ms) {	//diff entre mtn et le temps de debut
            buffer[i] = '\0'; 								// Terminer la chaîne
            return; 										// Sortir (timeout)
        }
    }
    buffer[i] = '\0';										//pour que l'on puisse sortir de la fonction si on lit la chaine'
}

//lire une réponse avec un timeout par défaut de 2 secondes.
void UART_LireReponse(char* buffer, int buffer_taille) {
    UART_LireReponse_AvecDelai(buffer, buffer_taille, 2000); // 2000ms
}
