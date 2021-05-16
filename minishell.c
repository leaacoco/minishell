/**************************************/
/*            minishell.c             */
/**************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "analex.h"

#define TAILLE_MAX 100 /* taille max d'un mot */
#define ARGS_MAX 10    /* nombre maximum d'arguments a une commande */

/* Execute la commande donnee par le tableau argv[] dans un nouveau processus ;
 * les deux premiers parametres indiquent l'entree et la sortie de la commande,
 * a rediriger eventuellement.
 * Renvoie le numero du processus executant la commande
 *
 * FONCTION À MODIFIER/COMPLÉTER.
 *
*/
pid_t execute(int entree, int sortie, char* argv[], int to_close) {
  pid_t pid = fork();
  if(pid == 0) {
    //fils
    //printf("executing %s\n", argv[0]);

    if(entree != STDIN_FILENO) {
      dup2(entree, STDIN_FILENO);
      close(entree);
    }
    if(sortie != STDOUT_FILENO) {
      dup2(sortie, STDOUT_FILENO);
      close(sortie);
    }
    if(to_close != 0) {
      close(to_close);
    }
    execvp(argv[0], argv);
    return pid;
  } 
  return pid;
}

static void free_arg(char *argv[]) {
  for(int i = 0; i < ARGS_MAX; i++) {
    if(argv[i] == NULL)
      return;
    free(argv[i]);
  }
}

/* Lit et execute une commande en recuperant ses tokens ;
 * les deux premiers parametres indiquent l'entree et la sortie de la commande ;
 * pid contient le numero du processus executant la commande et
 * background est non nul si la commande est lancee en arriere-plan
 *
 * FONCTION À MODIFIER/COMPLÉTER.
*/
TOKEN commande(int entree, int sortie, pid_t* pid, int* background) {
  TOKEN t;
  char *argv[ARGS_MAX] = { NULL };
  int nb_arg = 0;
  *background = 0;
  while(1) {
    char word[TAILLE_MAX];  
    t = getToken(word);
    switch(t) {
      case T_WORD:
        argv[nb_arg++] = strdup(word);
        break;
      case T_AMPER:
        *background = 1;
      case T_SEMI:
      case T_NL:
        if(nb_arg == 0) {
          return t;
        }
        argv[nb_arg] = NULL;
        *pid = execute(entree, sortie, argv, 0);
        free_arg(argv);
        return T_NL;
      case T_GT:
        getToken(word);
        sortie = open(word, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        break;
      case T_GTGT:
        getToken(word);
        sortie = open(word, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        break;
      case T_LT:
        getToken(word);
        entree = open(word, O_RDONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        break;
      case T_BAR: {
        int tube[2];
        pipe(tube);
        execute(entree, tube[1], argv, tube[0]);
        close(tube[1]);
        commande(tube[0], sortie, pid, background);
        return T_NL;
      }
      case T_EOF:
        return T_EOF;
    }
  }
}

/* Retourne une valeur non-nulle si minishell est en train de s'exécuter en mode interactif, 
 * c'est à dire, si l'affichage de minishell n'est pas redirigé vers un fichier.
 * (Important pour les tests automatisés.)
 *
 * NE PAS MODIFIER CETTE FONCTION.
 */
int is_interactive_shell() {
  return isatty(1);
}

/* Affiche le prompt "minishell>" uniquement si l'affichage n'est pas redirigé vers un fichier. 
 * (Important pour les tests automatisés.)
 *
 * NE PAS MODIFIER CETTE FONCTION.
 */
void print_prompt() {
  if(is_interactive_shell()) { 
    printf("mini-shell>");
    fflush(stdout);
  }  
}

/* Fonction main 
 * FONCTION À MODIFIER/COMPLÉTER.
 */
int main(int argc, char* argv[]) {
	TOKEN t;
	pid_t pid;
	int background = 0;
	int status = 0;


	print_prompt(); // affiche le prompt "minishell>" 
	
	while ( (t = commande(0, 1, &pid, &background)) != T_EOF) {
	  if(!background) {
      waitpid(pid, &status, 0);
      status = WEXITSTATUS(status);
    }
	  if (t == T_NL) {
	    print_prompt(); 
	  }
	}

	if(is_interactive_shell())
	  printf("\n") ;

	return status; // Attention à la valeur de retour du wait, voir man wait et la macro WEXITSTATUS
}  	 	 	  		 	 					
