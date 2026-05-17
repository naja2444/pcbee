# pcbee
projet de L3 ESET groupe 2

## 👥 Équipe

| Nom | Prénom |
|-----|--------|
| Manganello | Sophian |
| Guilloud | Bastian |
| Rakotovao | Irina |
| Hegoburu | Mathieu |

**Formation :** L3 ESET — Électronique Systèmes Embarqués et Télécommunications 
**Université :** Savoie Mont Blanc  
**Encadrants :** Sylvain Montagny - Cedric Bermond - Jean-François Roux

---

## 📖 Description du Projet

Ce dépôt regroupe l'ensemble du code et fichiers du projet réalisés. L'objectif est de concevoir un système embarqué permettant la mesure de poid et température d'une ruche à distance

---

## 📖 Cahier des charges détaillé

**Validation des fonctionnalités du produit :**

-  Mesure du poids, transmission en LoRaWAN et réception sur le serveur.

-  Vérification de la dynamique : mesure d'un nouveau poids (+x Kg), transmission LoRaWAN affichage.

-  Vérification de la précision : mesure d'un nouveau poids (+x g), transmission LoRaWAN affichage.

-  Vérification de la constance des mesures : plusieurs mesures successives donnent les même valeurs (moyennage, etc...)

-  Mesure de la température, transmission LoRaWAN affichage.

-  Mesure de la capacité restante de la batterie, transmission LoRaWAN affichage.

-  Sauvegarde de la valeur de la tare (mémoire flash)

-  Les données transmises sont sauvegardées dans une BDD.

 

**Validation des caractéristiques du produit :**

-  Consommation en transmission

-  Consommation en fonctionnement sans veille

-  Consommation en veille

-  Format : carte électronique dans un boitier pour intégration dans la ruche.

-  Le PCB intègre tous les composants (STM32, HX711, DS18B20) pour minimiser la taille.

 

**Validation des services à l'utilisateur :**

-  Tare de la balance manuelle (par appui d'un bouton sur la carte)

-  Tare de la balance à distance (par downlink LoRaWAN)

-  Modification du cycle de mesure à distance (downlink LoRaWAN) 5 minutes par défaut.

-  Affichage d'un dashboard et valorisation des données

-  Détection du vol de ruche

-  Détection d'essaimage

-  Détection de miellée

 

**Validation du rendu et de la documentation :**

-  Diffusion du code et versionning : GitHub.

-  Les documents sont dans le canal Teams avec les formats préconisés.

-  Le système tourne depuis au moins 3 semaines pour garantir son fonctionnement.

-  Le Device LoRaWAN final à été provisionné dans l'application fournie par M Montagny.

---

-  ## 🏗️ Architecture Globale du Système

Le système est conçu de bout en bout pour être autonome, fiable et modulaire. Le flux de données suit ce chemin :

**1. La Ruche (Acquisition)** ➡️ **2. The Things Network (Transport)** ➡️ **3. Node-RED (Traitement)** ➡️ **4. InfluxDB / Dashboard (Affichage)**

---

## 💻 1. Le Logiciel Embarqué (C / STM32)

Le code embarqué (situé dans les dossiers de version, ex: `V5`) est programmé en C bare-metal sur un microcontrôleur **STM32G031**. Il est conçu pour une consommation énergétique extrême (Ultra-Low Power).

### Composants Matériels
* **Cerveau :** Carte STM32G031.
* **Poids :** jauge de contrainte + amplificateur **HX711** (Broches PB1/PA15).
* **Température :** Sonde étanche **DS18B20** via protocole OneWire (Broche PB0).
* **Radio :** Module **LoRaWAN** E5 (Communication série via USART1).

### Fonctionnalités Clés du Code
* **Calibration Polynomiale Absolue :** Le poids n'est pas calculé par une simple division, mais via un polynôme du second degré ($y = ax^2 + bx + c$) paramétré dans le code, permettant de corriger les non-linéarités de la balance.
* **Communication Unidirectionnelle (Uplink Only) :** Pour maximiser l'autonomie, le système est configuré pour n'envoyer que des données. **Il n'y a pas d'écoute de Downlink.** Dès que le module LoRa a confirmé l'envoi (`Done`), le système s'endort immédiatement, réduisant le temps d'allumage radio au strict minimum.
* **Sécurité Anti-Fuite (Hardware & Software) :** * Les broches des capteurs (comme le OneWire sur PB0) sont reconfigurées en mode "Entrée" avant la mise en veille pour empêcher le courant de fuir par les résistances de tirage.
  * Le HX711 est forcé en mode "Power Down" matériel.
* **Deep Sleep (Stop Mode) :** Utilisation du timer basse consommation LPTIM1 avec l'instruction `__WFE()` pour endormir le microcontrôleur à ~10µA entre chaque mesure.

---

## 🌐 2. Le Serveur et Traitement des Données (Node-RED)

Plutôt que d'alourdir le microcontrôleur avec des calculs d'interface, toute la "logique métier" est déportée sur le serveur via **Node-RED**.

* **Décodage du Payload :** Le STM32 envoie une trame hexadécimale ultra-légère de 16 caractères (Température + Poids). Node-RED intercepte ce message depuis TTN, le parse en binaire et reconstitue les valeurs réelles.
* **Tare Logicielle (Soft-Tare) :** C'est l'une des forces de cette architecture. La balance de la ruche envoie le poids brut exact calculé par le polynôme. La Tare (le "zéro" lors de la pose d'une hausse ou le retrait de matériel) est gérée **exclusivement sur Node-RED** via des variables globales. L'utilisateur peut remettre la balance à zéro depuis son tableau de bord sans jamais toucher physiquement à la ruche.
* **Calculs Dérivés :** Node-RED calcule en temps réel le "Poids du miel" (Poids net - Poids de la ruche vide) et génère des historiques glissants.

---

## 📊 3. Stockage et Visualisation (InfluxDB & Dashboard)

Une fois traitées et nettoyées par Node-RED, les données sont expédiées vers deux canaux :
* **Base de données temporelle (InfluxDB) :** Stockage sécurisé dans le bucket `Projet_L3_Ruche` pour garder un historique complet des miellées et de la survie hivernale.
* **Dashboard Node-RED / Grafana :** Interface utilisateur permettant à l'apiculteur de visualiser les courbes de poids, les variations de température, et de déclencher la Tare logicielle.
