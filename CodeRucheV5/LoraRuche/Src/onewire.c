#include "onewire.h"
#include "timer.h" // Importation de la base de temps haute résolution (Delai_us)
#include <stm32g031xx.h>

/* Assignation matérielle de l'interface OneWire (Port B, Broche 0) */
#define PORT_ONEWIRE GPIOB
#define PIN_ONEWIRE  0

/* Fonctions privées de configuration matérielle (Couche Physique) */

void Onewire_GPIO_Init(void) {
    // Activation de l'arbre d'horloge du périphérique de port
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
}

// Configuration de la ligne de données en mode Sortie (Topologie Open-Drain)
static void Mode_Sortie(void) {
    PORT_ONEWIRE->MODER &= ~(GPIO_MODER_MODE0_1); // Effacement du bit supérieur de configuration
    PORT_ONEWIRE->MODER |=  (GPIO_MODER_MODE0_0); // Assignation du mode de sortie (01)
    PORT_ONEWIRE->OTYPER |= (1 << PIN_ONEWIRE);   // Activation du drain ouvert (protection électrique)
}

// Configuration de la ligne de données en mode Entrée (Haute Impédance)
static void Mode_Entree(void) {
    PORT_ONEWIRE->MODER &= ~(GPIO_MODER_MODE0_0 | GPIO_MODER_MODE0_1); // Assignation du mode entrée (00)
}

/* Implémentation du chronogramme OneWire (Séquençage temporel) */

// Émission d'un état logique unitaire sur le bus de données
static void Onewire_WriteBit(uint8_t bit) {
    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Forçage du signal à l'état logique bas (Génération de front)

    if (bit) {
        // Profil temporel de transmission d'un bit logique '1'
        Delai_us(6);       // Impulsion d'initialisation de séquence
        Mode_Entree();     // Transition en haute impédance (traction au niveau haut par résistance externe)
        Delai_us(64);      // Temporisation de clôture du créneau de transmission (Time Slot)
    } else {
        // Profil temporel de transmission d'un bit logique '0'
        Delai_us(60);      // Forçage prolongé à l'état bas (dominance du signal sur le bus)
        Mode_Entree();     // Libération de la ligne
        Delai_us(10);      // Délai de recouvrement capacitif
    }
}

// Échantillonnage d'un état logique unitaire émis par le périphérique cible
static uint8_t Onewire_ReadBit(void) {
    uint8_t bit_lu = 0;

    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Impulsion de synchronisation initiateur (Master Sync)
    Delai_us(6);

    Mode_Entree(); // Relâchement de la ligne pour autoriser le multiplexage temporel de l'esclave
    Delai_us(9);   // Temporisation de propagation et de stabilisation du signal

    // Échantillonnage de l'état du registre de données d'entrée matérielle
    if (PORT_ONEWIRE->IDR & (1 << PIN_ONEWIRE)) {
        bit_lu = 1;
    }

    Delai_us(55); // Attente de l'expiration du créneau de lecture (Read Time Slot)
    return bit_lu;
}

/* Fonctions publiques d'interface de bus (Couche Application) */

uint8_t Onewire_Reset(void) {
    uint8_t presence = 0;

    Mode_Sortie();
    PORT_ONEWIRE->ODR &= ~(1 << PIN_ONEWIRE); // Forçage du bus à l'état bas
    Delai_us(480); // Génération de l'impulsion de réinitialisation (Reset Pulse : 480µs)

    Mode_Entree(); // Libération du bus
    Delai_us(70);  // Temporisation d'attente du signal de présence (Presence Pulse)

    // Évaluation de l'impulsion de présence (Acknowledge matériel de l'esclave)
    if (!(PORT_ONEWIRE->IDR & (1 << PIN_ONEWIRE))) {
        presence = 1;
    }

    Delai_us(410); // Clôture de la séquence de synchronisation d'initialisation
    return presence;
}

void Onewire_WriteByte(uint8_t octet) {
    for (int i = 0; i < 8; i++) {
        // Sérialisation LSB-First (Convention protocolaire Maxim/Dallas)
        Onewire_WriteBit(octet & 0x01);
        octet >>= 1; // Décalage logique de registre
    }
}

uint8_t Onewire_ReadByte(void) {
    uint8_t octet = 0;
    for (int i = 0; i < 8; i++) {
        octet >>= 1;
        if (Onewire_ReadBit()) {
            octet |= 0x80; // Reconstruction de l'octet via assignation du bit de poids fort (MSB)
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
    // Concaténation des registres LSB et MSB pour recomposer la donnée thermique brute
    int16_t temp_brute = (donnees[1] << 8) | donnees[0];
    // Application de la constante de résolution matérielle (0.0625°C par bit pour l'ADC interne en 12-bit)
    return (float)temp_brute / 16.0f;
}