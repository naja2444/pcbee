#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <stdint.h>

// Initialise la broche PB0 pour le capteur
void Onewire_GPIO_Init(void);

// Réinitialise le bus et vérifie si un capteur est présent
// Retourne : 1 si présent, 0 si absent
uint8_t Onewire_Reset(void);

// Écrit un octet (8 bits) vers le capteur
void Onewire_WriteByte(uint8_t octet);

// Lit un octet (8 bits) depuis le capteur
uint8_t Onewire_ReadByte(void);

// Lit plusieurs octets d'un coup
void Onewire_ReadBytes(uint8_t *buffer, uint8_t longueur);

// Convertit les données brutes du capteur en température Celsius (float)
float Onewire_ConvertToCelsius(uint8_t *donnees);

#endif
