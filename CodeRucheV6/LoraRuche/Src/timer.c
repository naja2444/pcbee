#include <stdint.h>
#include "stm32g031xx.h"
#include "timer.h"

uint32_t SystemCoreClock = 16000000;
uint32_t ticks = 0;
// Drapeau d'état volatile (Maintien de la cohérence mémoire lors des interruptions)
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

/* Base de temps dédiée aux périphériques synchrones critiques */

/* ==============================================================
   GESTION DES BASES DE TEMPS HAUTE RÉSOLUTION (Périphérique TIM3)
   ============================================================== */

void Timer_Init_Microsecondes(void) {
    // 1. Activation de l'arbre d'horloge du périphérique Timer 3 (Bus APB1)
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    // 2. Configuration du diviseur de fréquence matériel (Prescaler)
    // Objectif : Abaisser la fréquence source (16 MHz) pour obtenir une résolution de 1 MHz (1 µs/tick).
    // Application : PSC = (F_Systeme / F_Cible) - 1 = (16 / 1) - 1 = 15
    TIM3->PSC = 15;

    // 3. Configuration du registre de rechargement automatique (Auto-Reload Register)
    TIM3->ARR = 0xFFFF; // Plage d'itération maximale pour un timer 16 bits (65535 µs)

    // 4. Activation matérielle du compteur (Counter Enable)
    TIM3->CR1 |= TIM_CR1_CEN;
}

void Delai_us(uint32_t us) {
    // Extraction de l'état courant du registre de comptage
    uint16_t temps_debut = TIM3->CNT;

    // Boucle d'attente active (Polling) sur le différentiel temporel
    // Note d'ingénierie : Le transtypage explicite en (uint16_t) absorbe mathématiquement les débordements (Overflows)
    while ((uint16_t)(TIM3->CNT - temps_debut) < us);
}

/* =========================================================================
   GESTION DE LA VEILLE PROFONDE (Mode STOP 1 via LPTIM1)
   MÉCANISME SÉCURISÉ ANTI-DÉFAILLANCE MATÉRIELLE
   ========================================================================= */

void Dormir_X_Secondes_Stop_Mode(uint32_t secondes) {
    // 1. Activation de l'oscillateur interne basse vitesse (Low-Speed Internal - LSI : ~32 kHz)
    RCC->CSR |= RCC_CSR_LSION;
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0); // Attente du drapeau de stabilisation de l'oscillateur

    // Routage de la source d'horloge LSI vers le timer basse consommation LPTIM1
    RCC->CCIPR &= ~RCC_CCIPR_LPTIM1SEL_Msk;
    RCC->CCIPR |= (1 << RCC_CCIPR_LPTIM1SEL_Pos);
    RCC->APBENR1 |= RCC_APBENR1_LPTIM1EN;

    // Configuration des registres du Low-Power Timer 1
    LPTIM1->CR &= ~LPTIM_CR_ENABLE;             // Désactivation transitoire requise pour configuration
    LPTIM1->CFGR = (5 << LPTIM_CFGR_PRESC_Pos); // Application du diviseur matériel (Résolution cible : 1ms)
    LPTIM1->IER |= LPTIM_IER_ARRMIE;            // Activation du signalement d'atteinte du seuil (ARR Match Interrupt)
    LPTIM1->CR |= LPTIM_CR_ENABLE;              // Réactivation du périphérique

    // --- FIX HARDWARE : TEMPS DE PROPAGATION (MITIGATION DU COMA) ---
    // Temporisation de ~60µs garantissant la stabilisation matérielle du LPTIM1 
    // et la propagation des signaux d'horloge avant exécution des instructions suivantes.
    for(volatile int i = 0; i < 1000; i++);

    // Préparation de l'architecture d'événements (Event Mask)
    EXTI->EMR1 |= (1 << 29);  // Autorisation matérielle de l'événement de réveil LPTIM1
    EXTI->IMR1 &= ~(1 << 29); // Désactivation de l'interruption vectorielle pour prévenir l'appel au Handler ISR

    // Suspension de l'horloge système principale et sélection du profil énergétique
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    PWR->CR1 &= ~PWR_CR1_LPMS;
    PWR->CR1 |= PWR_CR1_LPMS_0; // Sélection du Mode Stop 1 (Maintien SRAM, coupure des horloges noyau)

    // --- 3. BOUCLE DE GESTION TEMPORELLE SÉCURISÉE ---
    while (secondes > 0) {
        // Plafonnement matériel : la résolution 16-bits du LPTIM à 32kHz limite la période maximale à ~60s
        uint32_t duree_sieste_actuelle = (secondes > 60) ? 60 : secondes;

        // Requête de purge du drapeau d'événement de réveil
        LPTIM1->ICR |= LPTIM_ICR_ARRMCF;

        // --- FIX ARCHITECTURAL : SYNCHRONISATION D'HORLOGE LENTE (MINUTES SAUTÉES) ---
        // Attente active garantissant que le domaine d'horloge LSI (très lent) a physiquement 
        // pris en compte la demande de purge émise par le domaine APB (très rapide).
        while (LPTIM1->ISR & LPTIM_ISR_ARRM) {
            // Attente de synchronisation matérielle...
        }

        // Application de la consigne temporelle pour l'itération courante
        LPTIM1->ARR = (duree_sieste_actuelle * 1000) - 1;

        // Déclenchement du comptage en mode itération unique (Single Start) et armement du mode Deep Sleep
        LPTIM1->CR |= LPTIM_CR_SNGSTRT;
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

        // Transition en veille profonde via l'instruction WFE (Wait For Event)
        // Vérification continue du drapeau matériel (ARR Match) pour prévenir les réveils parasites
        while ((LPTIM1->ISR & LPTIM_ISR_ARRM) == 0) {
            __WFE();
        }

        // Décrémentation de la charge temporelle globale
        secondes -= duree_sieste_actuelle;
    }

    // 4. Procédure de rétablissement énergétique post-réveil
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;         // Rétablissement du profil nominal du processeur
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  // Réactivation du gestionnaire de temps système (Systick)
    LPTIM1->CR &= ~LPTIM_CR_ENABLE;             // Extinction matérielle du périphérique LPTIM1

    // 5. Synchronisation et relance de l'interface de communication série
    UART_Init();
}