#include "onewire.h"
#include "timer.h" 
#include <stm32g031xx.h>

/* Assignation matérielle du bus OneWire (Port B, Broche 0) */
#define PORT_ONEWIRE GPIOB
#define PIN_ONEWIRE  0

/* Fonctions privées d'interface GPIO */

void Onewire_GPIO_Init(void) {
    // Activation de l'horloge du périphérique GPIOB
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
}

// Configuration de la broche en mode Sortie (topologie Open-Drain)
static void Mode_Sortie(void) {
    PORT_ONEWIRE->MODER &= ~(GPIO_MODER_MODE0_1); // Effacement du bit de mode supérieur
    PORT_ONEWIRE->MODER |=  (GPIO_MODER_MODE0_0); // Définition du mode de sortie (01)
    PORT_ONEWIRE->OTYPER |= (1 << PIN_ONEWIRE);   // Activation du mode Open-Drain
}

// Configuration de la broche en mode Entrée (Haute impédance)
static void Mode_Entree(void) {
    PORT_ONEWIRE->MODER &= ~(GPIO_MODER_MODE0_0 | GPIO_MODER_MODE0_1); // Définition du mode d'entrée (00)
}

/* Implémentation du protocole OneWire (Couche Physique) */

// Transmission d'un niveau logique unitaire
static void Onewire_WriteBit(uint8_t bit) {
    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Transition descendante (Low)

    if (bit) {
        // Séquence de transmission d'un bit logique '1'
        Delai_us(6);       // Maintien à l'état bas (durée minimale)
        Mode_Entree();     // Relâchement du bus
        Delai_us(64);      // Temporisation de fin de créneau temporel
    } else {
        // Séquence de transmission d'un bit logique '0'
        Delai_us(60);      // Maintien à l'état bas (dominance du signal)
        Mode_Entree();     // Relâchement du bus
        Delai_us(10);
    }
}

// Échantillonnage d'un niveau logique unitaire
static uint8_t Onewire_ReadBit(void) {
    uint8_t bit_lu = 0;

    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Impulsion de synchronisation
    Delai_us(6);

    Mode_Entree(); // Relâchement pour autoriser la transmission de l'esclave
    Delai_us(9);   // Délai de stabilisation du signal

    // Échantillonnage de l'état du registre d'entrée (IDR)
    if (PORT_ONEWIRE->IDR & (1 << PIN_ONEWIRE)) {
        bit_lu = 1;
    }

    Delai_us(55); // Attente de la fin de la fenêtre de lecture
    return bit_lu;
}

/* Fonctions publiques (Couche Application) */

uint8_t Onewire_Reset(void) {
    uint8_t presence = 0;

    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Transition descendante
    Delai_us(480); // Séquence Reset (Maintien de 480µs)

    Mode_Entree(); // Relâchement du bus
    Delai_us(70);  // Délai d'attente du signal de présence esclave

    // Évaluation du signal de présence émis par l'esclave
    if (!(PORT_ONEWIRE->IDR & (1 << PIN_ONEWIRE))) {
        presence = 1;
    }

    Delai_us(410); // Fin du cycle de synchronisation initial
    return presence;
}

void Onewire_WriteByte(uint8_t octet) {
    for (int i = 0; i < 8; i++) {
        // Transmission sérielle (LSB First)
        Onewire_WriteBit(octet & 0x01);
        octet >>= 1; // Décalage logique à droite
    }
}

uint8_t Onewire_ReadByte(void) {
    uint8_t octet = 0;
    for (int i = 0; i < 8; i++) {
        octet >>= 1;
        if (Onewire_ReadBit()) {
            octet |= 0x80; // Reconstruction de l'octet (MSB assigné via LSB initial)
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
    // Extraction des 16 bits de la mesure thermométrique brute
    int16_t temp_brute = (donnees[1] << 8) | donnees[0];
    // Conversion tenant compte de la résolution matérielle du capteur (0.0625°C par bit)
    return (float)temp_brute / 16.0f;
}