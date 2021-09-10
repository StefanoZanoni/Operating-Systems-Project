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

#include "../../headers/client/commands_execution.h"
#include "../../headers/client/queue.h"
#include "../../headers/client/client.h"
#include "../API/api.h"
#include "../../headers/client/errors.h"

static int is_set_stdout = 0;
static long int wbytes;
static long int rbytes;

typedef struct flist {

	char **files;
	unsigned int pos;
	unsigned int dim;

} flist_t;

static void free_command(commandline_arg_t command) {

	if (command.args) {

		int i = 0;
		while (command.args[i]) 
			free(command.args[i++]);

		free(command.args);

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

	printf("\n");

	char *help[][2] = {
    			{"-h",               "visualizza help delle opzioni.\n\n"},
            {"-f <filename>",    "specifica il nome del socket AF_UNIX a cui connettersi.\n\n"},
            {"-w dirname[,n=0]", "invia al server i file nella cartella ‘dirname’, ovvero effettua una richiesta di scrittura al "
                                 "server per i file.\nSe la directory ‘dirname’ contiene altre directory, queste vengono visitate ricorsivamente "
                                 "fino a quando non si leggono ‘n‘ file; se n=0 (o non è specificato) non c’è un limite superiore al numero di "
                                 "file da inviare al server (tuttavia non è detto che il server possa scriverli tutti).\n\n"},
            {"-W file1[,file2]", "lista di nomi di file da scrivere nel server separati da ‘,’.\n\n"},
            {"-D dirname",       "cartella in memoria secondaria dove vengono scritti (lato client) i file che il server rimuove a "
                                 "seguito di capacity misses (e che erano stati modificati da operazioni di scrittura) per servire le scritture "
                                 "richieste attraverso l’opzione ‘-w’ e ‘-W’.\nL’opzione ‘-D’ deve essere usata quindi congiuntamente "
                                 "all’opzione ‘-w’ o ‘-W’, altrimenti viene generato un errore.\nSe l’opzione ‘-D’ non viene specificata, tutti i "
                                 "file che il server invia verso il client a seguito di espulsioni dalla cache, vengono buttati via.\nAd esempio, "
                                 "supponiamo i seguenti argomenti a linea di comando “-w send -D store”, e supponiamo che dentro la cartella "
                                 "‘send’ ci sia solo il file ‘pippo’.\nInfine supponiamo che il server, per poter scrivere nello storage il file "
                                 "‘pippo’ deve espellere il file ‘pluto’ e ‘minni’.\nAllora, al termine dell’operazione di scrittura, la cartella "
                                 "‘store’ conterrà sia il file ‘pluto’ che il file ‘minni’.\nSe l’opzione ‘-D’ non viene specificata, allora il server "
                                 "invia sempre i file ‘pluto’ e ‘minni’ al client, ma questi vengono buttati via.\n\n"},
            {"-r file1[,file2]", "lista di nomi di file da leggere dal server separati da ‘,’ (esempio: -r pippo,pluto,minni).\n\n"},
            {"-R [n=0]",         "tale opzione permette di leggere ‘n’ file qualsiasi attualmente memorizzati nel server;\nse n=0 (o "
                                 "non è specificato) allora vengono letti tutti i file presenti nel server.\n\n"},
            {"-d dirname",       "cartella in memoria secondaria dove scrivere i file letti dal server con l’opzione ‘-r’ o ‘-R’.\n"
                                 "L’opzione -d va usata congiuntamente a ‘-r’ o ‘-R’, altrimenti viene generato un errore;\nSe si utilizzano le "
                                 "opzioni ‘-r’ o ‘-R’senza specificare l’opzione ‘-d’ i file letti non vengono memorizzati sul disco.\n\n"},
            {"-t time",          "tempo in millisecondi che intercorre tra l’invio di due richieste successive al server (se non "
                                 "specificata si suppone -t 0, cioè non c’è alcun ritardo tra l’invio di due richieste consecutive).\n\n"},
            {"-l file1[,file2]", "lista di nomi di file su cui acquisire la mutua esclusione.\n\n"},
            {"-u file1[,file2]", "lista di nomi di file su cui rilasciare la mutua esclusione.\n\n"},
            {"-c file1[,file2]", "lista di file da rimuovere dal server se presenti.\n\n"},
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

	int retval = openConnection(sockname, msec, abstime);
	if (retval == -1) {
		free(sockname);
		return -1;
	}

	return 0;
}

static int get_files(char *entry_path, int *num_to_write, flist_t *file_list) {

	DIR *dirp = opendir(entry_path);
	if (!dirp)
		return -1;

	struct dirent *curr_entry = NULL;

	while (*num_to_write != 0 && (curr_entry = readdir(dirp)) != NULL) {

		size_t entry_len = 0;
		size_t curr_len = 0;
		size_t total_len = 0;
		int retval = 0;

		if (curr_entry->d_type == DT_DIR) {

			if (strcmp(curr_entry->d_name, ".") == 0 || strcmp(curr_entry->d_name, "..") == 0)
         	continue;

			char *dirpath = NULL;

			entry_len = strlen(entry_path) + 1;
			curr_len = strlen(curr_entry->d_name) + 1;
			total_len = entry_len + 1 + curr_len;
			dirpath = calloc(total_len, sizeof(char));
			if (!dirpath) {
				closedir(dirp);
				return -1;
			}

			snprintf(dirpath, total_len, "%s/%s", entry_path, curr_entry->d_name);

			retval = get_files(dirpath, num_to_write, file_list);
			if (retval == -1) {
				free(dirpath);
				closedir(dirp);
				return -1;
			}

			free(dirpath);

		}
		else if (curr_entry->d_type == DT_REG) {
			
			char *filepath = NULL;

			entry_len = strlen(entry_path) + 1;
			curr_len = strlen(curr_entry->d_name) + 1;
			total_len = entry_len + 1 + curr_len;
			filepath = calloc(total_len, sizeof(char));
			if (!filepath) {
				closedir(dirp);
				return -1;
			}
			
			snprintf(filepath, total_len, "%s/%s", entry_path, curr_entry->d_name);
			
			// If I have to write the last free position in the file_list->files vector,
			// I need to allocate more memory since I'm getting an undefined number of files
			if (*num_to_write < 0 && file_list->pos == file_list->dim - 2) {

				file_list->dim += file_list->dim / 2;
				char **temp = calloc(file_list->dim, sizeof(char*));
				if (!temp) {
					closedir(dirp);
					free(filepath);
					return -1;
				}

				int i = 0;
				while (i < file_list->pos) {

					size_t curr_len = strlen(file_list->files[i]) + 1;
					temp[i] = calloc(curr_len, sizeof(char));
					if (!temp[i]) {

						closedir(dirp);
						free(filepath);
						int j = 0;
						while (j < i) 
							free(temp[j++]);
						free(temp);

						return -1;

					}
					memmove(temp[i], file_list->files[i], curr_len);
					free(file_list->files[i]);
					file_list->files[i] = NULL;
					i++;

				}

				free(file_list->files);
				file_list->files = temp;
				temp = NULL;

			}
			
			file_list->files[file_list->pos++] = strndup(filepath, total_len);
			free(filepath);

			*num_to_write = *num_to_write - 1;

		}

	}

	closedir(dirp);

	return 0;
}

static int execute_w(char **args) {

	int num_to_write = 0;

	// If the number of files to be sent is 0 or not specified
	// I set num_to_write to -1 so since it is decremented by 1 
	// at each iteration I can read all those in the directory.
	if (args[1]) {

		num_to_write = strtoul(args[1], NULL, 10);
		if (num_to_write == 0)
			num_to_write = -1;

	}
	else 
		num_to_write = -1;

	char *dirpath = NULL;
	dirpath = realpath(args[0], dirpath);
	if (!dirpath)
		return -1;

	flist_t file_list;
	if (num_to_write > 0) {

		file_list.dim = num_to_write + 1;
		file_list.pos = 0;
		file_list.files = calloc(file_list.dim, sizeof(char*));
		if (!file_list.files) {
			free(dirpath);
			return -1;
		}

	}
	else {

		// I start by considering a maximum of 10 files to be sent.
		// If the number of files in the directory is more than 10
		// then I realloc files.
		file_list.dim = 11;
		file_list.pos = 0;
		file_list.files = calloc(11, sizeof(char*));
		if (!file_list.files) {
			free(dirpath);
			return -1;
		}

	}

	int i = 0;

	int retval = get_files(dirpath, &num_to_write, &file_list);
	if (retval == -1) {

		while (file_list.files[i] != NULL) 
			free(file_list.files[i++]);

		free(file_list.files);
		free(dirpath);

		return -1;

	}

	i = 0;

	while(file_list.files[i] != NULL) {

		if (writeFile(file_list.files[i], w_dir) == -1) {

			// if there is an error with the file writing 
			// then I free the memory of all of successive files
			while (file_list.files[i] != NULL)
				free(file_list.files[i++]);

			free(file_list.files);
			free(dirpath);

			return -1;

		}

		// If the file has been successfully written to the server,
		// I add its size to the amount of bytes written so far
		FILE* f = fopen(file_list.files[i], "rb");
		fseek(f, 0L, SEEK_END);
		wbytes += ftell(f);
		fclose(f);

		// if the current file has been successfully written to the server
		// then I can free its memory
		free(file_list.files[i]);
		file_list.files[i++] = NULL;

	}

	free(dirpath);
	free(file_list.files);

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
		if (retval == -1) {
			free(pathname);
			return -1;
		}

		// Only if the file has been successfully written to the server 
		// I store total amount of bytes written in wbytes
		FILE* f = fopen(pathname, "rb");
		fseek(f, 0L, SEEK_END);
		wbytes += ftell(f);
		fclose(f);

		free(pathname);

		i++;
		
	}

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

		if (r_dir) {

			FILE *f = fopen(pathname, "wb");
			if (!f) {
				free(pathname);
				free(buf);
				return -1;
			}
			fwrite((char*) buf, sizeof(char), size, f);
			fclose(f);
			
		}

		if (buf) {
			free(buf);
			buf = NULL;
		}
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

	int retval = readNFiles(N, r_dir);
	if (retval == -1)
		return -1;

	return 0;
}

static int execute_Dd(commandline_arg_t arg) {

	if (arg.args && arg.args[0]) {

		char *dirname = NULL;
		dirname = realpath(arg.args[0], dirname);
		if (!dirname)
			return -1;

		size_t len = strlen(dirname) + 1;
		dirname = realloc(dirname, len + 1);
		if (!dirname)
			return -1;

		strcat(dirname, "/");

		if (arg.option == 'd')
			r_dir = dirname;
		else 
			w_dir = dirname;
		
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

static int execute_luc(commandline_arg_t arg) {

	int num_args = args_counter(arg.args);

	int i = 0;

	while (i < num_args) {

		char *pathname = NULL;
		pathname = realpath(arg.args[i], pathname);
		if (!pathname)
			return -1;

		int retval = 0;

		if (arg.option == 'l')
			retval = lockFile(pathname);
		else if (arg.option == 'u')
			retval = unlockFile(pathname);
		else if (arg.option == 'c')
			retval = removeFile(pathname);

		if (retval == -1) {
			free(pathname);
			return -1;
		}

		free(pathname);

		i++;
		
	}

	return 0;
}

static void print_execution_result(commandline_arg_t command, int retval) {

	char* const success = "success";
	char* const failure = "failure";

	int i = 0;

	printf("\n-%c ", command.option);

	while (command.args[i] != NULL) {

		if (command.args[i + 1] == NULL)
			printf("%s | ", command.args[i]);
		else
			printf("%s,", command.args[i]);

		i++;

	}

	printf("result: %s\n", !retval ? success : failure);

	if (command.option == 'w' || command.option == 'W') {

		if (wbytes / 1000000 < 1) {
			double KB = (double) wbytes / 1000;
			if (!retval)
				printf("number of bytes written: %.3lf kB\n\n", KB);
			else
				printf("number of bytes written: %.3lf kB\n", KB);
		}
		else if (wbytes / 1000000000 < 1) {
			double MB = (double) wbytes / 1000000;
			if (!retval)
				printf("number of bytes written: %.3lf MB\n\n", MB);
			else
				printf("number of bytes written: %.3lf kB\n", MB);
		}

	}
	else if (command.option == 'r') {

		if (rbytes / 1000000 < 1) {
			double KB = (double) rbytes / 1000;
			if (!retval)
				printf("number of bytes read: %.3lf kB\n\n", KB);
			else
				printf("number of bytes read: %.3lf kB\n", KB);
		}
		else if (rbytes / 1000000000 < 1) {
			double MB = (double) rbytes / 1000000;
			if (!retval)
				printf("number of bytes read: %.3lf MB\n\n", MB);
			else
				printf("number of bytes read: %.3lf kB\n", MB);
		}

	}
}

int execute_command(commandline_arg_t command) {

	int errnocpy = 0;
	int retval = 0;
	errno = 0;
	local_errno = 0;

	switch (command.option) {

		case 'h': {

			execute_h();
			free_command(command);
			return 1;

		} break;

		case 'f': {

			retval = execute_f(command.args);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-f");
				free_command(command);
				return -1;
			}

		} break;

		case 'w': {

			wbytes = 0;

			retval = execute_w(command.args);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-w");
			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'W': {

			wbytes = 0;

			retval = execute_W(command.args);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-W");
			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'D': {

			retval = execute_Dd(command);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-D");
				free_command(command);
				return -1;
			}

		} break;

		case 'r': {

			rbytes = 0;

			retval = execute_r(command.args);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-r");
			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'R': {

			rbytes = 0;

			retval = execute_R(command.args);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-R");
			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'd': {

			retval = execute_Dd(command);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-d");
				free_command(command);
				return -1;
			}

		} break;

		case 't': {

			execute_t(command.args);	

			if (is_set_stdout)
				print_execution_result(command, retval);

		} break;

		case 'l': {

			retval = execute_luc(command);	
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {

				errno = errnocpy;
				my_perror("-l");

			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'u': {

			retval = execute_luc(command);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-u");
			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'c': {

			retval = execute_luc(command);
			errnocpy = errno;

			if (is_set_stdout)
				print_execution_result(command, retval);

			if (retval == -1) {
				errno = errnocpy;
				my_perror("-c");
			}

			nanosleep(&time_to_wait, NULL);

		} break;

		case 'p': {

			is_set_stdout = 1;

		} break;

	}

	free_command(command);

	return 0;
}
