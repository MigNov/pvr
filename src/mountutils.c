#define DEBUG_MOUNTUTILS

#include "vmrunner.h"

#ifdef DEBUG_MOUNTUTILS
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/mountutils ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

#if HAVE_LIBBLKID
char *getfstype(char *dev)
{
	blkid_cache cache = NULL;
	blkid_dev bdev;
	blkid_tag_iterate iter;
	const char *type, *value;
	char devPath[256];
	char *ret = NULL;

	if (blkid_get_cache(&cache, NULL) != 0)
		return NULL;

	snprintf(devPath, 256, "%s/%s", DEV_PATH, dev);
	bdev = blkid_get_dev(cache, devPath, BLKID_DEV_NORMAL);
	if (!bdev)
		return NULL;

	iter = blkid_tag_iterate_begin(bdev);
	while (blkid_tag_next(iter, &type, &value) == 0) {
		DPRINTF("%s: Type is '%s', value is '%s'\n", dev, type, value);
		if (strcmp(type, "TYPE") == 0) {
			ret = (char *) malloc( (strlen(value) + 1) * sizeof(char) );
			memset(ret, 0, (strlen(value) + 1) * sizeof(char));
			strcpy(ret, value);
			break;
		}
	}
	blkid_tag_iterate_end(iter);

	DPRINTF("Device %s fstype is %s\n", dev, ret);
	return ret;
}
#else
char *getfstype(char *dev)
{
	/* We can't get the fstype as we don't have libblkid compiled */
	return NULL;
}
#endif

int unmount_dev(char *dev)
{
	int res = -EBUSY, num = 0;
	DPRINTF("Unmounting %s\n", dev);
	while (res != 0) {
		num++;
		res = (umount(dev) == -1 ? -errno : 0);

		/* Set timeout to 10 seconds (10 tries after a second each) */
		if (num == 10)
			break;

		sleep(1);
	}
	if (res == 0)
		rmdir(dev);
	DPRINTF("%s unmount result: %d (in %d sec)\n", dev, res, num);
	return res;
}

char *mount_dev(char *dev, char *fstype, int *error)
{
	int ret = 0;
	time_t tm = time(NULL);
	char *tmp = NULL;
	char *tempdir = NULL;

	tempdir = (char *)malloc( 1024 * sizeof(char) );
	snprintf(tempdir, 1024, "/tmp/%d-%s-%s", (int)tm, dev, fstype);
	mkdir(tempdir, 0755);
	tmp = (char *) malloc( 512 * sizeof(char) );
	memset(tmp, 0, sizeof(512 * sizeof(char)) );
	snprintf(tmp, 512, "%s/%s", DEV_PATH, dev);
	DPRINTF("Mounting %s to %s\n", tmp, tempdir);
	if (mount(tmp, tempdir, fstype, MS_MGC_VAL, NULL) == -1) {
		int err = errno;
		DPRINTF("An error occured while mounting: %s\n", strerror(err));
		if (error != NULL)
			*error = -err;
		return NULL;
	}

	tmp = (char *)malloc( 512 * sizeof(char) );
	snprintf(tmp, 512, "%s/%s", tempdir, PVR_ROOT_FILE);
	ret = check_for_vm(tmp);
	if (ret == 0) {
			unmount_dev(tempdir);
		rmdir(tempdir);

		if (error != NULL)
			*error = 0;
		return NULL;
	}

	if (error != NULL)
   		*error = 0;

	DPRINTF("Got temporary directory: %s\n", tempdir);
	return tempdir;
}

char *check_dev_mountpoint(char *dev)
{
	char tmp[4096], tmp2[512], mp[1024];
	char *ret = NULL;
	FILE *fp = NULL;

	fp = fopen("/etc/mtab", "r");
	if (fp == NULL) {
		DPRINTF("Cannot open /etc/mtab to check mounts\n");
		return NULL;
	}

	while (!feof(fp)) {
		fgets(tmp, 4096, fp);
		if (strstr(tmp, dev) != NULL) {
			snprintf(tmp2, 512, "%s/%s %%s", DEV_PATH, dev);
			sscanf(tmp, tmp2, &mp);
			ret = (char *)malloc( strlen(mp) + 1 * sizeof(char) );
			memset(ret, 0, (strlen(mp) + 1) * sizeof(char) );
			strcpy(ret, mp);
			break;
		}
	}
	fclose(fp);

	if (ret != NULL)
		DPRINTF("%s/%s is mounted at %s\n", DEV_PATH, dev, ret);
	else
		DPRINTF("%s/%s is not mounted\n", DEV_PATH, dev);

	return ret;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
