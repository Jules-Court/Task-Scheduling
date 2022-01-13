# SY5-projet-2021-2022

Le projet du cours de systèmes d'exploitation (L3), 2021-2022

Ce dépôt contient :

  - l'[énoncé](enonce.md) du projet, complété de détails sur le
    [protocole](protocole.md) de communication entre le démon et son
    client,

  - un [script de test](run-cassini-tests.sh) du client (`cassini`), et
    le jeu de [tests](tests) correspondant, 

  - quelques fichiers sources pour vous aider à démarrer.

Par ailleurs, nous avons ouvert un [serveur
discord](https://discord.gg/7ArJtu8Xnv) avec un salon dédié aux questions concernant le projet.

## Mode d'emploi

Pour commencer, il faut lancer le daemon avec la commande ./saturnd  
Vous pouvez maintenant utilisé cassini.  
Commande de cassini :  
Pour lister une tâche :  
```
./cassini 
./cassini -l
```
Pour créer une tâche :
```
./cassini -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
//Exemple : 
./cassini -c -m 0-2,5,10 toto.sh

```
Pour fermer le daemon  :
```
./cassini -q
```
Pour supprimer une tache : 
```
./cassini -r TASKID
//Exemple : 
./cassini -r 0
```
Pour obtenir des informations (heure + code de sortie) sur toutes les exécutions passées d'une tâche :  
```
./cassini -x TASKID
//Exemple : 
./cassini -x 0
```
Pour obtenir la sortie standard de la dernière exécution d'une tâche :   
```
./cassini -o TASKID
//Exemple : 
./cassini -o 0
```
Pour obtenir l'erreur standard d'une tache :
```
./cassini -e TASKID
//Exemple : 
./cassini -e 0
```
Pour afficher l'aide : 
```
./cassini -h
```
Pour chercher les tuyaux dans PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes) :  
```
./cassini -p PIPES_DIR
```





