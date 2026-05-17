# Système Connecté pour Ruche (V2) - IoT LoRaWAN 🐝📡

Ce projet est un système embarqué conçu pour surveiller les constantes vitales d'une ruche (poids et température) et transmettre ces données via le réseau LoRaWAN. Ce projet est basé sur un microcontrôleur STM32G031.

## 🚀 Nouveautés et Améliorations de la V2

La Version 2 apporte des correctifs majeurs de stabilité et de précision par rapport à la V1 :

* **Sécurité Anti-Blocage (Capteur de Poids) :** Ajout d'un "timeout" matériel (100 000 itérations) dans la lecture du capteur HX711 (`Poids_Lire_Unique`). Si la balance est débranchée ou défectueuse, le programme ne se fige plus indéfiniment.
* **Optimisation des Délais LoRa :** Les temps d'attente (timeouts) lors de la configuration du module LoRa ont été drastiquement réduits (de 2000 ms à 300 ms) pour un démarrage beaucoup plus rapide.
* **Ordre d'Initialisation Logique :** La fonction d'étalonnage de la balance (`Poids_Tare()`) est désormais appelée *avant* la connexion au réseau LoRa. Cela évite d'attendre la connexion réseau (qui peut prendre du temps) pour effectuer la tare à vide.
* **Précision Accrue des Données :** Les valeurs flottantes de température et de poids sont désormais multipliées par 10 000 (au lieu de 100) pour conserver 4 décimales de précision avant l'envoi.
* **Filtrage des Bruits (Balance) :** Implémentation d'un filtre logiciel remettant la valeur du poids à `0.0f` si le capteur renvoie une légère valeur négative due au bruit électronique.

## 🛠️ Architecture Matérielle

| Composant | Rôle | Connexion STM32 |
| :--- | :--- | :--- |
| **STM32G031** | Microcontrôleur principal | N/A |
| **DS18B20** | Capteur de Température | **PB0** (Data) |
| **HX711** | Convertisseur A/N (Poids) | **PB4** (DT), **PB5** (SCK) |
| **Module LoRa** | Transmission Radio | **USART1** (TX/RX sur D0/D1) |

## 📦 Structure de la Trame de Données (Payload LoRa)

Le système envoie une chaîne hexadécimale de 16 caractères composée de deux entiers 32-bits concaténés. 

**Format :** `%08lX%08lX`
1. **Les 8 premiers caractères :** La température en degrés Celsius multipliée par 10 000.
2. **Les 8 derniers caractères :** Le poids en kilogrammes multiplié par 10 000.

*Exemple de décodage coté serveur (Node-RED / TTN) :*
* Trame reçue : `0003BDD00001E208`
* `0003BDD0` (Hex) -> `245200` (Décimal) -> / 10000 = **24.52 °C**
* `0001E208` (Hex) -> `123400` (Décimal) -> / 10000 = **12.34 kg**

## ⚙️ Configuration Rapide

1. **Réseau LoRaWAN (TTN) :** Modifiez les clés d'identification dans `lora.c` avec les identifiants de votre application TTN :
   * `LORA_APP_EUI`
   * `LORA_APP_KEY`
   * `LORA_DEV_EUI`
2. **Calibration de la Balance :** Remplacez la valeur de `#define FACTEUR_CALIBRATION` (actuellement `10750.0f`) dans `main.c` par la valeur correspondant à votre propre cellule de charge.
3. **Délai d'envoi :** À la fin de `main.c`, ajustez la valeur de `SYSTICK_Delay()`. Elle est actuellement réglée sur 30 secondes (`30000`) pour les tests, mais doit être passée à 1 heure (`3600000`) pour le déploiement sur la ruche.

## 🚧 Pistes d'Amélioration (Vers la V3)

Bien que cette V2 soit plus robuste, elle nécessite encore des optimisations pour un déploiement autonome de longue durée :

1. **Gestion de l'énergie (Deep Sleep) :** Remplacer le `SYSTICK_Delay()` bloquant par une véritable mise en veille du microcontrôleur (mode Stop ou Standby) couplée à une alarme RTC.
2. **Payload Binaire :** Remplacer l'envoi de chaînes hexadécimales (`AT+CMSGHEX`) par un envoi binaire pur pour réduire drastiquement le "Time on Air" (temps d'émission radio) et économiser la batterie.