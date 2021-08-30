#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../../headers/commands_execution.h"
#include "../../headers/queue.h"
#include "../../headers/client.h"
#include "../../headers/api.h"
#include "../../headers/errors.h"

static int is_set_stdout = 0;
static unsigned long int wbytes;
static unsigned long int rbytes;

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
		command = NULL;

	}

}

static int args_counter(char **args) {

	if (args) {

		int i = 0;

		while (args[i] != NULL) {

			i++;

		}

		return i;
	}

	return 0;
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
   
static int execute_f(char **args) {
	
	sockname = realpath(args[0], sockname);
	if (!sockname)
		return -1;

	int msec = 100;
	struct timespec abstime;
	abstime.tv_sec = 3;
	abstime.tv_nsec = 0;

	return openConnection(sockname, msec, abstime);
}

static int execute_w(char *dirname, unsigned long int *count) {

	char *dirpath = NULL;
	dirpath = realpath(dirname, dirpath);
	if (!dirpath)
		return -1;

	DIR *dirp = opendir(dirpath);
	if (!dirp)
		return -1;

	struct dirent *entry = NULL;

	while (*count != 0 && (entry = readdir(dirp)) != NULL) {

		char *temp = NULL;

		if (entry->d_type == DT_DIR) {

			temp = realpath(entry->d_name, temp);
			if (!temp) 
				return -1;

			if (execute_w(temp, count) == -1)
				return -1;

		}
		else if (entry->d_type == DT_REG) {
			
			temp = realpath(entry->d_name, temp);
			if (!temp) 
				return -1;

			int retval = writeFile(temp, w_dir);
			if (retval == -1)
				return -1;

			*count--;

		}

		free(temp);

	}

	return 0;

}

static int execute_W(char **args) {

	int num_args = args_counter(args);

	int i = 0;

	while (i < num_args) {

		char *pathname = NULL;
		pathname = realpath(args[i], pathname);
		if (!pathname) 
			return -1;

		int retval = writeFile(pathname, w_dir);
		if (retval == -1) 
			return -1;

		i++;
		
	}

	return 0;
}

static int execute_D(char **args) {

	char *dirname = NULL;
	dirname = realpath(args[0], dirname);
	if (!dirname)
		return -1;

	char* const slash = "/";
	size_t len = strlen(dirname) + 1;
	dirname = realloc(dirname, len + 1);
	strncat(dirname, slash, 1);

	w_dir = dirname;

	return 0;

}

static int execute_r(char **args) {

	int num_args = args_counter(args);

	int i = 0;

	while (i < num_args) {

		char *pathname = NULL;
		pathname = realpath(args[i], pathname);
		if (!pathname)
			return -1;

		void *buf = NULL;
		size_t size = 0;
		int retval = readFile(pathname, &buf, &size);
		if (retval == -1) {
			free(pathname);
			return -1;
		}

		rbytes += size;

		if (buf)
			free(buf);
		free(pathname);

		i++;
		
	}

	return 0;

}

static int execute_R(char **args) {

	int N = 0;

	if (args && args[0]) {

		N = atoi(args[0]);
		if (N == 0)
		N = -1;

	}
	else 
		N = -1;

	if (readNFiles(N, r_dir) == -1)
		return -1;

	return 0;
}

static int execute_d(char **args) {

	if (args && args[0]) {

		char *dirname = NULL;
		dirname = realpath(args[0], dirname);
		if (!dirname)
			return -1;

		char* const slash = "/";
		size_t len = strlen(dirname) + 1;
		dirname = realloc(dirname, len + 1);
		strncat(dirname, slash, 1);

		r_dir = dirname;
		
	}

	return 0;

}

static void execute_t(char **args) {

	if (args && args[0]) {

		unsigned long int msec = strtoul(args[0], NULL, 10);

		if (msec != 0) {

			time_to_wait.tv_sec = msec / 1000;
			time_to_wait.tv_nsec = (msec % 1000) * 1000000;

		}	
	}
}

static int execute_l(char **args) {

	int num_args = args_counter(args);

	int i = 0;

	while (i < num_args) {

		char *pathname = NULL;
		pathname = realpath(args[i], pathname);
		if (!pathname)
			return -1;

		int retval = lockFile(pathname);
		if (retval == -1) {
			free(pathname);
			return -1;
		}

		free(pathname);

		i++;
		
	}

	return 0;
}

static int execute_u(char **args) {

	int num_args = args_counter(args);

	int i = 0;

	while (i < num_args) {

		char *pathname = NULL;
		pathname = realpath(args[i], pathname);
		if (!pathname) 
			return -1;

		int retval = unlockFile(pathname);
		if (retval == -1) {
			free(pathname);
			return -1;
		}

		free(pathname);

		i++;
		
	}

	return 0;
}

static int execute_c(char **args) {

	int num_args = args_counter(args);

	int i = 0;

	while (i < num_args) {

		char *pathname = NULL;
		pathname = realpath(args[i], pathname);
		if (!pathname)
			return -1;

		int retval = removeFile(pathname);
		if (retval == -1) {
			free(pathname);
			return -1;
		}

		free(pathname);

		i++;
		
	}

	return 0;
}

static void execute_p() {

	is_set_stdout = 1;

}

int execute_command(argnode_t *command) {

	char* const success = "success";
	char* const failure = "failure";
	int errnocpy = 0;
	wbytes = 0;
	rbytes = 0;

	switch (command->arg->option) {

		case 'h': {

			execute_h();
			free_command(command);
			return 1;

		} break;

		case 'f': {

			errno = 0;
			local_errno = 0;
			int retval = execute_f(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				printf("-f %s | ", command->arg->args[0]);
				printf("result: %s\n", !retval ? success : failure);

			}

			if (retval == -1) {

				free_command(command);
				errno = errnocpy;
				my_perror("-f");
				return -1;

			}

		} break;

		case 'w': {

			errno = 0;
			char *dirname = NULL;
			dirname = realpath(command->arg->args[0], dirname);
			if (!dirname) {
				free(command);
				my_perror("-w");
				return 0;
			}

			unsigned long int count = 0;

			if (command->arg->args[1]) {

				count = strtoul(command->arg->args[1], NULL, 10);
				if (count == 0)
				count = -1;
			}
			else
				count = -1;


			errno = 0;
			local_errno = 0;
			int retval = execute_w(dirname, &count);
			errnocpy = errno;

			if (is_set_stdout) {

				int i = 0;

				printf("-w ");

				while (command->arg->args[i] != NULL) {

					if (command->arg->args[i + 1] == NULL)
						printf("%s | ", command->arg->args[i]);
					else
						printf("%s,", command->arg->args[i]);

					i++;

				}

				printf("result: %s\n", !retval ? success : failure);
				printf("number of bytes written: %lu\n", wbytes);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-w");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'W': {

			errno = 0;
			local_errno = 0;
			int retval = execute_W(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				int i = 0;

				printf("-W ");

				while (command->arg->args[i] != NULL) {

					if (command->arg->args[i + 1] == NULL)
						printf("%s | ", command->arg->args[i]);
					else
						printf("%s,", command->arg->args[i]);

					i++;

				}

				printf("result: %s\n", !retval ? success : failure);
				printf("number of bytes written: %lu\n", wbytes);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-W");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'D': {

			errno = 0;
			local_errno = 0;
			int retval = execute_D(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				printf("-D %s | ", command->arg->args[0]);
				printf("result: %s\n", !retval ? success : failure);

			}	

			if (retval == -1) {

				free_command(command);
				errno = errnocpy;
				my_perror("-D");
				return -1;
			}

		} break;

		case 'r': {

			errno = 0;
			local_errno = 0;
			int retval = execute_r(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				int i = 0;

				printf("-r ");

				while (command->arg->args[i] != NULL) {

					if (command->arg->args[i + 1] == NULL)
						printf("%s | ", command->arg->args[i]);
					else
						printf("%s,", command->arg->args[i]);

					i++;

				}

				printf("result: %s\n", !retval ? success : failure);
				printf("number of bytes read: %lu\n", rbytes);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-r");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'R': {

			errno = 0;
			local_errno = 0;
			int retval = execute_R(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				printf("-R ");

				if (command->arg->args && command->arg->args[0] != NULL)
					printf("%s | ", command->arg->args[0]);
				else
					printf("all files | ");

				printf("result: %s\n", !retval ? success : failure);
				printf("number of bytes read: %lu\n", rbytes);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-R");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'd': {

			errno = 0;
			local_errno = 0;
			int retval = execute_d(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				printf("-d %s | ", command->arg->args[0]);

				printf("result: %s\n", !retval ? success : failure);

			}

			if (retval == -1) {

				free_command(command);
				errno = errnocpy;
				my_perror("-d");
				return -1;

			}

		} break;

		case 't': {

			execute_t(command->arg->args);	

			if (is_set_stdout) {

				if (command->arg->args && command->arg->args[0])
					printf("-t %s | ", command->arg->args[0]);
				else
					printf("-t 0 | ");

				printf("result: success\n");

			}

		} break;

		case 'l': {

			errno = 0;
			local_errno = 0;
			int retval = execute_l(command->arg->args);	
			errnocpy = errno;

			if (is_set_stdout) {

				int i = 0;

				printf("-l ");

				while (command->arg->args[i] != NULL) {

					if (command->arg->args[i + 1] == NULL)
						printf("%s | ", command->arg->args[i]);
					else
						printf("%s,", command->arg->args[i]);

					i++;

				}

				printf("result: %s\n", !retval ? success : failure);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-l");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'u': {

			errno = 0;
			local_errno = 0;
			int retval = execute_u(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				int i = 0;

				printf("-u ");

				while (command->arg->args[i] != NULL) {

					if (command->arg->args[i + 1] == NULL)
						printf("%s | ", command->arg->args[i]);
					else
						printf("%s,", command->arg->args[i]);

					i++;

				}

				printf("result: %s\n", !retval ? success : failure);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-u");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'c': {

			errno = 0;
			local_errno = 0;
			int retval = execute_c(command->arg->args);
			errnocpy = errno;

			if (is_set_stdout) {

				int i = 0;

				printf("-c ");

				while (command->arg->args[i] != NULL) {

					if (command->arg->args[i + 1] == NULL)
						printf("%s | ", command->arg->args[i]);
					else
						printf("%s,", command->arg->args[i]);

					i++;

				}

				printf("result: %s\n", !retval ? success : failure);

			}

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-c");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'p': {

			execute_p();

		} break;

	}

	free_command(command);

	return 0;
}
