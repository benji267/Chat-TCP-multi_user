#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_NAME_SIZE 10
#define MAX_NUMBER_OF_CLIENTS 10
#define MAX_MSG_LENGTH 1024
#define PROTOTYPE_MESSAGE strlen("Message reçu de la part de : ")+MAX_NAME_SIZE
#define PROTOTYPE_MSG "Message reçu de la part de : "
#define LENGTH_PROTOTYPE_MSG PROTOTYPE_MESSAGE-MAX_NAME_SIZE

#define PORT_PAR_DEFAUT 2043

typedef enum Bool{False=0, True=1} Bool ;


typedef struct Client {
    int sockfd;
    char name[MAX_NAME_SIZE];
    Bool connected;
    struct sockaddr_in client;
} Client;

struct pthread_struct{
    int id_thread;
    Client* client; // tableau d'information des clients
    int sockfd;
    pthread_mutex_t m;
    struct sockaddr_in client_infos;
};

void raler(const char* message){
    perror(message);
    exit(1);
}

const char Welcome_message[]=
"Veuillez entrer votre identifiant (en 9 caractères) max pour pouvoir vous connecter au serveur :\n      ";

const char upcoming_connection[]="Connection comming from ";

const char connection_accepted[]="Connection accepted :D\n";


// variable globale servant de savoir le nombre de clients qui ont entrés 
// leurs noms et donc qu'on peut utiliser pthread_join() sur les threads 
// qu'ils l'ont créés

int nombre_total_clients=0; // les clients qui se sont connectés et ont entrés leurs id

// variable globale qui va être mise à jour à chaque nouvelle connection
// d'un thread
fd_set readfs;

/** cet entier servira pour enregistrer la valeur de retour du accept pour
 *  la comparer avec le maximum du descripteur de socket actuel et garder
 *  la valeur max de ces deux descripteurs pour pouvoir l'utiliser dans 
 *  la fonction select
*/


int max_sockfd;



void * fonction(void* arg)
{
    struct pthread_struct* new_client=arg;

    int socket_nv_client=new_client->sockfd;

    char name[MAX_NAME_SIZE]; // ---------- to free
    memset(name,'\0',MAX_NAME_SIZE);


    // on dit ensuite au nouveau client d'entrer son nom et on le crée en l'ajoutant
    // à la liste des clients

    if (send(socket_nv_client,Welcome_message,106, 0) < 0) {
        perror("send()");
        exit(-8);
    }

    if ( recv(socket_nv_client,name,MAX_NAME_SIZE - 1, 0)==-1) {
        perror("recv()");
        /* if recv error we disonnect the client */
        exit(-6);
    }

    int c=strlen(name)-1;

    while(c<9){ // pour que le nom de tous les client soit = 10
        name[c]=' ';
        c++;
    }

    int i;

    if(pthread_mutex_lock   (&new_client->m)!=0) 
        raler("pthread_mutex_lock");

    for(i=0; i<MAX_NUMBER_OF_CLIENTS;i++)
    {
        if(new_client->client[i].connected==False)
        {
            strcpy(new_client->client[i].name,name);

            // on ajoute ce descripteur à ceux qu'on avait avant pour lire là-dedant
            // readfs donc s'actualise à chaque nouvelle connection de clients 

            FD_SET(socket_nv_client,&readfs);

            nombre_total_clients++;

            new_client->client[i].sockfd=socket_nv_client;

            // on verra si ce nouveau descripteur de socket est supérieur au max qu'on avait
            // avant. Si oui on actualise
            if(socket_nv_client>max_sockfd)
                max_sockfd=socket_nv_client;
            
            new_client->client[i].connected=True;

            new_client->client[i].client=new_client->client_infos;

            break;
        }
    }

    if(pthread_mutex_unlock (&new_client->m)!=0)  
        raler("pthread_mutex_unlock");

    pthread_exit (0);
}

void send_peer_id(int sockfd,struct sockaddr_in client){

    socklen_t fromlen = sizeof(struct sockaddr_in);

    char str[6];
    sprintf(str, "%d", client.sin_port);
        
    if(send(sockfd,str,6, 0) < 0) {
        perror("send()");
        exit(-8);
    }

}


Bool demande_connexion(char* buffer){

    char prototype[3]="-p ";

    if(strncmp(buffer, prototype,3))
        return False;
    
    else
        if(strlen(buffer)>3)
            return True;
}

Bool verifie_identifiant(char* buffer,Client* clients,int* index){


    char nom_client[MAX_NAME_SIZE];
    memset(nom_client,' ',MAX_NAME_SIZE);
    nom_client[MAX_NAME_SIZE-1]='\0';

    int i=3;

    while(buffer[i]!='\n'){
        nom_client[i-3]=buffer[i];
        i++;
        if(i>=MAX_NAME_SIZE+3)
            break;
    }

    if(i==3 || i>=MAX_NAME_SIZE+3)
        return False;
    
    else{
    
        for(int j=0;j<nombre_total_clients;j++)
        {
            if(!strcmp(nom_client,clients[j].name)){
                *index=j;

                return True;
            }
        }
        return False;
    }

}

Bool peer_to_peer=False;


int main(int argc, char **argv)
{
    int sockfd;                   // descripteur de socket
    char buf[MAX_MSG_LENGTH];               // espace necessaire pour stocker le message recu

    char buf_envoyer[MAX_MSG_LENGTH];
    int nbre_threads=0;

    if(argc >=2){
        if(!strncmp(argv[1],"P",1)){
            peer_to_peer=True;
        }
    }

    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx, NULL);

    pthread_t* tid=malloc(MAX_NUMBER_OF_CLIENTS*sizeof(pthread_t)); // ------- to free
    

    // un tableau pour stocker les informations de tous les clients du serveur
    
    Client clients[MAX_NUMBER_OF_CLIENTS];
    for(int i=0; i<MAX_NUMBER_OF_CLIENTS;i++)
        clients[i].connected=False;

    // taille d'une structure sockaddr_in utile pour la fonction recvfrom
    socklen_t fromlen = sizeof(struct sockaddr_in);

    struct sockaddr_in my_addr;   // structure d'adresse qui contiendra les param reseaux du recepteur
    struct sockaddr_in client;    // structure d'adresse qui contiendra les param reseaux de l'expediteur


    // creation de la socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // initialisation de la structure d'adresse du recepteur (pg local)

    // famille d'adresse
    my_addr.sin_family = AF_INET;

    // recuperation du port du recepteur
    my_addr.sin_port = htons(PORT_PAR_DEFAUT);

    // adresse IPv4 du recepteur
    inet_aton("127.0.0.1", &(my_addr.sin_addr));

    // association de la socket et des param reseaux du recepteur
    if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(my_addr)) != 0)
    {
        perror("erreur lors de l'appel a bind -> ");
        exit(-2);
    }

    if(listen(sockfd, 5)==-1)
    {
        perror("erreur lors de l'appel a listen -> ");
        exit(-3);
    }



    max_sockfd=sockfd;

    int new_sockfd; // valeur de retour de accept
    
    fd_set readfs2;
    FD_ZERO(&readfs);
    FD_SET(0,&readfs);
    FD_SET(sockfd, &readfs); // pour être notifié s'il y a une nouvelle connection

    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int nb_clients_actuels;
   
    while(1)
    {
        FD_ZERO(&readfs2);

        memset(buf,'\0',1024);
        memset(buf_envoyer,'\0',1024);

        if(pthread_mutex_lock (&mtx)!=0) 
            raler("pthread_mutex_lock");
        
        // readfs2 qui va se modifier par select tandis que readfs reste constante
        readfs2=readfs;

        if(pthread_mutex_unlock (&mtx)!=0) 
            raler("pthread_mutex_lock");

        

        if(select(max_sockfd+1,&readfs2,NULL,NULL,&timeout)==-1)
        {
            perror("erreur lors de l'appel a select -> ");
            exit(-4);
        }

        if(FD_ISSET(sockfd, &readfs2))
        {   // le cas où un nouveau client arrive alors select dit qu'on peut lire dans
            // cette socket il faudra alors appeler socket

            if((new_sockfd=accept(sockfd,(struct sockaddr*)&client,&fromlen))==-1)
            {
                perror("erreur lors de l'appel a listen -> ");
                exit(-3);
            }

            printf("Client's port : %d\n",client.sin_port);

            struct pthread_struct set;

            set.id_thread=nbre_threads;
            set.client=clients;
            set.sockfd=new_sockfd;
            set.m=mtx;
            set.client_infos=client; // copie des infos du client (port et adresse)

            if((errno=pthread_create(&tid[nbre_threads],NULL,fonction,&set)>0))
                raler("pthread_create\n");

            nbre_threads++;
        }

        // si quelque chose arrive dans l'entrée standard
        if(FD_ISSET(0,&readfs2))
        {
            // -------------------
        }   

        if(pthread_mutex_lock (&mtx)!=0) 
            raler("pthread_mutex_lock");
        
        nb_clients_actuels=nombre_total_clients;

        if(pthread_mutex_unlock (&mtx)!=0) 
            raler("pthread_mutex_lock");

        // puis on regarde si un événement est arrivé dans les autres descripteur de socket
        for(int i=0;i<nb_clients_actuels;i++){

            if(clients[i].connected==False)
                continue;
            
            memset(buf,'\0',1024);
            memset(buf_envoyer,'\0',1024);
            // vu que select modifie le set de descripteur de sockets,
            // on regarde ceux qui ont écrit quelque chose dans le readfs2 tandis
            // que readfs ne change pas

            int n = 0;
            if(clients[i].connected && FD_ISSET(clients[i].sockfd,&readfs2))
            {
                if((n=recv(clients[i].sockfd,buf,MAX_MSG_LENGTH,0)) == -1)
                {
                    perror("erreur de reception -> ");
                    exit(-3);
                }
                if(!strncmp(buf,"/quit",5)){
                    clients[i].connected=False;
                    continue;
                }

                if(peer_to_peer)
                {
                    
                    if(n!=0)
                    {
                        if(demande_connexion(buf))
                        {
                            int index;
                            if(verifie_identifiant(buf,clients,&index))
                            {
                                strcat(buf_envoyer,upcoming_connection);
                                strcat(buf_envoyer,clients[i].name);
                                strcat(buf_envoyer,"would you like to accept it?\n (y/n)");
                                send(clients[index].sockfd,buf_envoyer,strlen(buf_envoyer),0);
                                char answer;
                                recv(clients[index].sockfd,&answer,1,0);


                                if(answer=='y' || answer=='O' || answer=='Y' || answer=='o')
                                    send(clients[index].sockfd,&(clients[i].client),fromlen,0);
                                    send(clients[i].sockfd,connection_accepted,24,0);
                                    sleep(1);
                                    send(clients[i].sockfd,&(clients[i].client),fromlen,0);
                                    
                            }
                        }
                    }
                }
                else
                {

                    if((n>=MAX_MSG_LENGTH-PROTOTYPE_MESSAGE-1))
                        buf[MAX_MSG_LENGTH-PROTOTYPE_MESSAGE-1] = '\0';

                    strcpy(buf_envoyer,PROTOTYPE_MSG);
                    strcpy(buf_envoyer+LENGTH_PROTOTYPE_MSG,clients[i].name);
                    strcpy(buf_envoyer+PROTOTYPE_MESSAGE-1,"\n      ");
                    strcpy(buf_envoyer+PROTOTYPE_MESSAGE+5,buf);
                    
                    for(int j=0;j<nb_clients_actuels;j++){
                        if(i==j) // ne pas l'envoyer au client qu'il l'a envoyé 
                            continue;
                        if(clients[j].connected==False)
                            continue;
                        
                        if (send(clients[j].sockfd,buf_envoyer,strlen(buf_envoyer), 0) < 0) {
                            perror("send()");
                            exit(-8);
                        }

                    }
                }
            }
        }

        // si des threads ont récupéré les infos de connection on récupère les infos

        // ----------------------

    }

    free(tid);

    // fermeture des descripteur de socket des clients
    for(int i=0;i<nombre_total_clients;i++)
        if(close(clients[i].sockfd)==-1)
            exit(-7);

    // fermeture de la socket
    if(close(sockfd)==-1)
        exit(-7);

    return 0;
}


    