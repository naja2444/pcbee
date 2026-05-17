#ifndef USART_H
#define USART_H

#include <stdint.h>

// Initialise l'UART (9600 bauds)
void UART_Init(void);

// Envoie un caractère
void UART_envoie_car(char c);

// Envoie une chaîne de caractères
void UART_envoie_chaine(const char* str);

// Lecture avec timeout
void UART_LireReponse_AvecDelai(char* buffer, int buffer_taille, uint32_t timeout_ms);

// Raccourci
void UART_LireReponse(char* buffer, int buffer_taille);

#endif
