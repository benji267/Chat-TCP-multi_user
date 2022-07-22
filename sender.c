#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h> 

#define MAX_NAME_SIZE 10
#define MAX_NUMBER_OF_CLIENTS 10
#define MAX_MSG_LENGTH 1024
#define PROTOTYPE_MESSAGE strlen("Message reçu de la part de : ")+MAX_NAME_SIZE
#define PROTOTYPE_MSG "Message reçu de la part de : "
#define LENGTH_PROTOTYPE_MSG PROTOTYPE_MESSAGE-MAX_NAME_SIZE

#define PORT_PAR_DEFAUT 2043


typedef enum Bool{False=0, True=1} Bool ;


Bool peer_to_peer=False;

const char upcoming_connection[]="Connection comming from ";

const char connection_accepted[]="Connection accepted :D\n";

const char disconnected[]="Your conversational-partner is disconnected ZZZZZZZ";

void reset () {
  printf("\033[0m");
}


void print_output(char * buffer)
{
    printf("\n===============================================\n");

    printf("\033[0;33m");

    printf("%s",buffer);
    
    reset();


}

void requester(struct sockaddr_in me);

void receiver(struct sockaddr_in requester);

#if defined(PEERTOPEER)


void requester(struct sockaddr_in me){
    int sockfd;                   // descripteur de socket
    char buf[1024];               // espace necessaire pour stocker le message recu

    char buf_envoyer[1024];

    // taille d'une structure sockaddr_in utile pour la fonction recvfrom
    socklen_t fromlen = sizeof(struct sockaddr_in); 

    struct sockaddr_in client;    // structure d'adresse qui contiendra les param reseaux de l'expediteur


    // creation de la socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    me.sin_port++;

    // association de la socket et des param reseaux du recepteur
    if(bind(sockfd,(struct sockaddr*)&me,sizeof(me)) != 0)
    {
        perror("erreur lors de l'appel a bind -> ");
        exit(-2);
    }

    if(listen(sockfd, 3)==-1)
    {
        perror("erreur lors de l'appel a listen -> ");
        exit(-3);
    }

        int client_d;

        if((client_d=accept(sockfd,(struct sockaddr*)&client,&fromlen))==-1)
        {
            perror("erreur lors de l'appel a listen -> ");
            exit(-3);
        }

        fd_set readfs;
        fd_set readfs2;
        FD_ZERO(&readfs2);
        FD_ZERO(&readfs);
        FD_SET(client_d,&readfs);
        FD_SET(0,&readfs);

        int n;
        
        while(1)
        {

        memset(buf,'\0',1024);
        memset(buf_envoyer,'\0',1024);
            readfs2=readfs;
            if(select(client_d+1,&readfs2,NULL,NULL,NULL)==-1)// après on fait la fonction max
            { 
                perror("erreur lors de l'appel a select -> ");
                exit(-4);
            }

            if(FD_ISSET(0,&readfs2))
            {
                fgets(buf_envoyer,1024,stdin);

                if(!strncmp(buf_envoyer,"/quit",5))
                {
                    close(sockfd);
                    exit(0);
                }
                
                if(send(client_d,buf_envoyer,strlen(buf_envoyer),0) == -1)
                {
                    perror("erreur a l'appel de la fonction sendto -> ");
                    exit(-2);
                }
            }

            if(FD_ISSET(client_d,&readfs2))
            { 
                // reception de la chaine de caracteres
                if((n=recv(client_d,buf,1024,0)) == -1)
                {
                    perror("erreur de reception -> ");
                    exit(-3);
                }

                if(n==0)
                    send(client_d,disconnected,52,0);
                // affichage de la chaine de caracteres recue
                print_output(buf); fflush(stdout);
            }
        }
    

    // fermeture de la socket
    close(sockfd);

    exit(0);
}

void receiver(struct sockaddr_in requester){
        int sockfd;

        sockfd = socket(AF_INET,SOCK_STREAM,0);

        if(sockfd==-1)
        {
            perror("socket\n");
            exit(1);
        }

        char buf[MAX_MSG_LENGTH];

        sleep(3);

        requester.sin_port++;

        if(connect(sockfd,(struct sockaddr*)&requester,sizeof(requester))== -1)
        {
            perror("erreur lors de l'appel a connect -> ");
            exit(-5);
        }

        fd_set readfs,readfs2;
        FD_ZERO(&readfs);
        FD_ZERO(&readfs2);
        FD_SET(0,&readfs);
        FD_SET(sockfd,&readfs);

        while(1)
        {
            memset(buf,'\0',MAX_MSG_LENGTH);

            readfs2=readfs;
            if(select(sockfd+1,&readfs2,NULL,NULL,NULL)==-1)
            { 
                perror("erreur lors de l'appel a select -> ");
                exit(-4);
            }

            if(FD_ISSET(0,&readfs2))
            {
                
                fgets(buf,1024,stdin);

                if(!strncmp(buf,"/quit",5))
                {
                    close(sockfd);
                    exit(0);
                }

                if(send(sockfd,buf,strlen(buf)+1,0) == -1)
                {
                    perror("erreur a l'appel de la fonction sendto -> ");
                    exit(-2);
                }
            }
            if(FD_ISSET(sockfd,&readfs2))
            { 
                int n=0;
                if((n=recv(sockfd,buf,1024,0)) == -1)
                {
                    perror("erreur de reception -> ");
                    exit(-3);
                }
                if(n==MAX_MSG_LENGTH)
                    buf[n-1]='\0';
                else
                    buf[strlen(buf)]='\0';
                
                // affichage de la chaine de caracteres recue
                print_output(buf); fflush(stdout);
            }
    }

    // fermeture de la socket
    close(sockfd);

    exit(0);

}

#endif





int main(int argc, char **argv)
{
    int sockfd;                      // descripteur de socket
    struct sockaddr_in dest;         // structure d'adresse qui contiendra les
                                    // parametres reseaux du destinataire

    char buf[1024];

    // creation de la socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        perror("socket\n");
        exit(1);
    }

    if(argc >=2){
        if(!strncmp(argv[1],"P",1)){
            peer_to_peer=True;
        }
    }
    // initialisation de la structure d'adresse du destinataire :

    // famille d'adresse
    dest.sin_family = AF_INET;

    // adresse IPv4 du destinataire
    inet_aton("127.0.0.1", &(dest.sin_addr));

    // inet_aton( htonl(INADDR_ANY), &(dest.sin_addr));

    // port du destinataire

    dest.sin_port = htons(PORT_PAR_DEFAUT);
    

    if(connect(sockfd,(struct sockaddr*)&dest,sizeof(dest))== -1)
    {
        perror("erreur lors de l'appel a connect -> ");
        exit(-5);
    }

    // les deux set pour les mêmes raisons mentionnées dans le serveur
    fd_set readfs,readfs2;
    FD_ZERO(&readfs);
    FD_ZERO(&readfs2);
    FD_SET(0,&readfs);
    FD_SET(sockfd,&readfs);

    Bool just_joined=True;


    while(1)
    {
        memset(buf,'\0',MAX_MSG_LENGTH);

        readfs2=readfs;
        if(select(sockfd+1,&readfs2,NULL,NULL,NULL)==-1)
        { 
            perror("erreur lors de l'appel a select -> ");
            exit(-4);
        }

        if(FD_ISSET(0,&readfs2))
        {
            
            fgets(buf,1024,stdin);

            if(!strncmp(buf,"/quit",5))
            {
                if(send(sockfd,buf,strlen(buf)+1,0) == -1)
                {
                    perror("erreur a l'appel de la fonction sendto -> ");
                    exit(-2);
                }
                close(sockfd);
                exit(0);
            }
            if(send(sockfd,buf,strlen(buf)+1,0) == -1)
            {
                perror("erreur a l'appel de la fonction sendto -> ");
                exit(-2);
            }
        }
        
        if(FD_ISSET(sockfd,&readfs2))
        { 
            int n=0;

            struct sockaddr_in client_asking_for_connection;
            
            struct sockaddr_in me;

            if(peer_to_peer){

                if((n=recv(sockfd,buf,1024,0)) == -1)
                {
                    perror("erreur de reception -> ");
                    exit(-3);
                }

                print_output(buf); fflush(stdout);

                if(!strncmp(buf,upcoming_connection,24))
                {
                    char answer='y';
                    send(sockfd,&answer,1,0);
                    if(answer=='y' || answer=='O' || answer=='Y' || answer=='o'){
                        
                        if((n=recv(sockfd,&client_asking_for_connection,sizeof(dest),0)) == -1)
                        {
                            perror("erreur de reception -> ");
                            exit(-3);
                        }
                        
                        
                        receiver(client_asking_for_connection);
                    
                    }
                    else{
                        send(sockfd,&answer,1,0);
                    }
                }
                if(!strncmp(buf,connection_accepted,23))
                {
                    if((n=recv(sockfd,&me,sizeof(dest),0)) == -1)
                    {
                        perror("erreur de reception -> ");
                        exit(-3);
                    }
                    
                    requester(me);

                }
                

            }
            else{
                if((n=recv(sockfd,buf,1024,0)) == -1)
                {
                    perror("erreur de reception -> ");
                    exit(-3);
                }


                if(n==MAX_MSG_LENGTH)
                    buf[n-1]='\0';
                else
                    buf[strlen(buf)]='\0';

                
                
                // affichage de la chaine de caracteres recue
               print_output(buf); fflush(stdout);
            }

        }
    }

    // fermeture de la socket
    close(sockfd);

    return 0;
}
