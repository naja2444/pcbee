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

§  Mesure du poids, transmission en LoRaWAN et réception sur le serveur.

§  Vérification de la dynamique : mesure d'un nouveau poids (+x Kg), transmission LoRaWAN affichage.

§  Vérification de la précision : mesure d'un nouveau poids (+x g), transmission LoRaWAN affichage.

§  Vérification de la constance des mesures : plusieurs mesures successives donnent les même valeurs (moyennage, etc...)

§  Mesure de la température, transmission LoRaWAN affichage.

§  Mesure de la capacité restante de la batterie, transmission LoRaWAN affichage.

§  Sauvegarde de la valeur de la tare (mémoire flash)

§  Les données transmises sont sauvegardées dans une BDD.

 

**Validation des caractéristiques du produit :**

§  Consommation en transmission

§  Consommation en fonctionnement sans veille

§  Consommation en veille

§  Format : carte électronique dans un boitier pour intégration dans la ruche.

§  Le PCB intègre tous les composants (STM32, HX711, DS18B20) pour minimiser la taille.

 

**Validation des services à l'utilisateur :**

§  Tare de la balance manuelle (par appui d'un bouton sur la carte)

§  Tare de la balance à distance (par downlink LoRaWAN)

§  Modification du cycle de mesure à distance (downlink LoRaWAN) 5 minutes par défaut.

§  Affichage d'un dashboard et valorisation des données

§  Détection du vol de ruche

§  Détection d'essaimage

§  Détection de miellée

 

**Validation du rendu et de la documentation :**

§  Diffusion du code et versionning : GitHub.

§  Les documents sont dans le canal Teams avec les formats préconisés.

§  Le système tourne depuis au moins 3 semaines pour garantir son fonctionnement.

§  Le Device LoRaWAN final à été provisionné dans l'application fournie par M Montagny.
