#include <stdint.h>
#include <stdio.h>
#include <stm32g031xx.h>
#include "main.h"
#include "timer.h"
#include "usart.h"
#include "lora.h"
#include "onewire.h"
#include "poids.h" 

/* Paramètres de calibration du système d'acquisition */
// Facteur de calibration spécifique à la balance utilisée
#define FACTEUR_CALIBRATION 10750.0f

int main(void) {
    /* 1. Initialisation des horloges et des périphériques matériels */
    SYSTICK_Init();             // Base de temps milliseconde
    Timer_Init_Microsecondes(); // Base de temps microseconde (TIM3) pour les bus de données
    UART_Init();                // Interface USART1 (Module LoRa)
    Onewire_GPIO_Init();        // Interface GPIO (Capteur DS18B20 sur PB0)
    Poids_GPIO_Init();          // Interface GPIO (Convertisseur HX711 sur PB4/PB5)

    __enable_irq(); // Activation globale des interruptions

    SYSTICK_Delay(1000); // Délai de stabilisation matériel post-initialisation

    /* 2. Procédure d'étalonnage initial */
    // La définition de la tare s'effectue avant l'initialisation réseau.
    // Pré-requis : la plateforme de pesée doit être exempte de charge.
    Poids_Tare();

    /* 3. Phase d'association réseau LoRaWAN (OTAA) */
    // L'exécution est suspendue jusqu'à la confirmation de l'attachement au réseau.
    LoRa_Setup_And_Join();

    /* Variables globales du cycle d'acquisition */
    uint8_t scratchpad[9];
    float temperature_lue = 0.0;
    int32_t poids_brut = 0;
    float poids_kg = 0.0;

    /* 4. Boucle d'exécution principale (Super-loop) */
    while(1)
    {
        /* ============================================
           A. ACQUISITION DE LA TEMPÉRATURE (DS18B20)
           ============================================ */
        if (Onewire_Reset()) {
            Onewire_WriteByte(0xCC); // Commande 'Skip ROM' (Adressage global)
            Onewire_WriteByte(0x44); // Déclenchement de la conversion thermique
            SYSTICK_Delay(800);      // Délai de traitement du capteur (résolution max)

            Onewire_Reset();
            Onewire_WriteByte(0xCC);
            Onewire_WriteByte(0xBE); // Lecture du registre Scratchpad
            Onewire_ReadBytes(scratchpad, 9);

            temperature_lue = Onewire_ConvertToCelsius(scratchpad);
        } else {
            temperature_lue = 0.0; // Valeur par défaut en cas de défaut matériel
        }

        /* ============================================
           B. ACQUISITION DU POIDS (HX711)
           ============================================ */
        // 1. Extraction de la valeur différentielle numérisée (tare déduite)
        poids_brut = Poids_Obtenir_Valeur();

        // 2. Application du ratio de calibration (Conversion en kg)
        poids_kg = (float)poids_brut / FACTEUR_CALIBRATION;

        // 3. Traitement des artéfacts négatifs résiduels (filtrage du bruit de mesure)
        if (poids_kg < 0.0f) poids_kg = 0.0f;

        /* ============================================
           C. FORMATAGE DE LA TRAME DE DONNÉES (PAYLOAD)
           ============================================ */
        // Représentation en virgule fixe (Fixed-point) pour optimiser le transfert radio
        // Multiplication par 10^2 pour conserver deux chiffres significatifs
        int16_t temp_int = (int16_t)(temperature_lue * 100);
        int16_t poids_int = (int16_t)(poids_kg * 100);

        char message_hex[20];
        // Encodage hexadécimal : 4 octets pour la température, 4 octets pour le poids
        sprintf(message_hex, "%04X%04X", temp_int, poids_int);

        /* ============================================
           D. TRANSMISSION RÉSEAU ET MISE EN ATTENTE
           ============================================ */
        LoRa_Envoyer_Message(message_hex);

        // Délai inter-mesure (Attente active). 
        // Valeur de test : 30s. Spécification de production : 300s (5 min).
        SYSTICK_Delay(30000);
    }
}