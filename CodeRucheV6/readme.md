# Système Connecté pour Ruche (V5) - IoT LoRaWAN, Autonomie Extrême & Version Finale 🐝🚀

Ce projet est un système embarqué autonome conçu pour surveiller les constantes vitales d'une ruche (poids et température) et transmettre ces données via le réseau LoRaWAN. Basée sur un microcontrôleur STM32G031, cette **Version 5** représente l'aboutissement du code pour un déploiement en production, combinant une calibration mathématique absolue, des délais de communication drastiquement réduits, et une protection maximale contre les crashs matériels.

## 🌟 Nouveautés et Optimisations de la V5 (Production-Ready)

La V5 compile toutes les sécurités des versions précédentes en poussant l'optimisation énergétique à son maximum :

* **Envoi "Aveugle" (Zéro Downlink) :** La fonction `LoRa_Envoyer_Message` a été re-conçue pour effectuer un "envoi unique sans lecture de downlink". Le système n'attend plus que la simple confirmation de transmission (`Done`) pendant 2 secondes maximum avant de se rendormir, ce qui coupe radicalement la consommation radio.
* **Démarrage TTN Ultra-Rapide :** Les temps d'attente (timeouts) lors de la configuration du module LoRa (AT+MODE, AT+DR, AT+CLASS, et l'injection des clés) ont été abaissés à seulement 300 ms au lieu des délais standards. 
* **Calibration Polynomiale Hardcodée :** Le système de tare physique (`Poids_Initialiser_Tare()`) a été explicitement désactivé dans la séquence de démarrage. Le poids est désormais exclusivement géré par un polynôme du second degré ($y = ax^2 + bx + c$) calculé en temps réel sur les valeurs brutes. Les coefficients `a`, `b` et le décalage du zéro `c` sont intégrés directement dans la boucle de traitement.
* **Stabilité du Deep Sleep (Correctifs Matériels) :** La mise en veille utilise le mode `Stop` via le timer LPTIM1 et l'instruction `__WFE()`. Deux correctifs critiques sont intégrés : un délai d'environ 60 µs (boucle de 1000 itérations) pour éviter un "coma" au réveil, et une boucle de synchronisation matérielle pour éviter le bug de la "minute sautée" dû à l'horloge lente de 32kHz.
* **Protection Anti-Fuite (OneWire) :** Avant de passer en mode veille profonde, la broche PB0 du capteur de température est automatiquement reconfigurée en mode Entrée (Haute Impédance) afin d'éviter toute perte de courant via la résistance de tirage.

## 🛠️ Architecture Matérielle

| Composant | Rôle | Connexion STM32 | Gestion de l'énergie |
| :--- | :--- | :--- | :--- |
| **STM32G031** | Microcontrôleur principal | N/A | Mode Stop (LPTIM1 via horloge LSI) |
| **DS18B20** | Capteur de Température | **PB0** (Data) | Basculement en Mode Entrée avant veille |
| **HX711** | Convertisseur A/N (Poids) | **PB1** (DT), **PA15** (SCK) | Extinction forcée (SCK à 1) avant veille |
| **Module LoRa** | Transmission Radio | **USART1** (TX/RX) | Mode `AT+LOWPOWER` forcé avant veille |

## 📦 Format des Données Transmises (Payload LoRa)

Le système envoie une trame simplifiée et compacte sous la forme d'une chaîne hexadécimale de 16 caractères de long.

**Format de la chaîne :** `%08lX%08lX`
1. **Les 8 premiers caractères :** Représentent la température brute en degrés Celsius multipliée par 10 000 et convertie en entier 32-bits.
2. **Les 8 derniers caractères :** Représentent le poids calculé en kilogrammes multiplié par 10 000 et converti en entier 32-bits.

*(Une sécurité logicielle garantit que le poids envoyé n'est jamais inférieur à 0.0 kg)*.

## ⚙️ Configuration pour le Déploiement

Pour déployer ce code sur une nouvelle ruche, seules trois zones doivent être modifiées :

1. **Clés LoRaWAN TTN :** Remplacez les constantes `LORA_APP_EUI`, `LORA_APP_KEY`, et `LORA_DEV_EUI` dans le fichier `lora.c`.
2. **Coefficients de la Balance :** Dans la boucle principale de `main.c`, modifiez les variables `a` (actuellement `-6.76251e-12`), `b` (actuellement `9.74269e-05`) et `c` (actuellement `-8.19810`) pour qu'elles correspondent à la courbe de calibration de votre cellule de charge.
3. **Fréquence de Transmission :** Ajustez le paramètre de la fonction `Dormir_X_Secondes_Stop_Mode(30)` à la fin du `main.c` (actuellement réglé sur 30 secondes pour les tests, à augmenter pour la production).