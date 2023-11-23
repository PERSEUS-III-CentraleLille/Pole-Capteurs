#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

typedef struct {
    char ** data;		//Ensemble de données
    int sizeLines;		//Nombre de lignes de données
    int sizeColumns;	//Nombre de colonnes de données
    int missingLines;	//Nombre de lignes incomplètes
} data_lines;

data_lines DataConvert ( char * lien ){    //Fonction de conversion des fichiers txt en char **

    //Ouverture du fichier
    FILE * fichier = fopen(lien, "r");

    //Initialisations
    data_lines dataConverted;
    char ** data;
    dataConverted.data = NULL;
    dataConverted.sizeColumns, dataConverted.sizeLines, dataConverted.missingLines = 0;
    int sizeColumns, sizeLines = 0;
    char currentChar;    				//Caractère lu actuellement
    int c1 = 1;  						//Compteur de lignes
    int c2 = 1;  						//Compteur de colonnes
    int c3 = 0;  						//Compteur annexe

    //Allocation mémoire initiale
    data = (char **) malloc (c1 * sizeof(char *));
    data[c1 - 1] = (char *) malloc (c2 * sizeof(char));
    while ( ! feof(fichier)) {  		//Tant qu'on est pas arrivés à la fin du fichier

        //Récupération du caractère lu
        currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {    		//Si on a un saut de ligne (car int "\n" = 10)

            c1++;

            if (c1==2){      
                c3 = c2;       			//Stockage du nombre de colonnes "normal" du fichier
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3){    
										// Présence (ou non) des - dans les données
                    c3 = c2;     		//Stockage du nouveau nombre "normal" de colonnes
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3){
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(c2-c3), c1-1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 1;      				//Retour à la première colonne

            //Réallocation mémoire pour la nouvelle ligne
            data = (char **) realloc (data, c1 * sizeof(char *));
            data [c1 - 1] = (char *) malloc (c2 * sizeof(char));
        } else {

            c2++;

            //Réallocation mémoire pour la nouvelle colonne
            data[c1 - 1] = (char *) realloc (data[c1 - 1], c2 * sizeof(char));
            data[c1 - 1][c2 - 2] = currentChar;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;
    return dataConverted;
}
	 
int set_l2cap_mtu( int s , uint16_t mtu ) { //Fonction qui change le MTU d'un socket

	struct l2cap_options opts ;
	int optlen = sizeof(opts ) ;
	int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
	if( status == 0) {
		opts.omtu = opts.imtu = mtu ;
		status = setsockopt( s , SOL_L2CAP , L2CAP_OPTIONS , &opts ,optlen ) ;
	}
	return status ;
};

int envoie(data_lines data){   //Fonction d'envoi de données

	//Création du socket dans le but de connecter entre elles les 2 raspberry
	struct sockaddr_l2 addr = { 0 } ;
	int s , status ;
	char dest[18] = "dc:a6:32:78:6d:92";
	
	//Allocation du socket
	s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP) ;
        if (s == -1) { //M. Bourdeaud'hui
		perror("socket"); 
		exit(1);
	}
	//Modification du MTU
	int mtu_value = 15620;    										//Valeur modifiée par le fichier python
   	set_l2cap_mtu( s , mtu_value );

	//Connexion entre les 2 raspberry
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(0x1001);
	str2ba( dest , &addr.l2_bdaddr ) ;
	status = connect (s , (struct sockaddr *)&addr , sizeof(addr ));
	
	if (status == -1) {   //  M.Bourdeau'hui
		perror("connect"); 
		exit(2); 
	}
	//Initialisations d'envoi
	int i,j = 0;
	struct timeval start, end;  									//Initialisation de variables de temps
	char paquet[mtu_value];
	memset(paquet,0,mtu_value * sizeof(char));  					//Initialisation du paquet avec des zéros
	
	//Envoi de données
	if( 0 == status ) {
		printf("Connexion réussie\n");
		gettimeofday(&start, NULL);									//Initialisation du temps de départ
		int nb_data = 0;               								//Nombre de données déjà inscrites dans le paquet
		for (i=0; i<data.sizeLines;i++){
		    if ((nb_data+data.sizeColumns) >= mtu_value){        	//On prend le parti de ne pas transmettre des bouts partiels de ligne
				send(s, paquet, sizeof(paquet), 0);             	//Envoi du paquet
				memset(paquet,0,mtu_value * sizeof(char));  		//Initialisation du paquet avec des zéros
				nb_data = 0;                                    	//Remise à zéro du nombre de données dans le paquet
		    }
		    int j=0;
		    int c = data.data[i][j];
		    while(c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)){	
																	//Si le caractère correspond aux données à transmettre
				paquet[nb_data] = data.data[i][j];   				//Ajout du caractère à paquet
				nb_data++;                          				//Incrémentation du nombre de données ajoutées à paquet
				j++;
				c = data.data[i][j];
		    }
		    char arobase = 64;
		    paquet[nb_data] = arobase;								//Ajout d'un @ en tant que saut de ligne
		    nb_data++;
		}		
		send(s,"stop",4,0);  										//Fin de la transmission du fichier
		gettimeofday(&end, NULL);									//Initialisation du temps de fin
		printf("Temps de transmission : %ld micro seconds\n",
		((end.tv_sec * 1000000 + end.tv_usec) -
		(start.tv_sec * 1000000 + start.tv_usec)));  				//Temps total de transmission
	}
	if( status < 0 ) {
		fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
		perror( "Connexion echouée\n" );
	}
	close (s);  													//Fermeture du socket
	return 0;
}

int main(int argc , char ** argv){
  // Commande pour exécuter bluetoothctl
    char *bluetoothctl_command = "bluetoothctl";

    // Ouvrir un tube pour envoyer des commandes à bluetoothctl
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    // Envoyer les commandes à bluetoothctl
    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "pair DC:A6:32:78:6D:92\n");
    fprintf(pipe, "exit\n");

    // Fermer le tube
    fclose(pipe);

    printf("%d",1);
	data_lines data;
	data = DataConvert("/home/pi/Desktop/Numerical_Results_capteur.txt");
	envoie(data);
	
}
