#ifndef POIDS_H
#define POIDS_H

#include <stdint.h>

// Initialise PB4 (DT) et PB5 (SCK)
void Poids_GPIO_Init(void);

// Lit la valeur brute (Moyenne de plusieurs lectures)
int32_t Poids_Lire_Moyenne(uint8_t nombre_lectures);

// Fait la tare (Mise à zéro)
void Poids_Tare(void);

// Récupère la valeur calibrée (Valeur - Tare)
int32_t Poids_Obtenir_Valeur(void);

#endif
