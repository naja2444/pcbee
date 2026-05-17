#include "lora.h"
#include <stdio.h>
#include <string.h>
#include <stm32g031xx.h>

/* Déclarations externes (cf. main.c) */
extern void UART_envoie_chaine(const char* str);
extern void UART_LireReponse_AvecDelai(char* buffer, int buffer_taille, uint32_t timeout_ms);

/* Variables privées d'interface USART */
char rx_buffer_lora[128]; // Buffer de réception
char cmd_buffer[128];     // Buffer de formatage des commandes (sprintf)

/* Identifiants The Things Network (TTN) - OTAA */
#define LORA_APP_EUI "526973696E674846"
#define LORA_APP_KEY "73B648C0D9EC9A79DC7C23A7EFC2A1F0"
#define LORA_DEV_EUI "2CF7F120510013D4"

/* Fonctions utilitaires */

// Évalue la présence d'une erreur dans la trame de retour du module
static int check_erreur(const char* buffer) {
    if (strstr(buffer, "ERROR") != NULL) {
        return 1;
    }
    return 0;
}

/* Fonctions de gestion LoRaWAN */

void LoRa_Setup_And_Join(void) {
    /* 1. Configuration des paramètres réseau (Délais d'exécution optimisés) */
    UART_envoie_chaine("AT+MODE=LWOTAA\r\n");
    // Délai réduit à 300ms pour l'optimisation de la séquence d'initialisation
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } // Interruption logicielle en cas d'échec matériel

    UART_envoie_chaine("AT+DR=EU868\r\n");
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } 

    UART_envoie_chaine("AT+CLASS=A\r\n");
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } 

    /* 2. Injection des identifiants cryptographiques (OTAA) */
    sprintf(cmd_buffer, "AT+ID=AppEui,%s\r\n", LORA_APP_EUI);
    UART_envoie_chaine(cmd_buffer);
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } 

    sprintf(cmd_buffer, "AT+KEY=AppKey,%s\r\n", LORA_APP_KEY);
    UART_envoie_chaine(cmd_buffer);
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } 

    sprintf(cmd_buffer, "AT+ID=DevEui,%s\r\n", LORA_DEV_EUI);
    UART_envoie_chaine(cmd_buffer);
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } 

    /* 3. Procédure d'attachement au réseau (Join Request) */
    UART_envoie_chaine("AT+JOIN\r\n");

    // Délai étendu pour autoriser la réception de l'acquittement de la passerelle
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 15000);

    // Vérification de la complétion du processus d'attachement
    if (strstr(rx_buffer_lora, "Network joined") == NULL) {
        // Échec d'attachement, maintien du microcontrôleur en boucle de sécurité
        while(1); 
    }
}

void LoRa_Envoyer_Message(const char* message_hex) {
    /* 4. Transmission de la trame de données (Uplink) */
    // Formatage de la commande d'envoi en représentation hexadécimale
    sprintf(cmd_buffer, "AT+CMSGHEX=\"%s\"\r\n", message_hex);
    UART_envoie_chaine(cmd_buffer);

    // Attente de l'acquittement de transmission du module radio (Timeout: 15s)
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 15000); 
}