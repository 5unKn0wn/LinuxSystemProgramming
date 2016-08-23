#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libudev.h>

#include "names.h"
#include "usb-spec.h"

#define HASH1  0x10
#define HASH2  0x02
#define HASHSZ 512

static unsigned int hashnum(unsigned int num)
{
	unsigned int mask1 = HASH1 << 27, mask2 = HASH2 << 27;

	for (; mask1 >= HASH1; mask1 >>= 1, mask2 >>= 1)
		if (num & mask1)
			num ^= mask2;
	return num & (HASHSZ-1);
}

/* ---------------------------------------------------------------------- */

static struct udev *udev = NULL;
static struct udev_hwdb *hwdb = NULL;
static struct audioterminal *audioterminals_hash[HASHSZ] = { NULL, };
static struct videoterminal *videoterminals_hash[HASHSZ] = { NULL, };
static struct genericstrtable *hiddescriptors_hash[HASHSZ] = { NULL, };
static struct genericstrtable *reports_hash[HASHSZ] = { NULL, };
static struct genericstrtable *huts_hash[HASHSZ] = { NULL, };
static struct genericstrtable *biass_hash[HASHSZ] = { NULL, };
static struct genericstrtable *physdess_hash[HASHSZ] = { NULL, };
static struct genericstrtable *hutus_hash[HASHSZ] = { NULL, };
static struct genericstrtable *langids_hash[HASHSZ] = { NULL, };
static struct genericstrtable *countrycodes_hash[HASHSZ] = { NULL, };

/* ---------------------------------------------------------------------- */

static const char *hwdb_get(const char *modalias, const char *key)
{
	struct udev_list_entry *entry;

	udev_list_entry_foreach(entry, udev_hwdb_get_properties_list_entry(hwdb, modalias, 0))
		if (strcmp(udev_list_entry_get_name(entry), key) == 0)
			return udev_list_entry_get_value(entry);

	return NULL;
}

const char *names_vendor(u_int16_t vendorid)
{
	char modalias[64];

	sprintf(modalias, "usb:v%04X*", vendorid);
	return hwdb_get(modalias, "ID_VENDOR_FROM_DATABASE");
}

const char *names_product(u_int16_t vendorid, u_int16_t productid)
{
	char modalias[64];

	sprintf(modalias, "usb:v%04Xp%04X*", vendorid, productid);
	return hwdb_get(modalias, "ID_MODEL_FROM_DATABASE");
}

const char *names_class(u_int8_t classid)
{
	char modalias[64];

	sprintf(modalias, "usb:v*p*d*dc%02X*", classid);
	return hwdb_get(modalias, "ID_USB_CLASS_FROM_DATABASE");
}

const char *names_subclass(u_int8_t classid, u_int8_t subclassid)
{
	char modalias[64];

	sprintf(modalias, "usb:v*p*d*dc%02Xdsc%02X*", classid, subclassid);
	return hwdb_get(modalias, "ID_USB_SUBCLASS_FROM_DATABASE");
}

const char *names_protocol(u_int8_t classid, u_int8_t subclassid, u_int8_t protocolid)
{
	char modalias[64];

	sprintf(modalias, "usb:v*p*d*dc%02Xdsc%02Xdp%02X*", classid, subclassid, protocolid);
	return hwdb_get(modalias, "ID_USB_PROTOCOL_FROM_DATABASE");
}

/* ---------------------------------------------------------------------- */

int get_vendor_string(char *buf, size_t size, u_int16_t vid)
{
        const char *cp;

        if (size < 1)
                return 0;
        *buf = 0;
        if (!(cp = names_vendor(vid)))
                return 0;
        return snprintf(buf, size, "%s", cp);
}

int get_product_string(char *buf, size_t size, u_int16_t vid, u_int16_t pid)
{
        const char *cp;

        if (size < 1)
                return 0;
        *buf = 0;
        if (!(cp = names_product(vid, pid)))
                return 0;
        return snprintf(buf, size, "%s", cp);
}

int get_class_string(char *buf, size_t size, u_int8_t cls)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_class(cls)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_subclass_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_subclass(cls, subcls)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_protocol_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls, u_int8_t proto)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_protocol(cls, subcls, proto)))
		return 0;
	
	return snprintf(buf, size, "%s", cp);
}

/* ---------------------------------------------------------------------- */

static int hash_audioterminal(struct audioterminal *at)
{
	struct audioterminal *at_old;
	unsigned int h = hashnum(at->termt);

	for (at_old = audioterminals_hash[h]; at_old; at_old = at_old->next)
		if (at_old->termt == at->termt)
			return -1;
	at->next = audioterminals_hash[h];
	audioterminals_hash[h] = at;
	return 0;
}

static int hash_audioterminals(void)
{
	int r = 0, i, k;

	for (i = 0; audioterminals[i].name; i++)
	{
		k = hash_audioterminal(&audioterminals[i]);
		if (k < 0)
			r = k;
	}

	return r;
}

static int hash_videoterminal(struct videoterminal *vt)
{
	struct videoterminal *vt_old;
	unsigned int h = hashnum(vt->termt);

	for (vt_old = videoterminals_hash[h]; vt_old; vt_old = vt_old->next)
		if (vt_old->termt == vt->termt)
			return -1;
	vt->next = videoterminals_hash[h];
	videoterminals_hash[h] = vt;
	return 0;
}

static int hash_videoterminals(void)
{
	int r = 0, i, k;

	for (i = 0; videoterminals[i].name; i++)
	{
		k = hash_videoterminal(&videoterminals[i]);
		if (k < 0)
			r = k;
	}

	return r;
}

static int hash_genericstrtable(struct genericstrtable *t[HASHSZ],
			       struct genericstrtable *g)
{
	struct genericstrtable *g_old;
	unsigned int h = hashnum(g->num);

	for (g_old = t[h]; g_old; g_old = g_old->next)
		if (g_old->num == g->num)
			return -1;
	g->next = t[h];
	t[h] = g;
	return 0;
}

#define HASH_EACH(array, hash) \
	for (i = 0; array[i].name; i++) { \
		k = hash_genericstrtable(hash, &array[i]); \
		if (k < 0) { \
			r = k; \
		}\
	}

static int hash_tables(void)
{
	int r = 0, k, i;

	k = hash_audioterminals();
	if (k < 0)
		r = k;

	k = hash_videoterminals();
	if (k < 0)
		r = k;

	HASH_EACH(hiddescriptors, hiddescriptors_hash);

	HASH_EACH(reports, reports_hash);

	HASH_EACH(huts, huts_hash);

	HASH_EACH(hutus, hutus_hash);

	HASH_EACH(langids, langids_hash);

	HASH_EACH(physdess, physdess_hash);

	HASH_EACH(biass, biass_hash);

	HASH_EACH(countrycodes, countrycodes_hash);

	return r;
}

int names_init(void)
{
	int r;

	udev = udev_new();
	if (!udev)
		r = -1;
	else {
		hwdb = udev_hwdb_new(udev);
		if (!hwdb)
			r = -1;
	}

	r = hash_tables();

	return r;
}

void names_exit(void)
{
	hwdb = udev_hwdb_unref(hwdb);
	udev = udev_unref(udev);
}
