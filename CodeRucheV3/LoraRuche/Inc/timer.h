

#ifndef TIMER_H_
#define TIMER_H_

void SYSTICK_Init(void);
void SYSTICK_Delay(uint32_t Delay);
uint32_t SYSTICK_Get(void);

// Initialise le timer pour compter les microsecondes
void Timer_Init_Microsecondes(void);

// Attend un nombre précis de microsecondes (blocant)
void Delai_us(uint32_t us);

void Dormir_X_Secondes_Stop_Mode(uint32_t secondes);

#endif
