#define DEBUG_DEVWATCH

#include <signal.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vmrunner.h"

#ifdef DEBUG_DEVWATCH
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/devwatch   ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

#define EVENT_SIZE ( sizeof(struct inotify_event) )
#define BUFFER_SIZE ( 512 * EVENT_SIZE )

int check_for_vm(char *xmlFile)
{
	DPRINTF("Checking for '%s'...\n", xmlFile);
	return (access(xmlFile, F_OK) == 0);
}

int handle_vm_xml(char *xmlFile, char *path, char *uri)
{
	char *xml = NULL;
	int ret = 0;

	DPRINTF("Reading libvirt XML definition file: %s\n", xmlFile);

	xml = replace(readXml(xmlFile), "%PATH%", path);
	xml = replace(xml, "%EMULATOR_PATH%", defemu);
	xml = replace(xml, "%ROOT_PATH%", path);
	xml = replace(xml, "%IMAGE_DIR%", images);
	xml = replace(xml, "%ISO_DIR%", isos);

	ret = startDomain(xml, uri);
	free(xml);
	return ret;
}

int handle_pvr_xml(char *xmlFile, char *path)
{
	char *xml = NULL;
	char *tmp = NULL;
	char *fhvtype = NULL;
	char arch[6] = { 0 };
	char xp[1024] = { 0 };
	char names[4096] = { 0 };
	char cmd[4096 + 256] = { 0 };
	int i, cnt = -1, ret = 0;
	long ver = -1;

	DPRINTF("Reading PVR file %s for path '%s'\n", xmlFile, path);

	xml = readXml(xmlFile);
	if (xml == NULL)
		return -ENOENT;

	/* For version sanity check */
	tmp = xml_query(xml, "//pvr/@version");
	if (tmp == NULL) {
		ret = -EINVAL;
		goto cleanup;
	}

	ver = get_version(tmp);
	free(tmp);

	if (ver > get_version(VERSION)) {
		ret = -ENOTSUP;
		goto cleanup;
	}

	/* Format hypervisor type */
	fhvtype = format_hv_type(hvtype);

	/* Check for architecture */
	if (hostarch != NULL) {
		if (strcmp(hostarch, "x86_64") == 0)
			snprintf(xp, sizeof(xp), "//pvr/domains[@type='%s']/@count", fhvtype);
		else
		if ((strlen(hostarch) > 0) && (hostarch[0] == 'i'))
			snprintf(xp, sizeof(xp), "//pvr/domains[@type='%s']/names[@arch='i686']/@count", fhvtype);
	}

	configs = xml_query(xml, "//pvr/paths/configs");
	images  = xml_query(xml, "//pvr/paths/images");
	isos    = xml_query(xml, "//pvr/paths/isos");

	cnt = -1;
	if ((tmp = xml_query(xml, xp)) != NULL) {
		cnt = atoi(tmp);
		free(tmp);
	}

	if (cnt <= 0) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (strlen(arch) > 0)
		snprintf(xp, sizeof(xp), "//pvr/domains[@type='%s']/names[@arch='%s']/name", fhvtype, arch);
	else
		snprintf(xp, sizeof(xp), "//pvr/domains[@type='%s']/names/name", fhvtype);

	DPRINTF("xPath is '%s'\n", xp);
	free(fhvtype);

	for (i = 0; i < cnt; i++) {
		if ((tmp = xml_query_by_id(xml, xp, i)) != NULL) {
			strcat(names, tmp);
			if (i < cnt - 1)
				strcat(names, " ");
		free(tmp);
		}
	}

	snprintf(cmd, sizeof(cmd), "./asker.sh '%s'", names);
	DPRINTF("Calling \"%s\"\n", cmd);

	/* Error code is being used to identify XML to be loaded */
	i = WEXITSTATUS(system(cmd));
	DPRINTF("Call error code is %d\n", i);
	if (i == 0) {
		ret = -EINVAL;
		goto cleanup;
	}

	tmp = xml_query_by_id(xml, xp, i - 1);
	if (tmp == NULL) {
		ret = -EINVAL;
		goto cleanup;
	}

	snprintf(names, sizeof(names), "%s/%s/%s.xml", path, configs, tmp);
	DPRINTF("Config file location is '%s'\n", names);

	if (access(names, R_OK) != 0) {
		DPRINTF("File '%s' cannot be opened for reading\n", names);
		ret = -ENOENT;
		goto cleanup;
	}

	handle_vm_xml(names, path, NULL);
cleanup:
	free(xml);
	return ret;
}

void handle_new_dev(char *dev, char *uri)
{
	char tmp[128], *mountpoint;
	struct stat buf;

	snprintf(tmp, 128, "%s/%s", DEV_PATH, dev);
	lstat(tmp, &buf);

	if (S_ISBLK(buf.st_mode)) {
		DPRINTF("Found new block device: %s\n", tmp);

		if ((mountpoint = check_dev_mountpoint(dev)) == NULL) {
			int error;
			char *tmp, *fsType;
			char tmp2[1024];

			DPRINTF("%s not mounted. Trying to mount\n", dev);
			fsType = getfstype(dev);
			if (fsType == NULL)
				return;
			tmp = mount_dev(dev, fsType, &error);
			if (error != 0)
				DPRINTF("mount_dev() returned error code: %d\n", error);
			if (tmp == NULL)
				DPRINTF("Device doesn't appear to contain VM images\n");
			else {
				DPRINTF("Device appears to contain a VM images\n");
				snprintf(tmp2, 1024, "%s/%s", tmp, PVR_ROOT_FILE);
				DPRINTF("PVR Root File: %s\n", tmp2);
				DPRINTF("Path: %s\n", tmp);
				DPRINTF("URI: %s\n", uri ? uri : "probing");
				handle_pvr_xml(tmp2, tmp);
				DPRINTF("PVR XML Definition handling done. Unmounting.\n");
				unmount_dev(tmp);
			}
		}
        	else {
			int err;

			DPRINTF("Mounted at %s. Checking\n", mountpoint);
			snprintf(tmp, 512, "%s/%s", mountpoint, PVR_ROOT_FILE);
			err = check_for_vm(tmp);
			if (err == 0)
				DPRINTF("Device doesn't appear to contain a VM image\n");
			else
			if (err == 1) {
				DPRINTF("Device appears to contain a VM image\n");
				handle_vm_xml(tmp, mountpoint, uri);
			}
		}
	}
}

int inotify_loop(int fd, char *uri)
{
	struct timeval timeout;
	fd_set read_fds;
	int i, result;
	char buf[BUFFER_SIZE];

	result = 0;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	while (!isDone) {
		select(fd + 1, &read_fds, (fd_set *)0, (fd_set *)0, &timeout);
		result = read(fd, buf, BUFFER_SIZE);
		if (result > 0) {
			i = 0;
			while (i < result) {
				struct inotify_event *event = (struct inotify_event *)&buf[i];

				if ((event->mask & IN_CREATE)
					&& (strlen(event->name) > 0)
					&& (event->name[0] != '.')) {
					DPRINTF("Found new device: %s\n", event->name);
					handle_new_dev(event->name, uri);
				}

				i += event->len + EVENT_SIZE;
			}
		}
	}

	return 1;
}


void handle_exit(void)
{
	int ret;

	DPRINTF("Handling exit\n");
	ret = virConnectClose(cp);
	DPRINTF("Return value is %d\n", ret);
}

void sig_handler(int sig) {
	if ((sig == SIGINT) || (sig == SIGKILL)) {
		DPRINTF("Signal %d caught. Terminating...\n", sig);
		isDone = 1;
	}
}

int main(int argc, char *argv[])
{
	char *xmlFile, *logFile, *uri, *vnccl, *str;
	char logfile[1024], cwd[1024];
	int fd, wd, ret, opt, fullscreen;

	isDone = 0;
	fullscreen = 0;
	signal(SIGINT, sig_handler);

	configs = NULL;
	images  = NULL;
	isos    = NULL;

	xmlFile = NULL;
	logFile = NULL;
	uri     = NULL;
	vnccl	= NULL;

	#ifdef HAVE_LIBGTK_VNC_1_0
	str = "l:x:u:v:fh";
	#else
	str = "l:x:u:h";
	#endif

	while ((opt = getopt(argc, argv, str)) != -1) {
		switch (opt) {
			case 'x':
				xmlFile = optarg;
				break;
			case 'l':
				logFile = optarg;
				break;
			case 'u':
				uri = optarg;
				break;
			#ifdef HAVE_LIBGTK_VNC_1_0
			case 'v':
				vnccl = optarg;
				break;
			case 'f':
				fullscreen = 1;
				break;
			#endif
			case 'h':
			default: /* '?' */
				fprintf(stderr, "Usage: %s [-x xmlFile] [-l logFile] [-u hypervisor-uri] %s\n",
					argv[0],
					#ifdef HAVE_LIBGTK_VNC_1_0
					"[-v vncserver:port [-f]]"
					#else
					""
					#endif
					);
				exit(EXIT_FAILURE);
		}
	}

	if ((logFile != NULL) && (strcmp(logFile, "console") != 0)) {
		if (!set_logfile(logFile))
			printf("Cannot set logging file to %s\n", logfile);
	}
	else
	if (logFile == NULL) {
		getcwd(cwd, 1024);
		snprintf(logfile, 1024, "%s/logs", cwd );
		if (access(logfile, F_OK) != 0)
			mkdir(logfile, 0755);
		snprintf(logfile, 1024, "%s/logs/vmrunner-%d.log", cwd, getpid() );
		if (!set_logfile(logfile))
			printf("Cannot set logging file to %s\n", logfile);
		else
			printf("Logging to file '%s'\n", logfile);
	}

	DPRINTF("Started with PID %d\n", getpid() );

	if (vnccl != NULL) {
		char *host = NULL;
		char *port = NULL;

		tTokenizer t = tokenize_by(vnccl, ":");
		if (t.numTokens != 2)
			return 0;

		host = strdup(t.tokens[0]);
		port = strdup(t.tokens[1]);
		free_tokens(t);

		return startVNCViewer(host, port, fullscreen);
	}

	hvtype   = libvirtGetHypervisorType(uri);
	defemu   = libvirtGetDefaultEmulator(uri, format_hv_type(hvtype));
	hostarch = libvirtGetHostArchitecture(uri);

	DPRINTF("Default emulator is %s\n", defemu ? defemu : "none");
	DPRINTF("Host architecture is %s\n", hostarch ? hostarch : "unknown");
	DPRINTF("Host hypervisor is %s\n", hvtype ? hvtype : "unknown");

	if (xmlFile != NULL) {
		char *path;
		int last;

		last = strrchr(xmlFile, '/') ? strlen(strrchr(xmlFile, '/')) : 0;
		path = strdup(xmlFile);
		path[last] = 0;

		if (((strlen(path) > 0) && (path[0] != '/'))
			|| (strlen(path) == 0)) {
			char path2[1024] = { 0 };
			getcwd(cwd, 1024);
			snprintf(path2, 1024, "%s/%s", cwd, path);
			path = strdup(path2);
		}

		ret = handle_pvr_xml(xmlFile, path);
	}
	else {
		DPRINTF("Initializing inotify watch on %s\n", DEV_PATH);

		fd = inotify_init();
		wd = inotify_add_watch(fd, DEV_PATH, IN_CREATE);
		inotify_loop(fd, uri);
		DPRINTF("Inotify loop terminated\n");
		inotify_rm_watch(fd, wd);
		DPRINTF("Removing inotify watch\n");
		close(fd);

		ret = 0;
	}

	handle_exit();

	DPRINTF("Program done, exiting\n");
	return ret;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
