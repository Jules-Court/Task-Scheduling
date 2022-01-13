
# Présentation

Deux modules avec deux rôles différents:

- Cassini : 
	- Lecture des commandes utilisateur via getopt
	- Préparation et envoi de requêtes vers Saturnd
	- Réception des réponses de Saturnd
	
- Saturnd : 
	- Réception des requêtes de Cassini
	- Gestion et execution des tâches
	
Caractéristiques principales :

- Communication entre les deux modules effectuées via pipes nommés :
- Requêtes / réponses transmises sous forme de tableau de bits en Big Endian.
- Pipes bloquants par défaut


## Structure de Cassini :
- Récupération des requêtes auprès de l'utilisateur via getopt
- Création de la requête et envoi sur le pipe
- Attente de la réponse de Saturnd
- Affichage de la réponse de Saturnd par les fonctions de request-reply.c
- Utilisation de SetTimer afin de renvoyer un message d'erreur après un certain delai en cas d'absence de Saturnd sur le pipe reponse. SetTimer renvoi un signal que l'on traite en renvoyant un message d'erreur. En cas de présence de Saturnd, on arrête l'éxecution de SetTimer.

## Structure de Saturnd :

### Aspect général
- Création d'un thread pour effectuer la gestion des tâches
  - Permet de libérer le thread principal, qui reste occupé à gèrer la communication avec Cassini
  - Ce thread détecte l'heure d'exécution de la prochaine tâche, et le cas échéant, fork(), puis execvp() la tâche, et attend le code de retour
- Création des pipes s'ils n'existent pas, dans un repertoire tmp/Username/saturnd/pipes
- Réception des requêtes et envoi des réponses
- Traitement des requêtes et réponses par les fonctions de file-system
  - Le format binaire des requêtes est généré par les fonctions de file-system
  - file-system stockant les paquets binaires sur disque sous forme BigEndian
- Création d'un fork() afin de détacher saturnd du terminal en faisant tourner le processus sur le fils
- Traitement des signaux SIGINT et SIGTERM, lorsque l'on reçoit ces signaux on ne fait rien ce qui empèche la fermeture de saturnd de façon inconventionelle. Cependant, on ne peut modifier la gestion du signal SIGKILL.

### Gestion du pipe de commande:
  - Utilisation de poll() pour détecter la présence de données
    - Faute d'utiliser poll, nous devrions effectuer un read(), qui retournerait immédiatement avec 0, tant qu'aucune donnée n'est présente
      - L'appel permanent de ce read(), afin de tester la présence de données dans la boucle principale entraînerait une forte consommation CPU
    - l'appel à poll() nous permet de détecter (POLLIN) la présence de données dans le pipe, et ainsi de n'effectuer le read() que quand c'est nécéssaire
    - poll() permettant d'attendre un timeout, le thread de lecture est donc bloqué en l'attente de données, ce qui permet de ne pas consommer inutilement de CPU
    - Cas particulier : poll() retourne en POLLHUP lorsque cassini ferme son côté du pipe.
      - Ce retour en POLLHUP est alors systèmatique et le poll() ne remplit plus son rôle d'attente de POLLIN
        - Dans ce cas, saturnd ferme puis ouvre à nouveau le pipe de requetes, afin de réinitialiser ce POLLHUP



### Structure de données genérées et gerées par file-system.c : 
- Création d'un répertoire .sysfile contenant tout les répertoires de tâches, nommés d'après leurs taskids.
  - Au sein de ces tâches, sont stockés des fichiers binaires (.bin) contenant :
    - les commandes à exécuter, 
    - les dates d'exécution et l'exitcode associé, 
    - la planification d'exécution, 
    - la dernière sortie standard 
    - la dernière sortie d'erreur.
  - Toutes ces informations sont stockées en Big Endian
  - 
- Création d'un fichier index.bin, contenant l'id de taskid le plus récent
  - Cela permet d'éviter de réutiliser des taskids précédents

- Fonctions de gestion de ces tâches (permettant de les créer, les supprimer, les modifier, lire les valeurs, ...)


## Autres aspects, communs à Saturnd et Cassini

- La gestion des chemins vers les répertoires des pipes est gèrée par les fonctions de path.c : génération des chemins

- Structure des commandes stockées de la manière suivant (selon protocole.md):
  - Structure commandline contenant un argc (longueur de la commande) et un tableau d'argv (commande et paramètres) stockés sous forme de structures string-utils
  - Structure string-utils contenant chacune un pointeur vers un tableau de uint8_t  représentant l'argument stocké ainsi que sa longueur

- Ainsi, les commandes sont préparées de la manière suivant pour pouvoir être par la suite interprétées :
  - commandline.c prépare les commandes sous la forme de tableaux de bits interprétables sous l'endian de l'host. 
  - Pour cela, il utilise les fonctions de string-utils.c
qui renvoie les argv de la commande sous forme de tableaux de bits que commandline.c pourra utiliser.
- struct commandline -> string-utils.c -> commandline.c -> commandes interprétables

