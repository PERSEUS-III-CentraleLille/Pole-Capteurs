/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

#define MAX_LINES 144171
#define MAX_COLUMNS 4

// Structure pour stocker les données lues depuis le fichier
typedef struct {
    char data[MAX_LINES][MAX_COLUMNS];
    int sizeLines;
    int sizeColumns;
    int missingLines;
} data_lines;

// Déclaration des variables pour le calcul du temps d'envoi
struct timeval start, end;
long int temps_envoi;

// Fonction pour configurer le MTU d'un socket L2CAP
int set_l2cap_mtu(int s, uint16_t mtu) {
    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);

    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }

    return status;
}

// Fonction pour convertir les données à partir d'un fichier
data_lines DataConvert(char *lien) {
    FILE *fichier = fopen(lien, "r");

    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier");
        exit(EXIT_FAILURE);
    }

    data_lines dataConverted;
    dataConverted.sizeLines = 0;
    dataConverted.sizeColumns = MAX_COLUMNS;
    dataConverted.missingLines = 0;

    int c1 = 0;  // Compteur de lignes
    int c2 = 0;  // Compteur de colonnes

    while (!feof(fichier)) {
        char currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {  // Si on a un saut de ligne (car int "\n" = 10)
            c1++;
            c2 = 0;

            if (c1 > dataConverted.sizeLines) {
                dataConverted.sizeLines = c1;
            }
        } else {
            if (c1 == 0) {
                dataConverted.sizeLines = 1;
            }

            dataConverted.data[c1][c2] = currentChar;
            c2++;

            if (c2 >= dataConverted.sizeColumns) {
                // Ignorer les caractères supplémentaires dans la même ligne
                while ((currentInt = fgetc(fichier)) != 10 && currentInt != EOF)
                    ;
            }
        }
    }

    fclose(fichier);
    return dataConverted;
}

// Fonction pour calculer le taux d'erreur entre deux ensembles de données
int errorRate(data_lines data1, data_lines data2) {
    float nb_errors = 0, nb_data = 0;
    double loss_rate, error_rate = 0.0;
    int minLines, maxColumns, deltaLines = 0;

    float Lines1 = data1.sizeLines;
    float mLines1 = data1.missingLines;
    float Lines2 = data2.sizeLines;
    float mLines2 = data2.missingLines;

    if (Lines1 >= Lines2) {
        loss_rate = (Lines1 - Lines2 + mLines1 + mLines2) / Lines1 * 100;
        minLines = Lines2;
        deltaLines = Lines1 - Lines2;
        maxColumns = data1.sizeColumns;
    } else {
        loss_rate = (Lines2 - Lines1 + mLines1 + mLines2) / Lines2 * 100;
        minLines = Lines1;
        deltaLines = Lines2 - Lines1;
        maxColumns = data1.sizeColumns;
    }

    printf("Taux de perte de %.10lf pourcents\n", loss_rate);

    int i, j;

    for (i = 0; i < minLines; i++) {
        for (j = 0; j < maxColumns; j++) {
            nb_data++;
            if (data1.data[i][j] != data2.data[i][j]) {
                nb_errors++;
            }
        }
    }

    nb_errors += deltaLines * maxColumns;

    error_rate = nb_errors / nb_data * 100;
    printf("Taux d'erreur de %.10lf pourcents\n", error_rate);

    return 0;
}

int main(int argc, char **argv) {
    // Commande pour exécuter bluetoothctl
    char *bluetoothctl_command = "bluetoothctl";
    FILE *pipe = popen(bluetoothctl_command, "w");

    if (!pipe) {
        perror("Erreur lors de l'ouverture du tube.");
        return 1;
    }

    fprintf(pipe, "power on\n");
    fprintf(pipe, "discoverable yes\n");
    fprintf(pipe, "exit\n");

    fclose(pipe);
    printf("bluetooth ok\n");

    struct sockaddr_l2 loc_addr = {0}, rem_addr = {0};
    int s = 0, client, bytes_read;
    unsigned int opt = sizeof(rem_addr);

    data_lines data;
    data.sizeLines = MAX_LINES;
    data.sizeColumns = MAX_COLUMNS;
    data.missingLines = 0;

    struct sched_param sched_p;
    sched_p.sched_priority = 50;

    if (sched_setscheduler(0, SCHED_RR, &sched_p) == -1) {
        perror("sched_setscheduler \n");
    }

    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    loc_addr.l2_family = AF_BLUETOOTH;
    loc_addr.l2_bdaddr = *BDADDR_ANY;
    loc_addr.l2_psm = htobs(0x1001);
    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s, 1);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    int mtu_value = 15620;
    set_l2cap_mtu(s, mtu_value);

    data_lines initial_data;
    initial_data = DataConvert("/home/PERSEUS2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    printf("fichier ouvert avec lien ok\n");

    char buf[mtu_value];
    memset(buf, 0, mtu_value * sizeof(char));

    FILE *fichier = fopen("test.txt", "w+");
    FILE *resultat = fopen("result.txt", "w+");

    if (fichier == NULL) {
        perror("Impossible d'ouvrir le fichier test.txt");
        exit(EXIT_FAILURE);
    }

    if (resultat == NULL) {
        perror("Impossible d'ouvrir le fichier resultat.txt");
        exit(EXIT_FAILURE);
    }

    printf("ouverture fichier ok\n");

    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    ba2str(&rem_addr.l2_bdaddr, buf);
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    printf("connexion ok\n");

    int check = 1;
    while (check) {
        gettimeofday(&start, NULL);
        printf("initialisation reception ok\n");

        bytes_read = recv(client, buf, sizeof(buf), 0);

        gettimeofday(&end, NULL);
        temps_envoi = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));

        if (bytes_read > 0) {
            fprintf(fichier, "%s", buf);
            fprintf(resultat, "%ld\n", temps_envoi);
        } else {
            fprintf(stderr, "Erreur de lecture depuis le client\n");
            exit(EXIT_FAILURE);
        }

        usleep(50000);  // temps d'attente pour laisser le temps de lire le buffer
    }

    fclose(fichier);
    fclose(resultat);

    // Compare les fichiers et calcule les taux d'erreur
    data_lines received_data = DataConvert("test.txt");
    errorRate(initial_data, received_data);

    close(client);
    close(s);
    return 0;
}
*/
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
    char **data;        // Ensemble de données
    int sizeLines;      // Nombre de lignes de données
    int sizeColumns;    // Nombre de colonnes de données
    int missingLines;   // Nombre de lignes incomplètes
} data_lines;

data_lines DataConvert(char *lien) {  // Fonction de conversion des fichiers txt en char **

    // Ouverture du fichier
    FILE *fichier = fopen(lien, "r");

    // Initialisations
    data_lines dataConverted;
    char **data;
    dataConverted.data = NULL;
    dataConverted.sizeColumns, dataConverted.sizeLines, dataConverted.missingLines = 0;
    int sizeColumns, sizeLines = 0;
    char currentChar;                    // Caractère lu actuellement
    int c1 = 1;                          // Compteur de lignes
    int c2 = 1;                          // Compteur de colonnes
    int c3 = 0;                          // Compteur annexe

    // Allocation mémoire initiale
    data = (char **)malloc(c1 * sizeof(char *));
    data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
    while (!feof(fichier)) {            // Tant qu'on est pas arrivés à la fin du fichier

        // Récupération du caractère lu
        currentChar = fgetc(fichier);
        int currentInt = currentChar;

        if (currentInt == 10) {          // Si on a un saut de ligne (car int "\n" = 10)

            c1++;

            if (c1 == 2) {
                c3 = c2;                 // Stockage du nombre de colonnes "normal" du fichier
                dataConverted.sizeColumns = c3;
            } else {
                if (c2 == c3 + 1 || c2 == c3 + 2 || c2 == c3 + 3 || c2 == c3 - 1 || c2 == c3 - 2 || c2 == c3 - 3) {
                    // Présence (ou non) des - dans les données
                    c3 = c2;             // Stockage du nouveau nombre "normal" de colonnes
                    dataConverted.sizeColumns = c3;
                }
                if (c2 != c3) {
                    printf("Il manque %d caracteres dans la ligne %d du fichier %s\n", abs(c2 - c3), c1 - 1, lien);
                    dataConverted.missingLines++;
                }
            }
            c2 = 1;                      // Retour à la première colonne

            // Réallocation mémoire pour la nouvelle ligne
            data = (char **)realloc(data, c1 * sizeof(char *));
            data[c1 - 1] = (char *)malloc(c2 * sizeof(char));
        } else {

            c2++;

            // Réallocation mémoire pour la nouvelle colonne
            data[c1 - 1] = (char *)realloc(data[c1 - 1], c2 * sizeof(char));
            data[c1 - 1][c2 - 2] = currentChar;
        }
    }

    dataConverted.data = data;
    dataConverted.sizeLines = c1;
    return dataConverted;
};

int set_l2cap_mtu(int s, uint16_t mtu) {  // Fonction qui change le MTU d'un socket

    struct l2cap_options opts;
    int optlen = sizeof(opts);
    int status = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen);
    if (status == 0) {
        opts.omtu = opts.imtu = mtu;
        status = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen);
    }
    return status;
};

int envoie(data_lines data) {  // Fonction d'envoi de données

    // Création du socket dans le but de connecter entre elles les 2 raspberry
    struct sockaddr_l2 addr = {0};
    int s, status;
    char dest[18] = "DC:A6:32:78:6D:92";

    // Allocation du socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (s == -1) {
        perror("socket");
        exit(1);
    }

    // Modification du MTU
    int mtu_value = 15620;  // Valeur modifiée par le fichier python
    set_l2cap_mtu(s, mtu_value);

    // Connexion entre les 2 raspberry
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba(dest, &addr.l2_bdaddr);
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    if (status == -1) {
        perror("connect");
        exit(2);
    }

    // Initialisations d'envoi
    int i, j = 0;
    char paquet[mtu_value];
    memset(paquet, 0, mtu_value * sizeof(char));  // Initialisation du paquet avec des zéros

    // Envoi de données
    if (0 == status) {
        printf("Connexion réussie\n");
        int nb_data = 0;                // Nombre de données déjà inscrites dans le paquet
        for (i = 0; i < data.sizeLines; i++) {
            if ((nb_data + data.sizeColumns) >= mtu_value) {  // On prend le parti de ne pas transmettre des bouts partiels de ligne
                send(s, paquet, sizeof(paquet), 0);            // Envoi du paquet
                memset(paquet, 0, mtu_value * sizeof(char));   // Initialisation du paquet avec des zéros
                nb_data = 0;                                     // Remise à zéro du nombre de données dans le paquet
            }
            int j = 0;
            int c = data.data[i][j];
            while (c == 101 || c == 43 || c == 46 || c == 32 || c == 45 || (c >= 48 && c <= 57)) {
                // Si le caractère correspond aux données à transmettre
                paquet[nb_data] = data.data[i][j];  // Ajout du caractère à paquet
                nb_data++;                          // Incrémentation du nombre de données ajoutées à paquet
                j++;
                c = data.data[i][j];
            }
            char arobase = 64;
            paquet[nb_data] = arobase;  // Ajout d'un @ en tant que saut de ligne
            nb_data++;
        }
        send(s, "stop", 4, 0);  // Fin de la transmission du fichier
    }
    if (status < 0) {
        fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
        perror("Connexion echouée\n");
    }
    close(s);  // Fermeture du socket
    return 0;
}

int main(int argc, char **argv) {

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

    printf("%d", 1);
    data_lines data;
    data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
    envoie(data);

    return 0;
}

/*
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
#include <stdio.h>



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
};
	 
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
	char dest[22] = "dc:a6:32:78:6d:92";
	
	//Allocation du socket
	s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP) ;

	if (s == -1) { //tom
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

	if (status == -1) {   //  tom
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
		printf("Temps de transmission : %ld micro seconds\n",((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));  //Temps total de transmission
	}
	if( status < 0 ) {
		fprintf(stderr, "error code %d: %s\n", errno, strerror(errno));
		perror( "Connexion echouée\n" );
	}
	close (s);  													//Fermeture du socket
	return 0;
}


int main(int argc , char ** argv) {

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
	  data = DataConvert("/home/PERSEUS_2023/Pole-Capteurs/client/Numerical_Results_capteur_v1.txt");
  	envoie(data);
	
}

*/