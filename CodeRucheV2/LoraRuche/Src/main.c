#include <stdint.h>
#include <stdio.h>
#include <stm32g031xx.h>
#include "main.h"
#include "timer.h"
#include "usart.h"
#include "lora.h"
#include "onewire.h"
#include "poids.h" 

/* Paramètres matériels de calibration */
// Constante d'étalonnage linéaire pour le pont de jauge de contrainte
#define FACTEUR_CALIBRATION 10750.0f

int main(void) {
    /* 1. Initialisation de la couche d'abstraction matérielle (HAL / Registres) */
    SYSTICK_Init();             // Base de temps système (résolution milliseconde)
    Timer_Init_Microsecondes(); // Timer matériel 3 (résolution microseconde) pour les bus synchrones
    UART_Init();                // Interface USART1 (Communication Modem LoRa)
    Onewire_GPIO_Init();        // Interface GPIO (Capteur DS18B20 - Broche PB0)
    Poids_GPIO_Init();          // Interface GPIO (Convertisseur HX711 - Broches PB4/PB5)
    
    __enable_irq();             // Activation du contrôleur d'interruptions vectorielles (NVIC)
    SYSTICK_Delay(1000);        // Délai de stabilisation transitoire des alimentations

    /* 2. Séquence d'étalonnage initial (Tare) */
    // L'acquisition de la ligne de base (zéro) précède la phase de connexion réseau.
    Poids_Tare();

    /* 3. Établissement de la liaison réseau LoRaWAN */
    // Routine bloquante : le flux d'exécution est suspendu dans l'attente d'un Join Accept.
    LoRa_Setup_And_Join();

    /* Allocation des variables de traitement du signal */
    uint8_t tempo[9];
    float temperature_lue = 0.0;
    int32_t poids_brut = 0;
    float poids_kg = 0.0;

    /* 4. Boucle de supervision principale (Super-loop) */
    while(1)
    {
        /* ============================================
           A. ACQUISITION THERMIQUE (BUS ONEWIRE / DS18B20)
           ============================================ */
        if (Onewire_Reset()) {
            Onewire_WriteByte(0xCC); // Commande "Skip ROM" (Adressage global sans identification)
            Onewire_WriteByte(0x44); // Commande "Convert T" (Déclenchement de la conversion A/N)
            SYSTICK_Delay(800);      // Délai de traitement du capteur (temps de conversion max 750ms)

            Onewire_Reset();
            Onewire_WriteByte(0xCC); // Séquence d'adressage global
            Onewire_WriteByte(0xBE); // Commande "Read Scratchpad" (Lecture de la RAM interne)
            Onewire_ReadBytes(tempo, 9); // Transfert de la trame de données

            temperature_lue = Onewire_ConvertToCelsius(tempo);
        } else {
            temperature_lue = 0.0; // Fallback en cas de défaut d'acquittement matériel
        }

        /* ============================================
           B. ACQUISITION MASSIQUE (CONVERTISSEUR HX711)
           ============================================ */
        // 1. Extraction de la valeur différentielle numérisée (offset de tare déduit)
        poids_brut = Poids_Obtenir_Valeur();

        // 2. Application du ratio de conversion analogique/numérique (Unités absolues)
        poids_kg = (float)poids_brut / FACTEUR_CALIBRATION;

        // 3. Filtrage logiciel des artéfacts négatifs résiduels (bruit de quantification)
        if (poids_kg < 0.0f) poids_kg = 0.0f;

        /* ============================================
           C. FORMATAGE DE LA CHARGE UTILE (PAYLOAD LORA)
           ============================================ */
        // Conversion en virgule fixe (Fixed-point) pour optimiser l'encombrement spectral
        // Multiplication par 10^4 pour garantir une résolution de 4 décimales significatives
        int32_t temp_int = (int32_t)(temperature_lue * 10000);
        int32_t poids_int = (int32_t)(poids_kg * 10000);

        char message_hex[32];
        // Encodage hexadécimal : 8 caractères (4 octets) pour la température, 8 caractères pour le poids
        sprintf(message_hex, "%08lX%08lX", temp_int, poids_int);

        /* ============================================
           D. TRANSMISSION RÉSEAU ET ORDONNANCEMENT
           ============================================ */
        LoRa_Envoyer_Message(message_hex);

        // Délai inter-cycle d'acquisition.
        // Valeur de test : 30 secondes (30000 ms).
        // Spécification de production recommandée : 1 heure (3600000 ms).
        SYSTICK_Delay(30000);
    }
}