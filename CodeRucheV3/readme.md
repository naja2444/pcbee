# Système Connecté pour Ruche (V3) - IoT LoRaWAN Ultra Basse Consommation 🐝🔋

Ce projet est un système embarqué autonome conçu pour surveiller les constantes vitales d'une ruche (poids et température) et transmettre ces données via le réseau LoRaWAN. Basée sur un STM32G031, cette **Version 3** est l'aboutissement du projet, optimisée pour un déploiement réel en pleine nature avec une gestion avancée de l'énergie.

## 🚀 Nouveautés et Améliorations de la V3
La V3 se concentre intégralement sur l'optimisation énergétique:

* **Deep Sleep (Stop 1 Mode - 100% Safe) :** Remplacement des délais bloquants par une véritable mise en veille profonde du microcontrôleur entre deux envois. Le système utilise le timer basse consommation (LPTIM1) cadencé par l'horloge LSI (32 kHz) et un réveil par événement (`__WFE()`) pour éviter les crashs d'interruption.
* **Mise en Sommeil des Périphériques :** Avant de s'endormir, le STM32 éteint activement ses capteurs :
  * Le module LoRa est endormi via la commande `AT+SLEEP`.
  * Le convertisseur de poids HX711 est mis en mode "Power Down" en maintenant sa broche d'horloge (SCK) à l'état HAUT.
 **Fiabilité :** L'UART est automatiquement réinitialisé à chaque réveil du STM32 pour garantir une communication sans faille avec le module LoRa.

## 🛠️ Architecture Matérielle

| Composant | Rôle | Connexion STM32 | Consommation (Veille) |
| :--- | :--- | :--- | :--- |
| **STM32G031** | Cerveau & Gestion de l'énergie | N/A | ~10 µA (Stop Mode) |
| **DS18B20** | Capteur de Température | **PB0** (Data) | ~1 µA |
| **HX711** | Convertisseur A/N (Poids) | **PB4** (DT), **PB5** (SCK) | < 1 µA (Power Down) |
| **Module LoRa** | Transmission Radio | **USART1** (TX/RX) | ~2 µA (Sleep Mode) |

*Note : Pour atteindre ces niveaux de consommation, assurez-vous d'avoir retiré les LEDs témoins d'alimentation matérielles de vos modules.*

## 📦 Structure de la Trame de Données (Uplink)

Le système envoie une chaîne hexadécimale de 16 caractères composée de deux entiers 32-bits concaténés.

**Format :** `%08lX%08lX`
1. **Les 8 premiers caractères :** La température en degrés Celsius multipliée par 10 000.
2. **Les 8 derniers caractères :** Le poids en kilogrammes multiplié par 10 000.

*Exemple : `0003BDD00001E208` équivaut à 24.52 °C et 12.34 kg.*

## ⚙️ Déploiement et Configuration Initiale

1. **Clés LoRaWAN :** Dans le fichier `lora.c`, configurez les clés de votre application TTN :
   * `LORA_APP_EUI`
   * `LORA_APP_KEY`
   * `LORA_DEV_EUI`
<<<<<<< HEAD
2. **Calibration de la Balance :** Dans le fichier `main.c`, ajustez la macro `#define FACTEUR_CALIBRATION` (actuellement `10764.17f`) avec le facteur multiplicateur de votre propre cellule de charge.
3. **Tare Automatique :** Le système effectue une Tare (mise à zéro) automatique au démarrage. Assurez-vous que la balance est vide lors de la mise sous tension.
=======
2. **Calibration de la Balance :** Remplacez la valeur de `#define FACTEUR_CALIBRATION` (actuellement `10750.0f`) dans `main.c` par la valeur correspondant à votre propre cellule de charge.
3. **Délai d'envoi :** À la fin de `main.c`, ajustez la valeur de `SYSTICK_Delay()`. Elle est actuellement réglée sur 30 secondes (`30000`) pour les tests, mais doit être passée à 1 heure (`3600000`) pour le déploiement sur la ruche.

## 🚧 Pistes d'Amélioration (Vers la V3)

Bien que cette V2 soit plus robuste, elle nécessite encore des optimisations pour un déploiement autonome de longue durée :

1. **Gestion de l'énergie (Deep Sleep) :** Remplacer le `SYSTICK_Delay()` bloquant par une véritable mise en veille du microcontrôleur (mode Stop ou Standby) couplée à une alarme RTC.
2. **Payload Binaire :** Remplacer l'envoi de chaînes hexadécimales (`AT+CMSGHEX`) par un envoi binaire pur pour réduire drastiquement le "Time on Air" (temps d'émission radio) et économiser la batterie.
