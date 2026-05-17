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
   GESTION DES BASES DE TEMPS HAUTE RÉSOLUTION (Périphérique TIM3)
   ============================================================== */

void Timer_Init_Microsecondes(void) {
    // 1. Activation du signal d'horloge du périphérique Timer 3 via le bus APB1
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    // 2. Configuration du diviseur de fréquence (Prescaler)
    // Objectif : Obtenir une résolution temporelle de 1 µs (1 MHz) depuis une source à 16 MHz.
    // Algorithme : PSC = (F_Systeme / F_Cible) - 1 = (16000000 / 1000000) - 1 = 15
    TIM3->PSC = 15;

    // 3. Configuration de la limite de comptage (Auto-Reload Register)
    TIM3->ARR = 0xFFFF; // Maximisation de la plage d'itération (16-bit, soit 65535 µs avant débordement)

    // 4. Activation du registre de comptage interne (Counter Enable)
    TIM3->CR1 |= TIM_CR1_CEN;
}

void Delai_us(uint32_t us) {
    // Mémorisation de l'état du registre de comptage à l'instant t0
    uint16_t temps_debut = TIM3->CNT;

    // Attente active (Polling) jusqu'à ce que le différentiel temporel atteigne la cible requise
    // Note d'ingénierie : Le transtypage explicite en (uint16_t) absorbe structurellement les cas de débordement (Overflow) du timer
    while ((uint16_t)(TIM3->CNT - temps_debut) < us);
}