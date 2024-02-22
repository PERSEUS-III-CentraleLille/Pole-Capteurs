#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1000

void printDifference(int lineNum, const char *file1Line, const char *file2Line) {
    printf("Difference at line %d:\n", lineNum);
    printf("File 1: %s", file1Line);
    printf("File 2: %s", file2Line);
}

int compareFiles(FILE *file1, FILE *file2) {
    char line1[MAX_LINE_LENGTH], line2[MAX_LINE_LENGTH];
    int lineNum = 0;

    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL && fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        lineNum++;

        if (strcmp(line1, line2) != 0) {
            printDifference(lineNum, line1, line2);
            return lineNum;
        }
    }

    // Check for additional lines in either file
    while (fgets(line1, MAX_LINE_LENGTH, file1) != NULL) {
        lineNum++;
        printDifference(lineNum, line1, "");
        return lineNum;
    }

    while (fgets(line2, MAX_LINE_LENGTH, file2) != NULL) {
        lineNum++;
        printDifference(lineNum, "", line2);
        return lineNum;
    }

    return 0;
}

int main() {
    FILE *file1, *file2;
    char filename1[MAX_LINE_LENGTH], filename2[MAX_LINE_LENGTH];

    // Saisie des noms de fichiers à comparer
    printf("Entrez le nom du premier fichier : ");
    scanf("%s", filename1);

    printf("Entrez le nom du deuxième fichier : ");
    scanf("%s", filename2);

    // Ouverture des fichiers
    file1 = fopen(filename1, "r");
    file2 = fopen(filename2, "r");

    // Vérification de l'ouverture des fichiers
    if (file1 == NULL || file2 == NULL) {
        printf("Erreur lors de l'ouverture des fichiers.\n");
        return 1;
    }

    // Comparaison des fichiers
    int firstDifference = compareFiles(file1, file2);

    // Fermeture des fichiers
    fclose(file1);
    fclose(file2);

    // Affichage du résultat
    if (firstDifference == 0) {
        printf("Les fichiers sont identiques.\n");
    } else {
        printf("Première différence rencontrée à la ligne %d.\n", firstDifference);
    }

    return 0;
}

