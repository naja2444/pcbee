#ifndef LORA_H
#define LORA_H
#include <stdint.h>

// Configure le module et rejoint le réseau (Bloquant)
void LoRa_Setup_And_Join(void);

// Envoie un message
void LoRa_Send_Data(const char* message_hex);
uint32_t LoRa_Envoyer_Message(const char* message_hex);

#endif
