#include <stdint.h>
#include "stm32g031xx.h"
#include "timer.h"

uint32_t SystemCoreClock = 16000000;
uint32_t ticks = 0;
// Variable d'état volatile (Maintien de la cohérence mémoire lors des interruptions)
volatile uint8_t alarme_a_sonne = 0;

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
    // 1. Activation de l'arbre d'horloge du périphérique Timer 3 (Bus APB1)
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    // 2. Configuration du diviseur de fréquence matériel (Prescaler)
    // Objectif : Abaisser la fréquence source (16 MHz) pour obtenir une résolution de 1 MHz (1 µs).
    // Application : PSC = (16 / 1) - 1 = 15
    TIM3->PSC = 15;

    // 3. Configuration du registre de rechargement automatique (Auto-Reload Register)
    TIM3->ARR = 0xFFFF; // Plage d'itération maximale pour un timer 16 bits (65535 µs)

    // 4. Activation conditionnelle du compteur
    TIM3->CR1 |= TIM_CR1_CEN;
}

void Delai_us(uint32_t us) {
    // Extraction de la valeur courante du registre de comptage
    uint16_t temps_debut = TIM3->CNT;

    // Boucle d'attente active (Polling) sur le différentiel temporel
    // Note d'architecture : Le transtypage en (uint16_t) absorbe mathématiquement les débordements (Overflows)
    while ((uint16_t)(TIM3->CNT - temps_debut) < us);
}

/* =========================================================================
   GESTION DE LA VEILLE PROFONDE (Mode STOP 1 via LPTIM1)
   MÉCANISME SÉCURISÉ PAR ÉVÉNEMENT MATÉRIEL (Safe-Event Mode)
   ========================================================================= */

void Dormir_X_Secondes_Stop_Mode(uint32_t secondes) {
    // 1. Activation de l'oscillateur interne basse vitesse (Low-Speed Internal - LSI : ~32 kHz)
    RCC->CSR |= RCC_CSR_LSION;
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0); // Attente de la stabilisation matérielle de l'oscillateur

    // 2. Routage de la source d'horloge LSI vers le timer basse consommation LPTIM1
    RCC->CCIPR &= ~RCC_CCIPR_LPTIM1SEL_Msk;
    RCC->CCIPR |= (1 << RCC_CCIPR_LPTIM1SEL_Pos);
    RCC->APBENR1 |= RCC_APBENR1_LPTIM1EN;

    // 3. Configuration des registres du Low-Power Timer 1
    LPTIM1->CR &= ~LPTIM_CR_ENABLE;             // Désactivation transitoire pour configuration sécurisée
    LPTIM1->CFGR = (5 << LPTIM_CFGR_PRESC_Pos); // Application du diviseur pour obtenir une résolution de 1ms
    LPTIM1->IER |= LPTIM_IER_ARRMIE;            // Activation du signalement d'atteinte du seuil de registre (ARR Match Interrupt)

    LPTIM1->CR |= LPTIM_CR_ENABLE;              // Réactivation du périphérique
    LPTIM1->ARR = (1000 * secondes) - 1;        // Application de la consigne temporelle globale

    // Purge de sécurité du drapeau d'interruption
    LPTIM1->ICR |= LPTIM_ICR_ARRMCF;

    // --- SÉQUENCE DE GÉNÉRATION D'ÉVÉNEMENT (Prévention des exceptions vectorielles) ---
    EXTI->EMR1 |= (1 << 29);  // Autorisation matérielle de l'événement de réveil (Event Mask Register)
    EXTI->IMR1 &= ~(1 << 29); // Désactivation de l'interruption vectorielle (Interrupt Mask Register) pour éviter l'appel au Handler

    LPTIM1->CR |= LPTIM_CR_SNGSTRT;             // Déclenchement du comptage en mode itération unique (Single Start)

    // 4. PRÉPARATION DE L'ARCHITECTURE CORTEX-M POUR L'ENDORMISSEMENT
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; // Suspension de l'horloge système principale
    PWR->CR1 &= ~PWR_CR1_LPMS;                  // Réinitialisation de la sélection du mode d'économie d'énergie
    PWR->CR1 |= PWR_CR1_LPMS_0;                 // Sélection du Mode Stop 1 (Coupure des horloges noyau, maintien de la SRAM)
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;          // Activation du bit Sleep-Deep dans le registre de contrôle du processeur

    // 5. --- TRANSITION EN VEILLE PROFONDE ---
    // Boucle de blocage basée sur l'instruction WFE (Wait For Event)
    // Sécurité : Vérification continue du drapeau matériel (ARR Match)
    while ((LPTIM1->ISR & LPTIM_ISR_ARRM) == 0) {
        __WFE(); 
    }

    // 6. --- PROCÉDURE DE RÉTABLISSEMENT POST-RÉVEIL ---
    LPTIM1->ICR |= LPTIM_ICR_ARRMCF;            // Purge du drapeau déclencheur
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;         // Rétablissement du profil énergétique nominal du processeur
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  // Réactivation du gestionnaire de temps système
    LPTIM1->CR &= ~LPTIM_CR_ENABLE;             // Extinction matérielle du périphérique LPTIM1 (Optimisation énergétique)

    // 7. --- SYNCHRONISATION DES PÉRIPHÉRIQUES EXTERNES ---
    // Réinitialisation indispensable du bloc matériel USART suite à la coupure transitoire des bus d'horloge
    UART_Init();
}