#include <stdint.h>
#include <stdio.h>
#include <stm32g031xx.h>
#include "main.h"
#include "timer.h"
#include "usart.h"
#include "lora.h"
#include "onewire.h"
#include "poids.h"
#include "gpio.h"

/* Paramètres matériels de calibration (Pont de jauge) */
#define FACTEUR_CALIBRATION 10764.17f

int main(void) {
    /* 1. INITIALISATION DE LA COUCHE MATÉRIELLE (HAL) */
    SYSTICK_Init();             // Base de temps système (milliseconde)
    Timer_Init_Microsecondes(); // Timer matériel 3 (microseconde) pour bus synchrones
    UART_Init();                // Interface USART1 (Modem LoRa)
    Onewire_GPIO_Init();        // Interface GPIO (DS18B20 - PB0)
    Poids_GPIO_Init();          // Interface GPIO (HX711 - PB4/PB5)

    // Initialisation du registre GPIO pour le témoin d'activité (LED PC6)
    RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODE6_Msk;
    GPIOC->MODER |= (1 << GPIO_MODER_MODE6_Pos); // Configuration en mode Sortie

    __enable_irq(); // Activation globale du contrôleur d'interruptions (NVIC)

    // Séquence de confirmation d'initialisation (Feedback visuel)
    for(int i=0; i<2; i++) {
        GPIOC->ODR |= (1 << 6); SYSTICK_Delay(200);
        GPIOC->ODR &= ~(1 << 6); SYSTICK_Delay(200);
    }

    /* 2. SÉQUENCE D'ÉTALONNAGE ET D'ASSOCIATION RÉSEAU */
    Poids_Tare();
    LoRa_Setup_And_Join();

    /* Allocation des variables d'état et de traitement */
    uint8_t tempo[9];
    float temperature_lue = 0.0;
    int32_t poids_brut = 0;
    float poids_kg = 0.0;
    char message_hex[32];
    uint32_t temps_sommeil = 30; // Période de cycle par défaut (en secondes)

    while(1)
        {
            /* ============================================
               PHASE DE RÉVEIL OPÉRATIONNEL (WAKE-UP)
               ============================================ */

            // 1. Réactivation du convertisseur A/N HX711 (Transition de la broche SCK à l'état logique bas)
            GPIOB->ODR &= ~(1 << 5);

            // 2. Interruption du mode Sleep du modem LoRa via transmission série (Dummy data)
            UART_envoie_chaine("AT\r\n");
            SYSTICK_Delay(100); // Délai de rétablissement de l'oscillateur du modem

            // 3. Activation du témoin de cycle
            GPIOC->ODR |= (1 << 6);

            /* ============================================
               A. ACQUISITION DES DONNÉES CAPTEURS
               ============================================ */
            if (Onewire_Reset()) {
                Onewire_WriteByte(0xCC); Onewire_WriteByte(0x44);
                SYSTICK_Delay(800); // Délai de conversion thermique (12-bit resolution)
                Onewire_Reset(); Onewire_WriteByte(0xCC); Onewire_WriteByte(0xBE);
                Onewire_ReadBytes(tempo, 9);
                temperature_lue = Onewire_ConvertToCelsius(tempo);
            }

            poids_brut = Poids_Obtenir_Valeur();
            poids_kg = (float)poids_brut / FACTEUR_CALIBRATION;
            if (poids_kg < 0.0f) poids_kg = 0.0f; // Filtrage du bruit de quantification autour du zéro

            /* ============================================
               B. TRANSMISSION (UPLINK) ET RECONFIGURATION (DOWNLINK)
               ============================================ */
            int32_t temp_int = (int32_t)(temperature_lue * 10000);
            int32_t poids_int = (int32_t)(poids_kg * 10000);
            // Formatage de la charge utile : 8 caractères hexadécimaux par métrique
            sprintf(message_hex, "%08lX%08lX", temp_int, poids_int);

            // Émission de la trame et traitement d'un éventuel retour de configuration
            uint32_t nouveau_temps = LoRa_Envoyer_Message(message_hex);

            // Application de la nouvelle consigne temporelle issue de la liaison descendante
            if (nouveau_temps > 0) {
            	temps_sommeil = nouveau_temps;
            }

            /* ============================================
               PHASE DE MISE EN VEILLE (POWER-DOWN SEQUENCE)
               ============================================ */

            // 1. Désactivation du témoin de cycle
            GPIOC->ODR &= ~(1 << 6);

            // 2. Extinction matérielle du HX711 (Maintien de l'horloge SCK à l'état haut > 60µs)
            GPIOB->ODR |= (1 << 5);

            // 3. Mise en sommeil profond du modem radio
            UART_envoie_chaine("AT+SLEEP\r\n");
            // Délai de garantie de transmission du buffer série avant coupure des horloges MCU
            SYSTICK_Delay(10000);

            // 4. Transition du microcontrôleur en mode d'économie d'énergie (Stop 1)
            Dormir_X_Secondes_Stop_Mode(temps_sommeil);
            
            // Post-réveil : Réinitialisation du contrôleur USART pour purger les registres d'erreurs éventuels
            UART_Init();
        }
}