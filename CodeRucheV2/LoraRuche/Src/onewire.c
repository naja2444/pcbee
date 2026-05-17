#include "onewire.h"
#include "timer.h" 
#include <stm32g031xx.h>

/* Assignation matérielle de l'interface OneWire */
#define PORT_ONEWIRE GPIOB
#define PIN_ONEWIRE  0

/* Fonctions privées de configuration matérielle (Couche Physique) */

void Onewire_GPIO_Init(void) {
    // Activation de l'horloge du périphérique GPIOB
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
}

// Configuration de la broche en mode Sortie avec topologie Open-Drain
static void Mode_Sortie(void) {
    PORT_ONEWIRE->MODER &= ~(GPIO_MODER_MODE0_1); // Effacement du bit de mode supérieur
    PORT_ONEWIRE->MODER |=  (GPIO_MODER_MODE0_0); // Définition du mode de sortie (01)
    PORT_ONEWIRE->OTYPER |= (1 << PIN_ONEWIRE);   // Activation du drain ouvert (Open-Drain)
}

// Configuration de la broche en mode Entrée (Haute impédance)
static void Mode_Entree(void) {
    PORT_ONEWIRE->MODER &= ~(GPIO_MODER_MODE0_0 | GPIO_MODER_MODE0_1); // Réinitialisation du registre de mode (00)
}

/* Implémentation du chronogramme OneWire (Gestion des créneaux temporels) */

// Transmission sérielle d'un niveau logique unitaire
static void Onewire_WriteBit(uint8_t bit) {
    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Forçage de la ligne à l'état logique bas

    if (bit) {
        // Séquence de génération d'un bit logique '1'
        Delai_us(6);       // Impulsion de synchronisation minimale
        Mode_Entree();     // Libération du bus (traction au niveau haut par résistance externe)
        Delai_us(64);      // Temporisation de fin de créneau temporel (Time Slot)
    } else {
        // Séquence de génération d'un bit logique '0'
        Delai_us(60);      // Maintien prolongé à l'état bas (dominance du signal)
        Mode_Entree();     // Libération du bus
        Delai_us(10);      // Délai de recouvrement
    }
}

// Échantillonnage d'un niveau logique unitaire émis par le périphérique esclave
static uint8_t Onewire_ReadBit(void) {
    uint8_t bit_lu = 0;

    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Génération du signal de synchronisation initiateur (Master Sync)
    Delai_us(6);

    Mode_Entree(); // Transition en haute impédance pour autoriser la transmission esclave
    Delai_us(9);   // Délai de propagation et de stabilisation du signal

    // Échantillonnage de l'état du registre de données d'entrée (IDR)
    if (PORT_ONEWIRE->IDR & (1 << PIN_ONEWIRE)) {
        bit_lu = 1;
    }

    Delai_us(55); // Attente de la complétion du créneau de lecture (Read Time Slot)
    return bit_lu;
}

/* Fonctions publiques d'interface de bus (Couche Liaison de Données) */

uint8_t Onewire_Reset(void) {
    uint8_t presence = 0;

    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Forçage du bus à l'état bas
    Delai_us(480); // Génération du signal de réinitialisation (Reset Pulse : 480µs min)

    Mode_Entree(); // Libération du bus
    Delai_us(70);  // Temporisation d'attente du signal de présence (Presence Pulse)

    // Évaluation de la réponse de l'esclave (Acknowledge)
    if (!(PORT_ONEWIRE->IDR & (1 << PIN_ONEWIRE))) {
        presence = 1;
    }

    Delai_us(410); // Clôture du cycle de synchronisation d'initialisation
    return presence;
}

void Onewire_WriteByte(uint8_t octet) {
    for (int i = 0; i < 8; i++) {
        // Séquentialisation de la transmission (LSB First selon la norme Dallas/Maxim)
        Onewire_WriteBit(octet & 0x01);
        octet >>= 1; // Décalage logique à droite
    }
}

uint8_t Onewire_ReadByte(void) {
    uint8_t octet = 0;
    for (int i = 0; i < 8; i++) {
        octet >>= 1;
        if (Onewire_ReadBit()) {
            octet |= 0x80; // Reconstruction de l'octet via assignation du MSB
        }
    }
    return octet;
}

void Onewire_ReadBytes(uint8_t *buffer, uint8_t longueur) {
    for (uint8_t i = 0; i < longueur; i++) {
        buffer[i] = Onewire_ReadByte();
    }
}

float Onewire_ConvertToCelsius(uint8_t *donnees) {
    // Reconstruction du mot de 16 bits contenant la donnée thermique brute
    int16_t temp_brute = (donnees[1] << 8) | donnees[0];
    // Conversion tenant compte du pas de quantification matériel (0.0625°C par bit en résolution 12-bit)
    return (float)temp_brute / 16.0f;
}