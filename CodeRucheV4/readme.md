# Système Connecté pour Ruche (V4) - IoT LoRaWAN, Calibration Avancée & Anti-Fuite 🐝🔬

Ce projet est un système embarqué autonome conçu pour surveiller les constantes vitales d'une ruche (poids et température) et transmettre ces données via le réseau LoRaWAN. Basée sur un microcontrôleur STM32G031, cette **Version 4** introduit des algorithmes de calibration mathématique complexes et résout des problématiques critiques de fuites de courant en veille profonde.

## 🚀 Nouveautés Majeures de la V4 (Précision & Stabilité)

La V4 repousse les limites de la précision matérielle et corrige des bugs profonds de micro-électronique :

* **Calibration Polynomiale (Mode Expert) :** Abandon de la simple calibration linéaire et de la tare au démarrage. Le poids est désormais calculé via une équation polynomiale du second degré ($y = ax^2 + bx + c$) dérivée d'un étalonnage sur Excel. Le coefficient `c` gère intrinsèquement la tare de la balance (le "zéro").
* **Sécurité Anti-Fuite (OneWire) :** Pour éliminer les fuites de courant résiduelles (qui vidaient la batterie à petit feu), la broche de données du capteur de température (PB0) est reconfigurée en mode "Entrée" (Haute Impédance) juste avant l'endormissement du STM32.
* **Correctifs "Coma" et "Minute Sautée" (Stop Mode) :** Le timer de réveil (LPTIM1) a été patché au niveau matériel. Des boucles d'attente ont été ajoutées pour garantir que les drapeaux d'interruption sont physiquement effacés avant de lancer l'instruction d'endormissement `__WFE()`, évitant ainsi que le STM32 ne se fige indéfiniment.
* **Optimisation de la Radio LoRa :** Utilisation de la commande `AT+LOWPOWER` pour forcer le module en veille ultra-profonde. De plus, la fonction d'envoi a été allégée pour ne plus écouter les fenêtres de réception (Downlink), ce qui réduit drastiquement la consommation énergétique de la transmission.

## 🛠️ Architecture Matérielle

| Composant | Rôle | Connexion STM32 | État en Veille |
| :--- | :--- | :--- | :--- |
| **STM32G031** | Cerveau & Calcul Polynôme | N/A | Mode Stop (LPTIM1 actif) |
| **DS18B20** | Capteur de Température | **PB0** (Data) | Broche isolée (High-Z) |
| **HX711** | Convertisseur A/N (Poids) | **PB1** (DT), **PA15** (SCK) | Mode Power Down (SCK=1) |
| **Module LoRa** | Transmission Radio | **USART1** (TX/RX) | AT+LOWPOWER |

## 📦 Structure de la Trame de Données (Uplink)

Le système envoie une chaîne hexadécimale de 16 caractères composée de deux entiers 32-bits concaténés.
1. **Les 8 premiers caractères :** La température en degrés Celsius multipliée par 10 000.
2. **Les 8 derniers caractères :** Le poids en kilogrammes multiplié par 10 000.

## 🧮 Configuration de la Calibration (Polynôme)

Étant donné que cette version utilise un algorithme de lissage polynomial, la configuration ne se fait plus via une simple variable, mais en modifiant les trois coefficients `a`, `b` et `c` directement dans le fichier `main.c`.

1. Obtenez les valeurs brutes du HX711 pour plusieurs poids connus en utilisant la fonction `Poids_Lire_Brut_Pur()`.
2. Tracez une courbe de tendance polynomiale d'ordre 2 sur Excel.
3. Reportez les valeurs générées dans les variables `a`, `b`, et `c`. Le coefficient `c` déterminera automatiquement le point zéro de la balance (pas de tare manuelle requise).

## ⚙️ Configuration Réseau (TTN)

Les identifiants Over-The-Air Activation (OTAA) doivent être renseignés dans le fichier `lora.c` :
* `LORA_APP_EUI` : Identifiant de l'application.
* `LORA_APP_KEY` : Clé de session cryptographique.
* `LORA_DEV_EUI` : Identifiant unique du composant physique.
