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

/* Constante d'étalonnage linéaire obsolète (Remplacée par l'approximation polynomiale absolue) */

// Variable d'état maintenue volatile pour supervision externe via interface de débogage (Live Expressions)
volatile int32_t poids_brut = 0;

int main(void) {
    /* 1. INITIALISATION DE LA COUCHE MATÉRIELLE (HAL) */
    SYSTICK_Init();             // Base de temps système (résolution milliseconde)
    Timer_Init_Microsecondes(); // Timer matériel 3 (résolution microseconde) pour bus synchrones
    UART_Init();                // Interface USART1 (Communication Modem LoRa)
    Onewire_GPIO_Init();        // Interface GPIO (Capteur DS18B20)
    Poids_GPIO_Init();          // Interface GPIO (Convertisseur HX711)

    // Configuration du registre GPIO pour le témoin d'activité (LED PC6)
    RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODE6_Msk;
    GPIOC->MODER |= (1 << GPIO_MODER_MODE6_Pos); // Assignation en mode Sortie

    __enable_irq(); // Activation globale du contrôleur d'interruptions vectorielles (NVIC)

    // Séquence de confirmation d'initialisation (Feedback visuel transitoire)
    for(int i=0; i<2; i++) {
        GPIOC->ODR |= (1 << 6); SYSTICK_Delay(200);
        GPIOC->ODR &= ~(1 << 6); SYSTICK_Delay(200);
    }

    /* 2. SÉQUENCE D'ÉTALONNAGE ET D'ASSOCIATION RÉSEAU */
    // Poids_Initialiser_Tare(); // Note d'architecture : Désactivé. La compensation d'offset (tare) est 
                                 // intrinsèquement gérée par la constante 'c' du polynôme de calibration.
    LoRa_Setup_And_Join();

    /* Allocation des variables d'état et de traitement */
    uint8_t tempo[9];
    float temperature_lue = 0.0;
    volatile float poids_kg = 0.0;
    char message_hex[32];
    char debug_buffer[80];

    while(1)
        {
            /* ============================================
               PHASE DE RÉVEIL OPÉRATIONNEL (WAKE-UP)
               ============================================ */
            // Réactivation du convertisseur A/N HX711 (Transition de la broche SCK à l'état logique bas sur PA15)
            GPIOA->ODR &= ~(1 << 15); 
            
            // Interruption du mode basse consommation du modem LoRa via transmission série (Dummy signal)
            UART_envoie_chaine("AT\r\n"); 
            SYSTICK_Delay(100);       // Délai de rétablissement de l'oscillateur du modem radio
            GPIOC->ODR |= (1 << 6);   // Activation du témoin de cycle (LED ON)

            /* ============================================
               A. ACQUISITION THERMIQUE (BUS ONEWIRE / DS18B20)
               ============================================ */
            if (Onewire_Reset()) {
                Onewire_WriteByte(0xCC); Onewire_WriteByte(0x44);
                SYSTICK_Delay(800); // Temporisation de la conversion thermique
                Onewire_Reset(); Onewire_WriteByte(0xCC); Onewire_WriteByte(0xBE);
                Onewire_ReadBytes(tempo, 9);
                temperature_lue = Onewire_ConvertToCelsius(tempo);
            }

            /* ============================================
               B. ACQUISITION MASSIQUE (ALGORITHME POLYNOMIAL)
               ============================================ */
            // 1. Extraction de la valeur absolue non compensée en sortie du pont de jauge
            poids_brut = Poids_Lire_Brut_Pur();

            // 2. Paramétrage des coefficients de l'approximation polynomiale (issus de l'étalonnage empirique)
            double a = -6.76251e-12; // Coefficient quadratique (Lissage de la non-linéarité matérielle)
            double b = 9.74269e-05;  // Coefficient directeur (Pente de conversion analogique/numérique)
            double c = -8.19810;     // Constante d'offset (Gestion logicielle du zéro / Tare absolue)

            // 3. Traitement de la mesure via l'équation polynomiale du second degré (y = ax² + bx + c)
            double x_double = (double)poids_brut;
            double resultat_poids = (a * x_double * x_double) + (b * x_double) + c;

            // 4. Transtypage et filtrage logiciel des artéfacts négatifs résiduels
            poids_kg = (float)resultat_poids;
            if (poids_kg < 0.0f) {
                poids_kg = 0.0f;
            }

            /* ============================================
               C. FORMATAGE DE LA CHARGE UTILE (PAYLOAD LORA)
               ============================================ */
            int32_t temp_int = (int32_t)(temperature_lue * 10000);
            int32_t poids_int = (int32_t)(poids_kg * 10000);
            // Encodage hexadécimal des variables d'état (8 octets ASCII cumulés)
            sprintf(message_hex, "%08lX%08lX", temp_int, poids_int);

            // Transmission réseau unidirectionnelle (Fire and Forget)
            LoRa_Envoyer_Message(message_hex);

            /* ============================================
               PHASE DE MISE EN VEILLE (POWER-DOWN SEQUENCE)
               ============================================ */
            GPIOC->ODR &= ~(1 << 6); // Désactivation du témoin de cycle

            // 1. Extinction matérielle du HX711 (Maintien de l'horloge SCK à l'état haut sur PA15)
            GPIOA->ODR |= (1 << 15);

            // 2. Forçage du modem radio en mode d'économie d'énergie extrême (Low Power State)
            UART_envoie_chaine("AT+LOWPOWER\r\n");
            SYSTICK_Delay(50); // Délai de garantie de transmission du buffer série

            // 3. PRÉVENTION DES COURANTS DE FUITE (Hardware Leakage Mitigation) :
            // Reconfiguration de la broche PB0 (Bus OneWire) en Haute Impédance (Input) pour couper le pull-up
            GPIOB->MODER &= ~(3 << (0 * 2)); // Assignation du registre de mode en 00 (Input)

            // 4. Transition du microcontrôleur en mode Stop 1
            Dormir_X_Secondes_Stop_Mode(30);

            /* ============================================
               RÉTABLISSEMENT POST-RÉVEIL
               ============================================ */
            // Note : L'interface OneWire (PB0) sera automatiquement réinitialisée en mode Open-Drain 
            // via les fonctions Onewire_GPIO_Init() et Mode_Sortie() au cycle suivant.
            UART_Init(); // Réinitialisation indispensable du contrôleur matériel USART
        }
}