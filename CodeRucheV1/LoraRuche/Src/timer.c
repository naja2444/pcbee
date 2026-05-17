#include <stdint.h>
#include "stm32g031xx.h"
#include "timer.h"

uint32_t SystemCoreClock = 16000000;
uint32_t ticks = 0;

void SYSTICK_Init(void){
	SysTick_Config(SystemCoreClock / 1000);
}

void SYSTICK_Delay(uint32_t Delay)
{
	uint32_t tickstart = SYSTICK_Get();
	uint32_t wait = Delay;

	while((SYSTICK_Get() - tickstart) < wait);
}

uint32_t SYSTICK_Get(void){
	return ticks;
}

/* ==============================================================
   GESTION DES BASES DE TEMPS HAUTE RÉSOLUTION (Capteurs externes)
   ============================================================== */

/* Configuration matérielle du Timer 3 (Base de temps microseconde) */

void Timer_Init_Microsecondes(void) {
    // 1. Activation du signal d'horloge du périphérique TIM3
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    // 2. Configuration du prédiviseur (Prescaler)
    // Application : Fréquence source 16 MHz. Cible 1 MHz (Résolution 1 µs/tick).
    // Calcul : (F_clk / F_cible) - 1 = (16 / 1) - 1 = 15
    TIM3->PSC = 15;

    // 3. Configuration de la limite de comptage (Auto-Reload Register)
    TIM3->ARR = 0xFFFF; // Extension à la valeur maximale (65535)

    // 4. Activation du registre de comptage
    TIM3->CR1 |= TIM_CR1_CEN;
}

void Delai_us(uint32_t us) {
    // Mémorisation du registre de comptage lors de l'appel de la fonction
    uint16_t temps_debut = TIM3->CNT;

    // Attente active jusqu'au franchissement du seuil requis
    // Note : Le cast (uint16_t) prend structurellement en charge les événements de dépassement (overflow)
    while ((uint16_t)(TIM3->CNT - temps_debut) < us);
}