#ifndef LORA_H
#define LORA_H

// Configure le module et rejoint le réseau (Bloquant)
void LoRa_Setup_And_Join(void);

// Envoie un message
void LoRa_Send_Data(const char* message_hex);

#endif
