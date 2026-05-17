#include "poids.h"
#include "timer.h" // Importation de la fonction de temporisation Delai_us
#include <stm32g031xx.h>

/* Définition du mapping matériel du convertisseur A/N HX711 */
// Assignation physique : ligne de données (DT) sur PB4, horloge synchrone (SCK) sur PB5

#define PORT_POIDS GPIOB
#define PIN_DT     4
#define PIN_SCK    5

/* Variable d'état interne pour la compensation d'offset global */
static int32_t valeur_tare = 0;

void Poids_GPIO_Init(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    // Configuration de la ligne d'horloge synchrone (SCK) en mode Sortie
    PORT_POIDS->MODER &= ~(GPIO_MODER_MODE5_1);
    PORT_POIDS->MODER |=  (GPIO_MODER_MODE5_0);

    // Configuration de la ligne de données série (DT) en mode Entrée
    PORT_POIDS->MODER &= ~(GPIO_MODER_MODE4_0 | GPIO_MODER_MODE4_1);
    PORT_POIDS->PUPDR |=  (GPIO_PUPDR_PUPD4_0); // Activation de la résistance de tirage au niveau haut (Pull-up)

    // Forçage de l'horloge au niveau logique bas (État de repos du périphérique)
    PORT_POIDS->BRR = (1 << PIN_SCK);
}

// Routine bas niveau d'acquisition synchrone (Résolution effective de 24 bits)
int32_t Poids_Lire_Unique(void) {
    uint32_t compte = 0;
    uint32_t timeout = 100000; // Mécanisme de chien de garde logiciel (Watchdog/Timeout)
    while (PORT_POIDS->IDR & (1 << PIN_DT)) {
        timeout--;
        if(timeout == 0) return 0; // Levée d'exception matérielle : périphérique non-répondant ou défaut de liaison
    }

    // Processus d'échantillonnage itératif de la trame de 24 bits via décalage de registre
    for (int i = 0; i < 24; i++) {
        PORT_POIDS->BSRR = (1 << PIN_SCK); // Génération du front montant d'horloge
        Delai_us(1);
        compte = compte << 1;
        PORT_POIDS->BRR = (1 << PIN_SCK);  // Génération du front descendant d'horloge
        Delai_us(1);

        if (PORT_POIDS->IDR & (1 << PIN_DT)) {
            compte++;
        }
    }

    // Génération de la 25ème impulsion d'horloge (Configuration du gain interne de l'A/N à 128)
    PORT_POIDS->BSRR = (1 << PIN_SCK);
    Delai_us(1);
    PORT_POIDS->BRR = (1 << PIN_SCK);
    Delai_us(1);

    // Extension du bit de signe (complément à deux) pour alignement sur type int32_t
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
    // Calcul et mémorisation de l'offset initial par suréchantillonnage
    valeur_tare = Poids_Lire_Moyenne(10);
}

int32_t Poids_Obtenir_Valeur(void) {
    // Restitution de la mesure brute compensée par l'offset du système statique
    return (Poids_Lire_Moyenne(3) - valeur_tare);
}