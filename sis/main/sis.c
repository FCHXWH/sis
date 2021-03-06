#include "sis.h"

#include <sys/stat.h>

#include <readline/readline.h>
#include <readline/history.h>

extern void init_sis();
extern void end_sis();

static int main_has_restarted = 0;
static network_t *network; /* allows for restart ... */

char **command_name_completion(const char *, int, int);
char *command_name_generator(const char *, int);
char *commands[] = {
	"act_map",
	"add_inverter",
	"alias",
	"astg_add_state",
	"astg_contract",
	"astg_current",
	"astg_encode",
	"astg_lockgraph",
	"astg_marking",
	"astg_persist",
	"astg_print_sg",
	"astg_print_statastg_slow",
	"astg_state_min",
	"astg_stg_scr",
	"astg_syn",
	"astg_to_f",
	"astg_to_stg",
	"atpg",
	"bdsyn",
	"buffer_opt",
	"c_check",
	"c_opt",
	"chng_clock",
	"chng_name",
	"collapse",
	"constraints",
	"decomp",
	"echo",
	"eliminate",
	"env_seq_dc",
	"env_verify_fsm",
	"equiv_nets",
	"espresso",
	"extract_seq_dc",
	"factor",
	"fanout_alg",
	"fanout_param",
	"force_init_0",
	"free_dc",
	"full_simplify",
	"fx",
	"gcx",
	"gkx",
	"help",
	"history",
	"invert",
	"invert_io",
	"ite_map",
	"latch_output",
	"map",
	"one_hot",
	"phase",
	"power_estimate",
	"power_free_info",
	"power_print",
	"print",
	"print_altname",
	"print_clock",
	"print_delay",
	"print_factor",
	"print_gate",
	"print_io",
	"print_kernel",
	"print_latch",
	"print_level",
	"print_library",
	"print_map_statsprint_state",
	"print_stats",
	"print_value",
	"quit",
	"read_astg",
	"read_blif",
	"read_eqn",
	"read_kiss",
	"read_library",
	"read_pla",
	"read_slif",
	"red_removal",
	"reduce_depth",
	"remove_dep",
	"remove_latches",
	"replace",
	"reset_name",
	"resub",
	"retime",
	"save",
	"set",
	"set_delay",
	"set_state",
	"short_tests",
	"sim_verify",
	"simplify",
	"simulate",
	"source",
	"speed_up",
	"speedup_alg",
	"state_assign",
	"state_minimize",
	"stg_cover",
	"stg_extract",
	"stg_to_astg",
	"stg_to_network",
	"sweep",
	"tech_decomp",
	"time",
	"timeout",
	"unalias",
	"undo",
	"unset",
	"usage",
	"verify",
	"verify_fsm",
	"wd",
	"write_astg",
	"write_bdnet",
	"write_blif",
	"write_eqn",
	"write_kiss",
	"write_pds",
	"write_pla",
	"write_slif",
	"xl_absorb",
	"xl_ao",
	"xl_coll_ck",
	"xl_cover",
	"xl_decomp_two",
	"xl_imp",
	"xl_k_decomp",
	"xl_merge",
	"xl_part_coll",
	"xl_partition",
	"xl_rl",
	"xl_split",
    	NULL
};

char **command_name_completion(const char *text, int start, int end)
{
    return rl_completion_matches(text, command_name_generator);
}

char *command_name_generator(const char *text, int state)
{
    static int list_index, len;
    char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = commands[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

static int source_sisrc(network)
	network_t **network; {
	char *cmdline;
	char *lib_name;
	char *homefile;
	struct stat home, cur;
	int s1, s2; /* Flags for checking the stat() call */
	int status0, status1, status2, status3, status4, status5;

	lib_name = sis_library();
	cmdline = ALLOC(char, strlen(lib_name) + 20);

	(void) sprintf(cmdline, "source -s %s/.misrc", lib_name);
	status0 = com_execute(network, cmdline);
	(void) sprintf(cmdline, "source -s %s/.sisrc", lib_name);
	status1 = com_execute(network, cmdline);
	status3 = com_execute(network, "source -s ~/.sisrc");

	homefile = util_tilde_expand("~/.misrc");
	s1 = stat(homefile, &home);
	s2 = stat(".misrc", &cur);
	status2 = status4 = TRUE;
	if ((s1 == 0) && (s2 == 0) && (home.st_ino == cur.st_ino)) {
		/* ~/.misrc == .misrc : Source the file only once */
		status2 = com_execute(network, "source -s ~/.misrc");
	} else {
		if (s1 == 0) { /* ~/.misrc exists and can be opened */
			status2 = com_execute(network, "source -s ~/.misrc");
		}
		if (s2 == 0) { /* ./.misrc exists and can be opened */
			status4 = com_execute(network, "source -s .misrc");
		}
	}
	FREE(homefile);

	homefile = util_tilde_expand("~/.sisrc");
	s1 = stat(homefile, &home);
	s2 = stat(".sisrc", &cur);
	status3 = status5 = TRUE;
	if ((s1 == 0) && (s2 == 0) && (home.st_ino == cur.st_ino)) {
		/* ~/.sisrc == .sisrc : Source the file only once */
		status3 = com_execute(network, "source -s ~/.sisrc");
	} else {
		if (s1 == 0) {
			status3 = com_execute(network, "source -s ~/.sisrc");
		}
		if (s2 == 0) {
			status5 = com_execute(network, "source -s .sisrc");
		}
	}
	FREE(homefile);

	FREE(cmdline);
	FREE(lib_name);
	return status0 && status1 && status2 && status3 && status4 && status5;
}

static void usage(prog)
	char *prog; {
	char *lib_name;

	(void) fprintf(miserr, "%s\n", sis_version());
	(void) fprintf(miserr,
			"usage: %s [-sx] [-c cmd] [-f script] [-o file] [-t type] [-T type] [file]\n",
			prog);
#ifdef SIS
	(void) fprintf(miserr, "    -c cmd\texecute SIS commands `cmd'\n");
	(void) fprintf(miserr, "    -f file\texecute SIS commands from a file\n");
#else
	(void) fprintf(miserr,
			"    -c cmd\texecute MIS commands `cmd'\n");
	(void) fprintf(miserr,
			"    -f file\texecute MIS commands from a file\n");
#endif
	(void) fprintf(miserr,
			"    -o file\tspecify output filename (default is -)\n");
	lib_name = sis_library();
#ifdef SIS
	(void) fprintf(miserr, "    -s\t\tsuppress initial 'source %s/.sisrc'\n",
			sis_library());
#else
	(void) fprintf(miserr,
			"    -s\t\tsuppress initial 'source %s/.misrc'\n", sis_library());
#endif
	FREE(lib_name);
#ifdef SIS
	(void) fprintf(miserr,
			"    -t type\tspecify input type (blif, eqn, kiss, oct, pla, slif, or none)\n");
	(void) fprintf(miserr,
			"    -T type\tspecify output type (blif, eqn, kiss, oct, pla, slif, or none)\n");
#else
	(void) fprintf(miserr,
			"    -t type\tspecify input type (bdnet, blif, eqn, oct, pla, or none)\n");
	(void) fprintf(miserr,
			"    -T type\tspecify output type (bdnet, blif, eqn, oct, pla, or none)\n");
#endif
	(void) fprintf(miserr, "    -x\t\tequivalent to '-t none -T none'\n");
	exit(2);
}

static int check_type(s)
	char *s; {
	if (strcmp(s, "bdnet") == 0) {
		return 1;
	} else if (strcmp(s, "blif") == 0) {
		return 1;
	} else if (strcmp(s, "eqn") == 0) {
		return 1;
#ifdef SIS
	} else if (strcmp(s, "kiss") == 0) {
		return 1;
#endif
	} else if (strcmp(s, "oct") == 0) {
		return 1;
	} else if (strcmp(s, "pla") == 0) {
		return 1;
#ifdef SIS
	} else if (strcmp(s, "slif") == 0) {
		return 1;
#endif
	} else if (strcmp(s, "none") == 0) {
		return 1;
	} else {
		(void) fprintf(miserr, "unknown type %s\n", s);
		return 0;
	}
}

int main(argc, argv)
	int argc;char **argv; {
	int c, status, batch, initial_source, initial_read, final_write;
	int quit_flag;
	char readcmd[20], writecmd[20];
	char *dummy, *cmdline, *cmdline1, *infile, *outfile;

	program_name = argv[0];

	quit_flag = -1; /* Quick quit */
	util_getopt_reset();

	if (main_has_restarted) {
		(void) fprintf(stderr, "Restarting frozen image ...\n");
	} else {
		main_has_restarted = 1;
		init_sis();
		network = network_alloc();
	}
	// printf("start debug\n");
	cmdline = util_strsav("");
	(void) strcpy(readcmd, "read_blif");
	(void) strcpy(writecmd, "write_blif");
	infile = "-";
	outfile = "-";
	command_hist = array_alloc(char *, 0);
	initial_source = 1;
	initial_read = 1;
	final_write = 1;
	batch = 0;
	util_getopt_reset();
	while ((c = util_getopt(argc, argv, "c:f:o:st:T:xX:")) != EOF) {
		switch (c) {
		case 'c':
			FREE(cmdline);
			cmdline = util_strsav(util_optarg);
			batch = 1;
			break;

		case 'f':
			FREE(cmdline);
			cmdline = ALLOC(char, strlen(util_optarg) + 20);
			(void) sprintf(cmdline, "source %s", util_optarg);
			batch = 1;
			break;

		case 'o':
			outfile = util_optarg;
			break;

		case 's':
			initial_source = 0;
			break;

		case 't':
			if (check_type(util_optarg)) {
				if (strcmp(util_optarg, "none") == 0) {
					initial_read = 0;
				} else {
					(void) sprintf(readcmd, "read_%s", util_optarg);
				}
			} else {
				usage(argv[0]);
			}
			batch = 1;
			break;

		case 'T':
			if (check_type(util_optarg)) {
				if (strcmp(util_optarg, "none") == 0) {
					final_write = 0;
				} else {
					(void) sprintf(writecmd, "write_%s", util_optarg);
				}
			} else {
				usage(argv[0]);
			}
			batch = 1;
			break;

		case 'x':
			final_write = 0;
			initial_read = 0;
			batch = 1;
			break;

		case 'X':
			/* Handled in previous option loop. */
			break;

		default:
			usage(argv[0]);
		}
	}
	// printf("cmdline is : %s\n",cmdline);
	// printf("test\n");
	if (!batch) {
		/* interactive use ... */
		if (argc - util_optind != 0) {
			(void) fprintf(miserr, "warning -- trailing arguments ignored\n");
		}
		(void) fprintf(misout, "%s\n", sis_version());
		if (initial_source) {
			(void) source_sisrc(&network);
		}
		rl_attempted_completion_function = command_name_completion;
		do{
			cmdline=readline("sis> ");
			if(cmdline){
				quit_flag=com_execute(&network, cmdline);
				add_history(cmdline);
			}
		}while(quit_flag>=0);
		status = 0;
	} else {
		// printf("util_optind: %d",util_optind);
		/* read initial network */
		if(argc > 2)
			infile = argv[3];
		else
			usage(argv[0]);
		// if (argc - util_optind == 0) {
		// 	printf("The name of infile: -\n");
		// 	infile = "-";
		// } else if (argc - util_optind == 1) {
		// 	infile = argv[util_optind];
		// 	printf("The name of infile: %s\n",infile);
		// } else {
		// 	usage(argv[0]);
		// }

		if (initial_source) {
			(void) source_sisrc(&network);
		}
		// printf("end of source_sisrc\n");
		status = 0;
		// if (initial_read) {
		// 	cmdline1 = ALLOC(char, strlen(infile) + 20);
		// 	(void) sprintf(cmdline1, "%s %s", readcmd, infile);
		// 	status = com_execute(&network, cmdline1);
		// 	printf("command1 is : %s\n",cmdline1);
		// 	printf("end of com_execute\n");
		// 	FREE(cmdline1);
		// }

		if (status == 0) {
			status = com_execute(&network, cmdline);
			// if ((status == 0 || status == -1) && final_write) {
			// 	cmdline1 = ALLOC(char, strlen(outfile) + 20);
			// 	(void) sprintf(cmdline1, "%s %s", writecmd, outfile);
			// 	status = com_execute(&network, cmdline1);
			// 	FREE(cmdline1);
			// }
		}

	}

	FREE(cmdline);
	for (c = array_n(command_hist); c-- > 0;) {
		dummy = array_fetch(char *, command_hist, c);
		FREE(dummy);
	}
	array_free(command_hist);
	/* Value of "quit_flag" is determined by the "quit" command */
	if (quit_flag == -1 || quit_flag == -2) {
		status = 0;
	}
	if (quit_flag == -2) {
		network_free(network);
		end_sis();
	}
	exit(status);
}
