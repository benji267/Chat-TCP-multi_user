# Chat-TCP-multi_user

# compilation :

Le programme compile avec un Makefile avec la recette make

# fonctionnement :

Le serveur marche soit en broadcast soit en mode peer-to-peer servant de base de données et de validation des connexions entre les clients
Pour que le serveur fonctionne en broadcast il suffit de lancer les programmes :

./server
./sender

Pour que le serveur offre la possibilité du peer-to-peer il suffit de lancer les programmes :

./server P
./sender P


Le port utilisé par défaut pour la communication du serveur avec les clients est le port 2043, on peut simplement le changer ici: 

#define PORT_PAR_DEFAUT 2043

Les utilisateurs quittent avec un /quit

