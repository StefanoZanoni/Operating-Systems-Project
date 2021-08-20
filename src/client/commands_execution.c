#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "../../headers/commands_execution.h"
#include "../../headers/queue.h"

static unsigned int set_stdout = 0;
static unsigned int time_to_wait = 0;

static void free_command(argnode_t *command) {

	if (command) {

		if (command->arg) {

			if (command->arg->args) {

				int i = 0;
				while (command->arg->args[i]) {
					free(command->arg->args[i]);
					command->arg->args[i++] = NULL;
				}

				free(command->arg->args);
				command->arg->args = NULL;

			}

			free(command->arg);
			command->arg = NULL;

		}

		free(command);

	}

}

void execute_h() {

	char *help[][2] = {
    			{"-h",               "visualizza help delle opzioni\n\n"},
            {"-f <filename>",    "specifica il nome del socket AF_UNIX a cui connettersi\n\n"},
            {"-w dirname[,n=0]", "invia al server i file nella cartella ‘dirname’, ovvero effettua una richiesta di scrittura al "
                                 "server per i file.\nSe la directory ‘dirname’ contiene altre directory, queste vengono visitate ricorsivamente "
                                 "fino a quando non si leggono ‘n‘ file;\nse n=0 (o non è specificato) non c’è un limite superiore al numero di "
                                 "file da inviare al server (tuttavia non è detto che il server possa scriverli tutti).\n\n"},
            {"-W file1[,file2]", "lista di nomi di file da scrivere nel server separati da ‘,’\n\n"},
            {"-D dirname",       "cartella in memoria secondaria dove vengono scritti (lato client) i file che il server rimuove a "
                                 "seguito di capacity misses (e che erano stati modificati da operazioni di scrittura) per servire le scritture "
                                 "richieste attraverso l’opzione ‘-w’ e ‘-W’.\nL’opzione ‘-D’ deve essere usata quindi congiuntamente "
                                 "all’opzione ‘-w’ o ‘-W’, altrimenti viene generato un errore.\nSe l’opzione ‘-D’ non viene specificata, tutti i "
                                 "file che il server invia verso il client a seguito di espulsioni dalla cache, vengono buttati via.\nAd esempio, "
                                 "supponiamo i seguenti argomenti a linea di comando “-w send -D store”, e supponiamo che dentro la cartella "
                                 "‘send’ ci sia solo il file ‘pippo’.\nInfine supponiamo che il server, per poter scrivere nello storage il file "
                                 "‘pippo’ deve espellere il file ‘pluto’ e ‘minni’.\nAllora, al termine dell’operazione di scrittura, la cartella "
                                 "‘store’ conterrà sia il file ‘pluto’ che il file ‘minni’.\nSe l’opzione ‘-D’ non viene specificata, allora il server "
                                 "invia sempre i file ‘pluto’ e ‘minni’ al client, ma questi vengono buttati via\n\n"},
            {"-r file1[,file2]", "lista di nomi di file da leggere dal server separati da ‘,’ (esempio: -r pippo,pluto,minni)\n\n"},
            {"-R [n=0]",         "tale opzione permette di leggere ‘n’ file qualsiasi attualmente memorizzati nel server;\nse n=0 (o "
                                 "non è specificato) allora vengono letti tutti i file presenti nel server\n\n"},
            {"-d dirname",       "cartella in memoria secondaria dove scrivere i file letti dal server con l’opzione ‘-r’ o ‘-R’.\n"
                                 "L’opzione -d va usata congiuntamente a ‘-r’ o ‘-R’, altrimenti viene generato un errore;\nSe si utilizzano le "
                                 "opzioni ‘-r’ o ‘-R’senza specificare l’opzione ‘-d’ i file letti non vengono memorizzati sul disco\n\n"},
            {"-t time",          "tempo in millisecondi che intercorre tra l’invio di due richieste successive al server (se non "
                                 "specificata si suppone -t 0, cioè non c’è alcun ritardo tra l’invio di due richieste consecutive)\n\n"},
            {"-l file1[,file2]", "lista di nomi di file su cui acquisire la mutua esclusione\n\n"},
            {"-u file1[,file2]", "lista di nomi di file su cui rilasciare la mutua esclusione\n\n"},
            {"-c file1[,file2]", "lista di file da rimuovere dal server se presenti\n\n"},
            {"-p",               "abilita le stampe sullo standard output per ogni operazione.\nLe stampe associate alle varie operazioni "
                                 "riportano almeno le seguenti informazioni: tipo di operazione, file di riferimento, esito e dove è rilevante i "
                                 "bytes letti o scritti.\n\n"},
   };

   int size = sizeof help / sizeof help[0];

   for (int i = 0; i < size; i++) {

      printf("%s: %s\n", help[i][0], help[i][1]);

   }
}
   


static void execute_f(char **args) {

	
}

static void execute_w(char **args) {

	
}

static void execute_W(char **args) {

	
}

static void execute_D(char **args) {

	
}

static void execute_r(char **args) {

	
}

static void execute_R(char **args) {

	
}

static void execute_d(char **args) {

	
}

static void execute_t(char **args) {

	
}

static void execute_l(char **args) {

	
}

static void execute_u(char **args) {

	
}

static void execute_c(char **args) {

	
}

static void execute_p() {

	
}

int execute_command(argnode_t *command, argsQueue_t *queue) {

	switch (command->arg->option) {

		case 'h': {

			execute_h();
			//cleanup(queue, NULL);
			//exit(EXIT_SUCCESS);

		} break;

		case 'f': {

			execute_f(command->arg->args);

		} break;

		case 'w': {

			execute_w(command->arg->args);

		} break;

		case 'W': {

			execute_W(command->arg->args);

		} break;

		case 'D': {

			execute_D(command->arg->args);	

		} break;

		case 'r': {

			execute_r(command->arg->args);

		} break;

		case 'R': {

			execute_R(command->arg->args);

		} break;

		case 'd': {

			execute_d(command->arg->args);	

		} break;

		case 't': {

			execute_t(command->arg->args);	

		} break;

		case 'l': {

			execute_l(command->arg->args);	

		} break;

		case 'u': {

			execute_u(command->arg->args);

		} break;

		case 'c': {

			execute_c(command->arg->args);

		} break;

		case 'p': {

			execute_p();

		} break;

	}

	free_command(command);

	return 0;
}