#define DEBUG_VMRUNNER
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include "vmrunner.h"

#ifdef DEBUG_VMRUNNER
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/vmrunner   ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

char *readXml(char *xml)
{
	FILE *fp;
	char *ret = NULL;
	int size;

	fp = fopen(xml, "r");
	if (!fp)
		return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	ret = (char *) malloc(size * sizeof(char));
	fread(ret, size, 1, fp);
	fclose(fp);

	return ret;
}

void errHandler(void *userData, virErrorPtr error)
{
	if (error->code == VIR_ERR_NO_DOMAIN) {
		domainIsOff = 1;
		return;
	}
	lastErrorCode = error->code;
	DPRINTF("libvirtError: %s (code %d)\n", error->message, lastErrorCode);
	virFreeError(error);
}

virConnectPtr libvirtConnect(char *uri)
{
	virConnectPtr cp;

	virSetErrorFunc("vmrunner", errHandler);
	domainIsOff = 0;
	lastErrorCode = 0;
	DPRINTF("Opening connection to hypervisor, uri %s\n", uri ? uri : "probed");
	cp = virConnectOpen(uri);
	if (cp == NULL) {
		DPRINTF("virConnectOpen call failed\n");
		return NULL;
	}

	DPRINTF("Connected to %s\n", virConnectGetURI(cp));
	return cp;
}

char *libvirtGetDefaultEmulator(char *uri, char *type)
{
	char *caps = NULL;
	char *ret = NULL;
	char xp[1024] = { 0 };

	if (type == NULL)
		return NULL;

	if (cp == NULL) {
		cp = libvirtConnect(uri);
		if (cp == NULL) {
			DPRINTF("Connection to %s failed\n", uri);
			return NULL;
		}
	}

	if ((caps = virConnectGetCapabilities(cp)) == NULL)
		return NULL;

	snprintf(xp, sizeof(xp), "//capabilities/guest/arch/domain[@type='%s']/emulator", type);
	ret = xml_query(caps, xp);
	free(caps);

	return ret;
}

char *libvirtGetHostArchitecture(char *uri)
{
	char *caps = NULL;
	char *ret = NULL;

	if (cp == NULL) {
		cp = libvirtConnect(uri);
		if (cp == NULL) {
			DPRINTF("Connection to %s failed\n", uri);
			return NULL;
		}
	}

	if ((caps = virConnectGetCapabilities(cp)) == NULL)
		return NULL;

	ret = xml_query(caps, "//capabilities/host/cpu/arch");
	free(caps);
	return ret;
}

char *libvirtGetHypervisorType(char *uri)
{
	if (cp == NULL) {
		cp = libvirtConnect(uri);
		if (cp == NULL) {
			DPRINTF("Connection to %s failed\n", uri);
			return NULL;
		}
	}

	return (char *)virConnectGetType(cp);
}

int startDomain(char *xml, char *uri)
{
	int id, tries, res = 0;
	char *port = NULL;
	char *xmldesc = NULL;
	virDomainPtr dp = NULL;

	if (cp == NULL) {
		cp = libvirtConnect(uri);
		if (cp == NULL) {
			DPRINTF("Connection to %s failed\n", uri);
			return -EIO;
		}
	}

	DPRINTF("Starting domain\n");
	dp = virDomainCreateXML(cp, xml, 0);
	if (dp == NULL) {
		DPRINTF("virDomainCreateXML call failed\n");
		DPRINTF("XML File data:\n%s\n", xml);
		return -EINVAL;
	}

	DPRINTF("Domain started\n");

	tries = 0;
	xmldesc = virDomainGetXMLDesc(dp, 0);
	if (xmldesc == NULL) {
		if (tries > 5) {
			DPRINTF("Cannot get domain XML description\n");
			virDomainFree(dp);
			return -EIO;
		}

		sleep(1);
		tries++;
	}

	port = xml_query(xmldesc, "//domain/devices/graphics/@port");
	free(xmldesc);

	if (port == NULL) {
		DPRINTF("Port lookup failed, node not accessible\n");
		virDomainFree(dp);
		return -ENOENT;
	}

	DPRINTF("Graphics port number: %s\n", port);

	id = virDomainGetID(dp);
	DPRINTF("Domain created with ID %d\n", id);
#ifdef USE_HACK
	if (startVNCViewer(NULL, NULL, 1) != VNC_STATUS_UNSUPPORTED) {
		char path[1024] = { 0 };
		char buf[2048] = { 0 };
		char cmd[4096] = { 0 };

		snprintf(path, sizeof(path), "/proc/%d/exe", getpid());
		readlink(path, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "%s -v localhost:%s -f -l console 2> /dev/null", buf, port);
		DPRINTF("About to run '%s'\n", cmd);
		res = WEXITSTATUS(system(cmd));
	}
	else
		res = VNC_STATUS_NO_CONNECTION;
#else
	res = startVNCViewer("localhost", port, 1);
#endif
	if (((virDomainIsActive(dp)) && (!domainIsOff))
		|| (res != VNC_STATUS_SHUTDOWN)) {
		DPRINTF("Domain is active, destroying...\n");
		virDomainDestroy(dp);
	}

	DPRINTF("Domain %d done.\n", id);
	virDomainFree(dp);

	DPRINTF("Returning with %d\n", lastErrorCode);
	return lastErrorCode;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
