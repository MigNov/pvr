#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libvirt/libvirt.h>

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#if HAVE_LIBBLKID
#include <blkid/blkid.h>
#endif

#if HAVE_LIBGTK_VNC_1_0
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "vncdisplay.h"
#endif

#define PVR_VERSION			"0.1"
#define DEV_PATH			"/dev"
#define PVR_ROOT_FILE			"pvr.xml"
#define VNC_STATUS_OK			0
#define VNC_STATUS_SHUTDOWN		1
#define VNC_STATUS_NO_CONNECTION	2
#define VNC_STATUS_UNSUPPORTED		3

#define USE_HACK

virConnectPtr cp;
char *defemu;
char *hostarch;
char *hvtype;

char *configs;
char *images;
char *isos;

int isDone;
int domainIsOff;
int lastErrorCode;
int g_allow_external_mount;

char	*get_datetime(void);
char	*replace(char *str, char *what, char *with);
int	set_logfile(char *filename);
char	*xml_query(char *xmlFile, char *xPath);
char	*xml_query_by_id(char *xmlFile, char *xPath, int id);
int	set_logfile(char *filename);
char	*replace(char *str, char *what, char *with);
char	*readXml(char *xmlFile);
int	startDomain(char *xml, char *uri);
char	*check_dev_mountpoint(char *dev);
int	unmount_dev(char *dev);
char	*mount_dev(char *dev, char *fstype, int *error);
char	*getfstype(char *dev);
int	check_for_vm(char *xmlFile);
int	startVNCViewer(char *hostname, char *port, int fullscreen);
int	show_selector(char *title, char **items, int num_items);
void	validate_and_strip_xml(char *str);

typedef struct tTokenizer {
        char **tokens;
        int numTokens;
} tTokenizer;

tTokenizer	tokenize(char *string);
tTokenizer	tokenize_by(char *string, char *what);
void	free_tokens(tTokenizer t);
long	get_version(char *string);
char	*format_hv_type(char *hvtype);

char *libvirtGetDefaultEmulator(char *uri, char *type);
char *libvirtGetHostArchitecture(char *uri);
char *libvirtGetHypervisorType(char *uri);
