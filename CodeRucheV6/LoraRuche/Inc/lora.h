#ifndef LORA_H
#define LORA_H
#include <stdint.h>

// Configure le module et rejoint le réseau (Bloquant)
void LoRa_Setup_And_Join(void);

// Envoie un message

// Change uint32_t en void
void LoRa_Envoyer_Message(const char* message_hex);

#endif
