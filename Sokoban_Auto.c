#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define NB_LIGNES 12
#define NB_COLONNES 12
#define CARACTERE 15
#define MAX_DEP 1000
#define MAX_SAVE 31

#define MUR '#'
#define CAISSE '$'
#define CIBLE '.'
#define SOKOBAN '@'
#define CAISSE_CIBLE '*'
#define SOKOBAN_CIBLE '+'
#define VIDE ' '
#define EXT ".dep"
#define NON 'n'
#define OUI 'y'
#define DROITE 'd'
#define GAUCHE 'g'
#define HAUT 'h'
#define BAS 'b'
#define DROITE_MAJ 'D'
#define GAUCHE_MAJ 'G'
#define HAUT_MAJ 'H'
#define BAS_MAJ 'B'
#define BORNE_NEGATIVE -1
#define BORNE_POSITIVE 1
#define DIST_CAISSE 2


const int LIMITE_ZOOM_MIN = 1;
const int LIMITE_ZOOM_MAX = 3;

typedef char t_plateau[NB_LIGNES][NB_COLONNES];
typedef char t_tabDeplacement[MAX_DEP];

/* --- Prototypes --- */
void charger_partie(t_plateau plateau, char fichier[]);
void afficher_plateau(t_plateau plateau, char touche);
void afficher_entete(t_plateau plateau, char niveau[CARACTERE], int nbdeplacement);
bool deplacer(t_plateau plateau, char touche, int *nbDeplacement);
void chargerDeplacements(t_tabDeplacement t, char fichier[], int * nb);
bool gagne(t_plateau plateau);
bool test_deplacement(t_tabDeplacement dep, int *i, int nbDep);
bool bon_deplacement(t_plateau plateau, char touche, int *nbDeplacement, t_tabDeplacement listDep);
int kbhit();
void enregistrerDeplacements(t_tabDeplacement t, int nb, char fic[]);


/* ---------------- main ---------------- */
int main(void)
{
    t_plateau tab;
    char niveau[CARACTERE];
    char deplacements[CARACTERE];
    t_tabDeplacement listeDeplacements;
    t_tabDeplacement listeBonDeplacement;
    t_tabDeplacement listeDepFin;
    char touche = '\0';
    int nbDeplacementInitial = 0;
    int nbDeplacement = 0;
    int bonDeplacement = 0;
    int indiceBonDep = 0;
    int indice = 0;

    printf("Quel niveau voulez-vous charger ? ");   //demande du niveau et de la chaine de déplacement à utiliser
    scanf("%s", niveau);
    printf("Quel déplacement voulez-vous charger ? ");
    scanf("%s", deplacements);
    
    chargerDeplacements(listeDeplacements, deplacements, &nbDeplacement);   //chagement du niveau pour vérifier quels déplacements seront réellement joués
    nbDeplacementInitial = nbDeplacement;
    indice = nbDeplacement;
    charger_partie(tab, niveau);
    for(int i = 0; i < nbDeplacement; i++){
        touche = listeDeplacements[i];
        if (deplacer(tab, touche, &indice)){
            listeBonDeplacement[indiceBonDep] = listeDeplacements[i];
            indiceBonDep++;
        }
    }
    nbDeplacement = indiceBonDep;   //suppression des déplacements qui sont inutiles
    indiceBonDep = 0;
    for (int j = 0; j < nbDeplacement; j++) {
        if (test_deplacement(listeBonDeplacement, &j, nbDeplacement)) {
            listeDepFin[indiceBonDep++] = listeBonDeplacement[j];
        }
    }


    nbDeplacement = indiceBonDep;  //affichage des déplacements optimisés
    charger_partie(tab, niveau);
    system("clear");
    afficher_entete(tab, niveau, nbDeplacement);
    afficher_plateau(tab, touche);
    indice = 0;
    while ((indice < indiceBonDep)){
        system("clear");
        touche = listeDepFin[indice];
        deplacer(tab, touche, &bonDeplacement);
        afficher_entete(tab, niveau, bonDeplacement);
        afficher_plateau(tab, touche);
        usleep(250000);
        indice++;
    }
    if (gagne(tab)){   //teste si la chaîne de déplacments est une solution pour notre niveau
        printf("La suite de déplacements %s est bien une solution pour la partie %s.\n",deplacements, niveau);
        printf("Elle contient initialement %d caractères. Après optimisation elle contient %d caractères. Souhaitez-vous l’enregistrer (y/n) : ", nbDeplacementInitial, nbDeplacement);
        touche = '\0';
        int c;
        while ((c = getchar()) != '\n' && c != EOF);  // vider stdin 
        while(kbhit()==0){
        }
        touche = getchar();
        while(touche != OUI && touche != NON){
                printf("Touche incorrecte, veulliez taper une réponse valide.\n");
                while(kbhit() == 0){
                }
                touche = getchar();
            }
            if (touche == OUI){
                printf("Quelle nom voulez-vous donner à votre sauvegarde ? (30 caractères maximum) : ");
                char save[MAX_SAVE];
                scanf("%s", save);
                while (strlen(save) > (MAX_SAVE - 1)){
                    printf("Trop de caractères ! Tapez un nouveau nom : ");
                    scanf("%s", save);
                }
                strcat(save,EXT);
                enregistrerDeplacements(listeDepFin, bonDeplacement, save);
            }
            else{
                printf("\nDommage, une prochaine fois peut-être...");
            }
    }
    else{
        printf("La suite de déplacements %s N'EST PAS une solution pour la partie %s.\n",deplacements, niveau);
    }
    return EXIT_SUCCESS;
}

bool test_deplacement(t_tabDeplacement dep, int *i, int nbDep){
    int dx = 0, dy = 0;
    int pos = *i;

    // si majuscule alors on garde
    if (isupper(dep[pos]))
        return true;

    // on accumule tant qu'on est en minuscule
    while (pos < nbDep && islower(dep[pos])) {
        switch (dep[pos]) {
            case DROITE: dy++; break;
            case GAUCHE: dy--; break;
            case HAUT: dx--; break;
            case BAS: dx++; break;
        }

        pos++;

        // retour exact au point de départ alors séquence inutile
        if (dx == 0 && dy == 0) {
            *i = pos - 1; // on saute toute la séquence
            return false;
        }
    }

    return true;
}




void chargerDeplacements(t_tabDeplacement t, char fichier[], int * nb){
    FILE * f;
    char dep;
    *nb = 0;

    f = fopen(fichier, "r");
    if (f==NULL){
        printf("FICHIER NON TROUVE\n");
    } else {
        fread(&dep, sizeof(char), 1, f);
        if (feof(f)){
            printf("FICHIER VIDE\n");
        } else {
            while (!feof(f)){
                t[*nb] = dep;
                (*nb)++;
                fread(&dep, sizeof(char), 1, f);
            }
        }
    }
    fclose(f);
}

/* ---------------- afficher_plateau ---------------- */
void afficher_plateau(t_plateau plateau, char touche)
{
    // Affichage avec zoom
    for (int i = 0; i < NB_LIGNES; i++)
    {
        for (int j = 0; j < NB_COLONNES; j++)
        {
            char c = plateau[i][j];
            if (c == SOKOBAN_CIBLE)
                c = SOKOBAN; // joueur sur cible
            if (c == CAISSE_CIBLE)
                c = CAISSE; // caisse sur cible
            printf("%c", c);
        }
        printf("\n");
    }
}

/* ---------------- afficher_entete ---------------- */
void afficher_entete(t_plateau plateau, char niveau[CARACTERE], int nbDeplacement)
{
    (void)plateau; // silence warning si non utilisé
    printf("Vous êtes au niveau : %s\n", niveau);
    printf("Nombre de déplacements: %d\n", nbDeplacement);
    putchar('\n');
}

/* ---------------- deplacer ----------------
   - effectue le déplacement si possible
   - mémorise le code du déplacement dans historique (hist[indiceHist++])
   - nbDeplacement est incrémenté seulement si déplacement effectué
*/
bool deplacer(t_plateau plateau, char touche, int *nbDeplacement)
{
    int i, j, x = BORNE_NEGATIVE, y = BORNE_NEGATIVE;

    // trouver le joueur
    for (i = 0; i < NB_LIGNES; i++)
    {
        for (j = 0; j < NB_COLONNES; j++)
        {
            if (plateau[i][j] == SOKOBAN || plateau[i][j] == SOKOBAN_CIBLE)
            {
                x = i;
                y = j;
                break;
            }
        }
        if (x != BORNE_NEGATIVE)
            break;
    }
    if (x == BORNE_NEGATIVE)
        return false; // sécurité

    // direction
    int dx = 0, dy = 0;
    if (touche == HAUT || touche == HAUT_MAJ)
        dx = BORNE_NEGATIVE;
    else if (touche == BAS || touche == BAS_MAJ)
        dx = BORNE_POSITIVE;
    else if (touche == GAUCHE || touche == GAUCHE_MAJ)
        dy = BORNE_NEGATIVE;
    else if (touche == DROITE || touche == DROITE_MAJ)
        dy = BORNE_POSITIVE;
    else
        return false; // touche non gérée

    //test pour savoir si la lettre est en majuscule
    bool en_maj = false;
    if (toupper(touche) == touche){
        en_maj = true;
    }

    int nx = x + dx, ny = y + dy;
    int nx2 = x + DIST_CAISSE * dx, ny2 = y + DIST_CAISSE * dy;

    // contrôle de bornes
    if (nx < 0 || nx >= NB_LIGNES || ny < 0 || ny >= NB_COLONNES)
        return false;

    char caseDevant = plateau[nx][ny];

    // si mur => blocage
    if (caseDevant == MUR)
        return false;

    // CAS : il y a une caisse devant -> on doit tenter de la pousser
    if ((caseDevant == CAISSE || caseDevant == CAISSE_CIBLE) && en_maj)
    {
        // contrôle des bornes arrières
        if (nx2 < 0 || nx2 >= NB_LIGNES || ny2 < 0 || ny2 >= NB_COLONNES)
            return false;
        char caseApresCaisse = plateau[nx2][ny2];
        if (caseApresCaisse != VIDE && caseApresCaisse != CIBLE)
            return false; // bloquée

        // pousser la caisse
        if (caseApresCaisse == CIBLE)
            plateau[nx2][ny2] = CAISSE_CIBLE;
        else
            plateau[nx2][ny2] = CAISSE;

        // remettre l'ancienne case de la caisse (celle en nx,ny) en VIDE ou CIBLE selon son état
        if (caseDevant == CAISSE_CIBLE)
            plateau[nx][ny] = CIBLE; 
        else
            plateau[nx][ny] = VIDE;

        // si la case nx,ny est CIBLE (on l'a peut-être restaurée juste au-dessus), le joueur devient SOKOBAN_CIBLE
        if (plateau[nx][ny] == CIBLE)
            plateau[nx][ny] = SOKOBAN_CIBLE;
        else
            plateau[nx][ny] = SOKOBAN;

        // remettre l'ancienne case du joueur (x,y) selon si il était sur cible
        if (plateau[x][y] == SOKOBAN_CIBLE)
            plateau[x][y] = CIBLE;
        else
            plateau[x][y] = VIDE;
        (*nbDeplacement)++;
        return true;
    }

    // maintenant déplacer le joueur dans nx,ny
    // si la case nx,ny est CIBLE (on l'a peut-être restaurée juste au-dessus), le joueur devient SOKOBAN_CIBLE
    if (!en_maj && ((caseDevant != CAISSE) && (caseDevant != CAISSE_CIBLE))){
        if (plateau[nx][ny] == CIBLE)
            plateau[nx][ny] = SOKOBAN_CIBLE;
        else
            plateau[nx][ny] = SOKOBAN;

        // remettre l'ancienne case du joueur (x,y) selon si il était sur cible
        if (plateau[x][y] == SOKOBAN_CIBLE)
            plateau[x][y] = CIBLE;
        else
            plateau[x][y] = VIDE;
        (*nbDeplacement)++;
        return true;
    }
    return false;

}
    

/* ---------------- gagne ---------------- */
bool gagne(t_plateau plateau)
{
    bool gagner = true;
    for (int i = 0; i < NB_LIGNES; i++)
    {
        for (int j = 0; j < NB_COLONNES; j++)
        {
            // si une cible reste ('.') : pas gagné (on attend plus simple : toutes cibles recouvertes par caisses)
            if (plateau[i][j] == CIBLE)
                gagner = false;
            // ou si des caisses non sur cible existent -> dépend de ta règle ; ici on accepte quand plus de CIBLE
        }
    }
    return gagner;
}

/* ---------------- fichiers plateau ---------------- */
void charger_partie(t_plateau plateau, char fichier[]){
    FILE * f;
    char finDeLigne;

    f = fopen(fichier, "r");
    if (f==NULL){
        printf("ERREUR SUR FICHIER");
        exit(EXIT_FAILURE);
    } else {
        for (int ligne=0 ; ligne<NB_LIGNES ; ligne++){
            for (int colonne=0 ; colonne<NB_COLONNES ; colonne++){
                fread(&plateau[ligne][colonne], sizeof(char), 1, f);
            }
            fread(&finDeLigne, sizeof(char), 1, f);
        }
        fclose(f);
    }
}

int kbhit(){
	// la fonction retourne :
	// 1 si un caractere est present
	// 0 si pas de caractere présent
	int unCaractere=0;
	struct termios oldt, newt;
	int ch;
	int oldf;

	// mettre le terminal en mode non bloquant
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
	ch = getchar();

	// restaurer le mode du terminal
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
 
	if(ch != EOF){
		ungetc(ch, stdin);
		unCaractere=1;
	}
	return unCaractere;
}

void enregistrerDeplacements(t_tabDeplacement t, int nb, char fic[]){
    FILE * f;

    f = fopen(fic, "w");
    fwrite(t,sizeof(char), nb, f);
    fclose(f);
}