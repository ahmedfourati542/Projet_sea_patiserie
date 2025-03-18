# Simulation de Pâtisserie

Ce projet simule un système de traitement de commandes pour une pâtisserie. Il présente trois modes de fonctionnement différents : traitement séquentiel (Mono), traitement parallèle utilisant plusieurs processus (Multiprocessus), et traitement parallèle utilisant plusieurs threads (Multithread).

## Structure du Projet

Le projet est divisé dans les fichiers suivants :

* **main.c :** Contient la fonction principale du programme, l'interface utilisateur pour choisir le mode d'exécution, et la boucle principale pour gérer plusieurs commandes. Il définit également le tableau global `gateaux`.
* **mode_mono.c :** Implémente le mode de traitement de commandes séquentiel.
* **mode_multiprocess.c :** Implémente le mode de traitement de commandes parallèle en utilisant plusieurs processus. Il définit également le tableau global `ingredients` et gère la mémoire partagée pour la communication inter-processus.
* **mode_multithread.c :** Implémente le mode de traitement de commandes parallèle en utilisant plusieurs threads au sein d'un même processus. Il utilise des mutex et des variables de condition pour la synchronisation des threads et la gestion de la file d'attente des commandes et du stock d'ingrédients.

## Comment Compiler

Pour compiler le projet, vous aurez besoin d'un compilateur C (comme GCC) et de la librairie POSIX threads. Ouvrez votre terminal et naviguez jusqu'au répertoire contenant les fichiers sources. Ensuite, exécutez la commande suivante :

```bash
gcc main.c mode_mono.c mode_multiprocess.c mode_multithread.c -o patisserie -lpthread
