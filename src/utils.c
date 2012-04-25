#define DEBUG_UTILS
#include "vmrunner.h"

#ifdef DEBUG_UTILS
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/utils      ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

char *get_datetime(void)
{
	char *outstr = NULL;
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL)
		return NULL;

	outstr = (char *)malloc( 32 * sizeof(char) );
	if (strftime(outstr, 32, "%Y-%m-%d %H:%M:%S", tmp) == 0)
		return NULL;

	return outstr;
}

tTokenizer tokenize_by(char *string, char *by)
{
	char *tmp = NULL;
	char *str = NULL;
	char *save = NULL;
	char *token = NULL;
	int i = 0;
	tTokenizer t;

	tmp = strdup(string);
	t.tokens = malloc( sizeof(char *) );
	save = NULL;
	for (str = tmp; ; str = NULL) {
		token = strtok_r(str, by, &save);
		if (token == NULL)
			break;

		t.tokens = realloc( t.tokens, (i + 1) * sizeof(char *) );
		t.tokens[i++] = strdup(token);
	}
	token = save;

	t.numTokens = i;
	return t;
}

tTokenizer tokenize(char *string)
{
	return tokenize_by(string, " ");
}

void free_tokens(tTokenizer t)
{
	int i;

	for (i = 0; i < t.numTokens; i++) {
		free(t.tokens[i]);
		t.tokens[i] = NULL;
	}
}

long get_version(char *string)
{
	int major, minor;
	tTokenizer t;

	t = tokenize_by(string, ".");
	if (t.numTokens < 2) {
		free_tokens(t);
		return -EINVAL;
	}

	major = atoi(t.tokens[0]);
	minor = atoi(t.tokens[1]);
	free_tokens(t);

	return (major * 1000) + minor;
}

char *format_hv_type(char *hvtype)
{
	if (hvtype == NULL)
		return NULL;

	if (strcmp(hvtype, "Xen") == 0)
		return strdup("xen");
	else
	if ((strcmp(hvtype, "KVM") == 0)
		|| (strcmp(hvtype, "QEMU") == 0))
		return strdup("kvm");

	return NULL;
}

int set_logfile(char *filename)
{
	int res;

	res = (freopen(filename, "w", stderr) == NULL) ? 0 : 1;
	DPRINTF("Setting up log file to %s ... %s\n", filename, res ? "OK" : "FAILED");

	return res;
}

char *replace(char *str, char *what, char *with)
{
	int size, idx;
	char *new, *part, *old;
    
	DPRINTF("About to replace %d bytes with %d bytes\n", (int)strlen(what), (int)strlen(with));
	DPRINTF("Original string at %p (%d bytes)\n", str, (int)strlen(str));
	part = strstr(str, what);
	if (part == NULL) {
		DPRINTF("Cannot find partial token (%s)\n", what);
		return str;
	}

	DPRINTF("Have first part at %p, %d bytes\n", part, (int)strlen(part));

	size = strlen(str) - strlen(what) + strlen(with);
	new = (char *)malloc( size * sizeof(char) );
	DPRINTF("New string size allocated to %d bytes\n", size);
	old = strdup(str);
	DPRINTF("Duplicated string str at %p to old at %p\n", str, old);
	idx = strlen(str) - strlen(part);
	DPRINTF("Setting idx %d of old at %p to 0\n", idx, old);
	old[idx] = 0;
	DPRINTF("Old string (%p) idx %d set to 0\n", old, idx);
	strcpy(new, old);
	strcat(new, with);
	strcat(new, part + strlen(what) );
	DPRINTF("About to return new at %p\n", new);
#if 0
	/* free(part); */
	part = NULL;
	DPRINTF("Part freed\n");
	/* free(old); */
	old = NULL;
	DPRINTF("Old freed\n");
#endif
	return new;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
