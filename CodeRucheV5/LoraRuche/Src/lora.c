#include "lora.h"
#include <stdio.h>
#include <string.h>
#include <stm32g031xx.h>

/* Déclarations externes (Interface avec l'application principale) */
extern void UART_envoie_chaine(const char* str);
extern void UART_LireReponse_AvecDelai(char* buffer, int buffer_taille, uint32_t timeout_ms);

/* Allocation des tampons de communication (Buffers USART) */
char rx_buffer_lora[256]; // Tampon de réception série (Rx)
char cmd_buffer[128];     // Tampon de formatage des commandes AT (Tx)

/* Identifiants d'approvisionnement réseau LoRaWAN (TTN - Méthode OTAA) */
#define LORA_APP_EUI "526973696E674846"
#define LORA_APP_KEY "73B648C0D9EC9A79DC7C23A7EFC2A1F0"
#define LORA_DEV_EUI "2CF7F120510013D4"

/* Fonctions utilitaires privées */
static int check_erreur(const char* buffer) {
    if (strstr(buffer, "ERROR") != NULL) {
        return 1;
    }
    return 0;
}

/* Fonctions publiques d'interface LoRaWAN */

void LoRa_Setup_And_Join(void) {
    /* 1. Configuration des paramètres physiques MAC (Délais de scrutation optimisés) */
    UART_envoie_chaine("AT+MODE=LWOTAA\r\n");
    // Configuration du délai d'attente de réponse du modem (Timeout réduit à 300ms)
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); } // Exception matérielle : Interruption du flux d'exécution

    UART_envoie_chaine("AT+DR=EU868\r\n");
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); }

    UART_envoie_chaine("AT+CLASS=A\r\n");
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 300);
    if (check_erreur(rx_buffer_lora)) { while(1); }

    /* 2. Injection des clés cryptographiques de session (OTAA) */
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

    /* 3. Initialisation de la procédure d'attachement au réseau (Join Request) */
    UART_envoie_chaine("AT+JOIN\r\n");

    // Allocation d'un délai étendu pour la réception de l'acquittement de la passerelle (Join Accept)
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 15000);

    // Validation de l'état d'association réseau
    if (strstr(rx_buffer_lora, "Network joined") == NULL) {
        // Échec d'attachement : verrouillage de sécurité de l'application (Fallback state)
        while(1); 
    }
}

/* 4. Séquence de transmission aveugle (Uplink Only - Zéro Downlink) */
void LoRa_Envoyer_Message(const char* message_hex) {
    // 1. Purge du tampon de réception pour éviter toute corruption de mémoire
    memset(rx_buffer_lora, 0, sizeof(rx_buffer_lora));

    // 2. Encapsulation et transmission de la charge utile (Payload)
    sprintf(cmd_buffer, "AT+CMSGHEX=\"%s\"\r\n", message_hex);
    UART_envoie_chaine(cmd_buffer);

    // 3. Attente stricte de l'acquittement de transmission (Done) sans scrutation des fenêtres RX (Timeout: 2s)
    UART_LireReponse_AvecDelai(rx_buffer_lora, sizeof(rx_buffer_lora), 2000);
}