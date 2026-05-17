#include "poids.h"
#include "timer.h" 
#include <stm32g031xx.h>

/* Configuration de l'interface matérielle du HX711 */
#define PORT_POIDS GPIOB
#define PIN_DT     4  // Ligne de données (Data)
#define PIN_SCK    5  // Ligne d'horloge série (Clock)

/* Variable statique de référence de la balance */
static int32_t valeur_tare = 0;

void Poids_GPIO_Init(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

    // Configuration de la ligne d'horloge (SCK) en mode Sortie
    PORT_POIDS->MODER &= ~(GPIO_MODER_MODE5_1);
    PORT_POIDS->MODER |=  (GPIO_MODER_MODE5_0);

    // Configuration de la ligne de données (DT) en mode Entrée
    PORT_POIDS->MODER &= ~(GPIO_MODER_MODE4_0 | GPIO_MODER_MODE4_1);
    PORT_POIDS->PUPDR |=  (GPIO_PUPDR_PUPD4_0); // Activation de la résistance de tirage (Pull-up) interne

    // Initialisation de l'horloge au niveau logique bas (État inactif)
    PORT_POIDS->BRR = (1 << PIN_SCK);
}

// Routine d'échantillonnage numérisé (24 bits) du convertisseur HX711
int32_t Poids_Lire_Unique(void) {
    uint32_t compte = 0;

    // Attente du signal Data Ready (DT au niveau bas)
    // Implémentation d'un timeout logiciel pour prévenir le verrouillage du microcontrôleur
    uint32_t timeout = 100000;
    while (PORT_POIDS->IDR & (1 << PIN_DT)) {
        timeout--;
        if(timeout == 0) return 0; // Levée d'exception : capteur non-répondant
    }

    // Extraction des 24 bits de données via décalage de registre
    for (int i = 0; i < 24; i++) {
        PORT_POIDS->BSRR = (1 << PIN_SCK); // Front montant de l'horloge
        Delai_us(1);
        compte = compte << 1;
        PORT_POIDS->BRR = (1 << PIN_SCK);  // Front descendant de l'horloge
        Delai_us(1);

        if (PORT_POIDS->IDR & (1 << PIN_DT)) {
            compte++;
        }
    }

    // 25ème impulsion d'horloge pour définir le gain à 128 sur le canal A
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
    valeur_tare = Poids_Lire_Moyenne(10);
}

int32_t Poids_Obtenir_Valeur(void) {
    return (Poids_Lire_Moyenne(3) - valeur_tare);
}