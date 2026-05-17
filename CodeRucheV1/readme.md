# Système Connecté pour Ruche (V1) - IoT LoRaWAN

Ce projet est un système embarqué conçu pour surveiller les constantes vitales d'une ruche (poids et température) et transmettre ces données via le réseau LoRaWAN. Ce projet est basé sur un microcontrôleur STM32G031.

## 📋 Fonctionnalités de la V1

Cette première version du code implémente les fonctionnalités de base du système de surveillance :

* **Acquisition de données :**
    * Lecture de la température interne de la ruche via un capteur **DS18B20** (bus OneWire).
    * Lecture du poids de la ruche via une balance et un module convertisseur **HX711**.
* **Traitement :**
    * Gestion d'une tare automatique au démarrage pour étalonner la balance.
    * Calibration du poids via un facteur modifiable dans le code (`FACTEUR_CALIBRATION`).
    * Conversion des valeurs en entiers pour optimiser la transmission.
* **Transmission LoRaWAN :**
    * Connexion au réseau The Things Network (TTN) via la méthode OTAA (Over-The-Air Activation).
    * Envoi des données (Température et Poids) concaténées dans une trame hexadécimale.
    * Configuration du Data Rate LoRa sur EU868.

## 🛠️ Architecture Matérielle

Le système s'articule autour des composants et des connexions suivantes :

| Composant | Rôle | Connexion STM32 |
| :--- | :--- | :--- |
| **STM32G031** | Microcontrôleur principal | N/A |
| **DS18B20** | Capteur de Température | **PB0** (Data) |
| **HX711** | Amplificateur / ADC (Poids) | **PB4** (DT), **PB5** (SCK) |
| **Module LoRa** | Transmission Radio | **USART1** (TX/RX sur D0/D1) |

### Remarques sur le câblage :
* Le capteur **DS18B20** nécessite l'utilisation d'un timer (TIM3) configuré pour des délais très précis en microsecondes, indispensables pour le bon fonctionnement du protocole OneWire.
* La broche PB0 du capteur de température est configurée en mode "Open-Drain" et nécessite une résistance de tirage (pull-up) matérielle (généralement 4.7kΩ).

## ⚙️ Configuration Logicielle et Clés LoRa

Avant de flasher le code, vous devez configurer vos identifiants réseau LoRaWAN (TTN).

1.  Ouvrez le fichier `lora.c`.
2.  Modifiez les valeurs des variables suivantes avec les clés fournies par votre console TTN:
    * `LORA_APP_EUI`
    * `LORA_APP_KEY`
    * `LORA_DEV_EUI`

## 🚀 Fonctionnement du Code

Le programme suit une logique d'exécution séquentielle :

1.  **Initialisation :** Le système configure les horloges (SysTick pour les millisecondes, TIM3 pour les microsecondes), l'USART pour le module LoRa, et les ports GPIO pour les capteurs.
2.  **Calibration Initiale :** Le système effectue une série de lectures sur le HX711 pour définir la tare (la balance doit être vide à ce moment-là).
3.  **Connexion Réseau :** Le module LoRa est configuré (OTAA, EU868, Classe A) et tente de rejoindre le réseau. Le programme est bloqué tant que le "Network joined" n'est pas confirmé.
4.  **Boucle Principale (`while(1)`) :**
    * **Acquisition Température :** Le STM32 interroge le DS18B20 et convertit la valeur brute en degrés Celsius.
    * **Acquisition Poids :** Le STM32 lit la valeur du HX711, soustrait la tare, applique le `FACTEUR_CALIBRATION`, et s'assure que le résultat n'est pas négatif.
    * **Formatage :** Les valeurs flottantes (Température et Poids) sont multipliées par 100 pour être envoyées sous forme de deux entiers de 16 bits. Ils sont ensuite formatés dans une chaîne hexadécimale de 8 caractères (ex: `099204D2`).
    * **Transmission :** La trame est envoyée au module LoRa.
    * **Mise en veille simulée :** Le système marque une pause (actuellement réglée sur 30 secondes pour les tests via `SYSTICK_Delay()`) avant le prochain cycle.

## 🔧 Points d'attention pour les futures versions (V2)

Ce code (V1) est fonctionnel mais présente des axes d'amélioration critiques pour un déploiement réel :

* **Gestion de l'énergie (Crucial) :** Actuellement, la pause entre les envois utilise `SYSTICK_Delay()` qui garde le microcontrôleur actif. Pour un fonctionnement sur batterie, il est impératif d'implémenter un véritable mode "Deep Sleep" (Stop Mode) du STM32 et de mettre les modules LoRa et capteurs en veille.
* **Robustesse LoRa :** La fonction `LoRa_Setup_And_Join()` bloque indéfiniment si la connexion échoue. Une gestion d'erreur avec tentatives (retries) et retour en veille est nécessaire.
* **Calibration Dynamique :** La tare est définie au démarrage. Il faudrait pouvoir la réinitialiser à distance (via un downlink LoRaWAN) sans avoir à redémarrer physiquement la carte.
* **Transmission binaire :** Actuellement, les données sont envoyées sous forme de chaîne de caractères hexadécimale (`AT+CMSGHEX="%s"`). Envoyer directement les octets en binaire réduirait le temps de transmission radio (Time on Air) et la consommation.