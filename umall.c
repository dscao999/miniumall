#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>

struct mount_path {
	char mpath[128];
	int level;
};
#define MAX_PATHS 20
static struct mount_path mps[MAX_PATHS];
static const char *filter = "/oldroot";

static int getlvl(const char *path)
{
	int lvl;
	const char *slash;

	lvl = 0;
	slash = strchr(path, '/');
	while (slash) {
		lvl++;
		slash = strchr(slash+1, '/');
	}
	return lvl;
}

static int getline(char *lbuf, int len, FILE *fh)
{
	int numb, ichar;
	char *cbuf;

	cbuf = lbuf;
	numb = 0;
	ichar = fgetc(fh);
	while (numb < len && ichar != EOF && ichar != '\n') {
		*cbuf = ichar;
		cbuf++;
		numb++;
		ichar = fgetc(fh);
	}
	if (numb < len)
		lbuf[numb] = 0;
	else 
		lbuf[len-1] = 0;
	return numb;
}

static int kexec_start(void)
{
	int retv, loaded;
	FILE *fh;

	retv = 0;
	fh = fopen("/sys/kernel/kexec_loaded", "rb");
	if (fh) {
		loaded = fgetc(fh) == '1'? 1 : 0;
		fclose(fh);
		if (loaded) {
			retv = execl("/sbin/kexec", "kexec", "-e", NULL);
			if (retv)
				fprintf(stderr, "kexec failed: %s\n",
					strerror(errno));
		} else {
			fprintf(stderr, "No kexec image loaded.\n");
			retv = 8;
		}
	} else {
		fprintf(stderr, "No kernel image loaded for exec!\n");
		retv = 1;
	}
	return retv;
}

int main(int argc, char *argv[])
{
	FILE *mfd;
	char *lbuf, *mpath;
	const int len = 1024;
	ssize_t nread;
	int numpaths, i;

	lbuf = malloc(len);
	if (lbuf == NULL) {
		fprintf(stderr, "Out of Memory!\n");
		return 10000;
	}

	mfd = fopen("/proc/mounts", "rb");
	if (mfd == NULL) {
		fprintf(stderr, "Cannot open /proc/mounts: %s\n",
			strerror(errno));
		free(lbuf);
		return 1000;
	}
	numpaths = 0;
	nread = getline(lbuf, len, mfd);
	while (nread > 0) {
		mpath = strtok(lbuf, " ");
		mpath = strtok(NULL, " ");
		if (mpath && strstr(mpath, filter) == mpath) {
			strcpy(mps[numpaths].mpath, mpath);
			mps[numpaths].level = getlvl(mpath);
			numpaths++;
		}
		nread = getline(lbuf, len, mfd);
	}
	fclose(mfd);

	int j, mlp, failed;
	struct mount_path mlpath;
	for (i = 0; i < numpaths; i++) {
		mlp = i;
		for (j = mlp + 1; j < numpaths; j++) {
			if (mps[mlp].level < mps[j].level)
				mlp = j;
		}
		if (mlp != i) {
			strcpy(mlpath.mpath, mps[mlp].mpath);
			mlpath.level = mps[mlp].level;
			strcpy(mps[mlp].mpath, mps[i].mpath);
			mps[mlp].level = mps[i].level;
			strcpy(mps[i].mpath, mlpath.mpath);
			mps[i].level = mlpath.level;
		}
	}
	failed = 0;
	for (i = 0; i < numpaths; i++)
		if (umount(mps[i].mpath) == -1) {
			fprintf(stderr, "Unable to umount %s: %s\n",
				mps[i].mpath, strerror(errno));
			failed = 1;
		}

	if (!failed && argc > 1) {
		if (strcmp(argv[1], "poweroff") == 0 ||
		    strcmp(argv[1], "reboot") == 0 ||
		    strcmp(argv[1], "halt") == 0) {
			strcpy(lbuf, "/sbin/");
			strcat(lbuf, argv[1]);
			if (execl(lbuf, argv[1], NULL))
				fprintf(stderr, "execl failed: %s\n",
					strerror(errno));
		} else if (strcmp(argv[1], "kexec") == 0)
			kexec_start();
		else
			fprintf(stderr, "Unknown action: %s\n", argv[1]);
	}
	free(lbuf);
	printf("Entering into sh...\n");
	if ((failed = execl("/bin/sh", "sh", NULL)))
		fprintf(stderr, "Cannot execute sh: %s\n",
			strerror(errno));
	return 0;
}
