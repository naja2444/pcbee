#include "poids.h"
#include "timer.h" // Importation de la base de temps haute résolution (Delai_us)
#include <stm32g031xx.h>

/* Définition du mapping matériel du pont d'acquisition HX711 */
// Assignation : ligne de données (DT) sur PB4, horloge synchrone (SCK) sur PB5
#define PORT_POIDS GPIOB
#define PIN_DT     4
#define PIN_SCK    5

/* Variable d'état interne pour la compensation d'offset (Tare absolue) */
static int32_t valeur_tare = 0;

void Poids_GPIO_Init(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    // Configuration de la ligne d'horloge synchrone (SCK) en mode Sortie (Push-Pull)
    PORT_POIDS->MODER &= ~(GPIO_MODER_MODE5_1);
    PORT_POIDS->MODER |=  (GPIO_MODER_MODE5_0);

    // Configuration de la ligne de données série (DT) en mode Entrée
    PORT_POIDS->MODER &= ~(GPIO_MODER_MODE4_0 | GPIO_MODER_MODE4_1);
    PORT_POIDS->PUPDR |=  (GPIO_PUPDR_PUPD4_0); // Activation de la résistance de tirage au niveau haut (Pull-up interne)

    // Forçage de la ligne d'horloge au niveau bas (État de repos nominal du composant)
    PORT_POIDS->BRR = (1 << PIN_SCK);
}

// Routine bas niveau d'acquisition et de conversion (Trame 24 bits)
int32_t Poids_Lire_Unique(void) {
    uint32_t compte = 0;

    // Scrutation du signal de disponibilité des données (Data Ready via abaissement de DT)
    // Implémentation d'un Watchdog logiciel pour prévenir le blocage du thread principal
    uint32_t timeout = 100000;
    while (PORT_POIDS->IDR & (1 << PIN_DT)) {
        timeout--;
        if(timeout == 0) return 0; // Levée d'exception : périphérique défaillant ou défaut de liaison électrique
    }

    // Échantillonnage de la trame de 24 bits via génération de fronts d'horloge
    for (int i = 0; i < 24; i++) {
        PORT_POIDS->BSRR = (1 << PIN_SCK); // Génération du front montant (Setup)
        Delai_us(1);
        compte = compte << 1;              // Décalage du registre de réception
        PORT_POIDS->BRR = (1 << PIN_SCK);  // Génération du front descendant (Hold)
        Delai_us(1);

        if (PORT_POIDS->IDR & (1 << PIN_DT)) {
            compte++;                      // Insertion du bit échantillonné
        }
    }

    // Génération de la 25ème impulsion de configuration (Sélection du Gain A=128 pour l'itération suivante)
    PORT_POIDS->BSRR = (1 << PIN_SCK);
    Delai_us(1);
    PORT_POIDS->BRR = (1 << PIN_SCK);
    Delai_us(1);

    // Extension du bit de signe (Complément à deux) pour alignement sur type standard int32_t
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

void Poids_Tare(void) {
    // Détermination de l'offset statique du système par suréchantillonnage de la ligne de base
    valeur_tare = Poids_Lire_Moyenne(10);
}

int32_t Poids_Obtenir_Valeur(void) {
    // Restitution de la mesure courante compensée par l'offset d'étalonnage
    return (Poids_Lire_Moyenne(3) - valeur_tare);
}