#include "poids.h"
#include "timer.h" // Importation de la base de temps haute résolution (Delai_us)
#include <stm32g031xx.h>

/* Définition du mapping matériel du pont d'acquisition HX711 */
// Assignation : ligne de données série (DT) sur PB1, horloge synchrone (SCK) sur PA15
#define PORT_DT     GPIOB
#define PIN_DT      1
#define PORT_SCK    GPIOA
#define PIN_SCK     15

/* Variable d'état interne pour la compensation d'offset (Tare relative) */
static volatile int32_t valeur_tare = 0;

void Poids_GPIO_Init(void) {
    // Activation de la distribution d'horloge pour les ports A et B
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;

    // --- Configuration de l'horloge synchrone (SCK sur PA15) en mode Sortie (Push-Pull) ---
    PORT_SCK->MODER &= ~(GPIO_MODER_MODE15_1);
    PORT_SCK->MODER |=  (GPIO_MODER_MODE15_0);

    // --- Configuration de la ligne de données (DT sur PB1) en mode Entrée ---
    PORT_DT->MODER &= ~(GPIO_MODER_MODE1_0 | GPIO_MODER_MODE1_1);
    PORT_DT->PUPDR |=  (GPIO_PUPDR_PUPD1_0); // Activation de la résistance de tirage au niveau haut (Pull-up interne)

    // Forçage de l'horloge au niveau logique bas (État de repos nominal du composant)
    PORT_SCK->BRR = (1 << PIN_SCK);
}

// Routine bas niveau d'acquisition synchrone (Résolution effective de 24 bits)
int32_t Poids_Lire_Unique(void) {
    uint32_t compte = 0;

    // Scrutation du signal de disponibilité des données (Data Ready via abaissement de DT)
    // Implémentation d'un Watchdog logiciel pour prévenir le verrouillage du thread principal
    uint32_t timeout = 100000;
    while (PORT_DT->IDR & (1 << PIN_DT)) {
        timeout--;
        if(timeout == 0) return 0; // Levée d'exception : périphérique défaillant ou perte de liaison électrique
    }

    // Échantillonnage de la trame de 24 bits via génération de fronts d'horloge
    for (int i = 0; i < 24; i++) {
        PORT_SCK->BSRR = (1 << PIN_SCK); // Génération du front montant (Setup Time)
        Delai_us(1);
        compte = compte << 1;            // Décalage du registre de réception
        PORT_SCK->BRR = (1 << PIN_SCK);  // Génération du front descendant (Hold Time)
        Delai_us(1);

        if (PORT_DT->IDR & (1 << PIN_DT)) {
            compte++;                    // Insertion de l'état logique échantillonné
        }
    }

    // Génération de la 25ème impulsion de configuration (Sélection du Gain interne A=128)
    PORT_SCK->BSRR = (1 << PIN_SCK);
    Delai_us(1);
    PORT_SCK->BRR = (1 << PIN_SCK);
    Delai_us(1);

    // Extension du bit de signe (Complément à deux) pour alignement sur l'architecture 32-bits (int32_t)
    if (compte & 0x800000) {
        compte |= 0xFF000000;
    }
    return (int32_t)compte;
}

int32_t Poids_Lire_Moyenne(uint8_t nombre_lectures) {
    long long somme = 0;
    for (int i = 0; i < nombre_lectures; i++) {
        somme += Poids_Lire_Unique();
    }
    return (int32_t)(somme / nombre_lectures);
}

/* Routine d'extraction de la donnée brute non compensée (Mode Calibration/Polynôme) */
int32_t Poids_Lire_Brut_Pur(void) {
    return Poids_Lire_Moyenne(5); // Sous-échantillonnage de lissage (5 itérations)
}

void Poids_Tare(void) {
    // Détermination de l'offset statique du système par suréchantillonnage de la ligne de base
    valeur_tare = Poids_Lire_Moyenne(10);
}

int32_t Poids_Obtenir_Valeur(void) {
    // Restitution de la mesure courante compensée par l'offset d'étalonnage dynamique
    return (Poids_Lire_Moyenne(3) - valeur_tare);
}

/* =========================================================================
   GESTION DE LA MÉMOIRE NON VOLATILE (FLASH INTERNE)
   ========================================================================= */
#define ADRESSE_FLASH_TARE 0x0800F800 // Allocation d'un secteur dédié en fin de mémoire programme

void Sauvegarder_Tare_Flash(int32_t tare_a_sauver) {
    // Attente de la disponibilité du contrôleur mémoire (Busy Flag)
    while ((FLASH->SR & FLASH_SR_BSY1) != 0);

    // Déverrouillage de sécurité des registres de la mémoire Flash (Séquence d'accès)
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }

    // Séquence d'effacement de page (Page Erase)
    FLASH->CR |= FLASH_CR_PER;
    FLASH->CR &= ~FLASH_CR_PNB_Msk;
    FLASH->CR |= (31 << FLASH_CR_PNB_Pos); // Sélection de la page cible (Page 31)
    FLASH->CR |= FLASH_CR_STRT;            // Déclenchement de l'opération
    while ((FLASH->SR & FLASH_SR_BSY1) != 0);
    FLASH->CR &= ~FLASH_CR_PER;

    // Séquence de programmation des mots mémoire (Programming mode)
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint32_t*)ADRESSE_FLASH_TARE = (uint32_t)tare_a_sauver; // Injection de la donnée (Mot de poids faible)
    *(volatile uint32_t*)(ADRESSE_FLASH_TARE + 4) = 0x00000000;        // Initialisation du double mot (Alignement 64-bit)
    while ((FLASH->SR & FLASH_SR_BSY1) != 0);
    FLASH->CR &= ~FLASH_CR_PG;

    // Reverrouillage de sécurité du contrôleur
    FLASH->CR |= FLASH_CR_LOCK;
}

void Poids_Initialiser_Tare(void) {
    // Lecture directe du pointeur mémoire
    uint32_t memoire = *(volatile uint32_t*)ADRESSE_FLASH_TARE;

    // Évaluation de l'état vierge de la mémoire (0xFFFFFFFF post-effacement)
    if (memoire == 0xFFFFFFFF) {
        valeur_tare = Poids_Lire_Moyenne(10);
        Sauvegarder_Tare_Flash(valeur_tare);
    } else {
        valeur_tare = (int32_t)memoire; // Restauration de l'état persistant
    }
}