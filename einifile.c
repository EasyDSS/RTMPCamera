
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "einifile.h"

#define dbg(fmt, ...) do {/*printf("[DEBUG %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);*/} while(0)
#define info(fmt, ...) do {printf("[INFO  %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define warn(fmt, ...) do {printf("[WARN  %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define err(fmt, ...) do {printf("[ERROR %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)

#if defined(__WIN32__)
#define strdup _strdup
#endif
#if defined(__LINUX__)
#endif

#define EINI_MAX_SEC_NUM			(64)
#define EINI_MAX_PARM_NUM_PER_SEC	(256)

struct eini_parameter
{
	char* key;
	char* value;
};

struct eini_section
{
	char* section;
	struct eini_parameter* parms[EINI_MAX_PARM_NUM_PER_SEC];
	int parmcnt;
};

struct eini_contex
{
	struct eini_section* secs[EINI_MAX_SEC_NUM];
	int seccnt;
	char buff[EINI_NAME_BUF_SIZ + EINI_VALUE_BUF_SIZ];
};

static int _eini_name_is_valid(const char* name)
{
	const char* ptr = name;
	if (NULL == ptr) {
		return 0;
	}
	if (*ptr == 0 || (!isalpha((unsigned char)(*ptr)) && *ptr != '_')) {
		return 0;
	}
	while (*ptr) {
		if (!isalnum((unsigned char)(*ptr)) && *ptr != '_') {
			return 0;
		}
		ptr ++;
		if (ptr - name + 1 > EINI_NAME_BUF_SIZ) {
			return 0;
		}
	}
	return 1;
}

static int _eini_value_is_valid(const char* value)
{
	const char* ptr = value;
	if (NULL == ptr) {
		return 0;
	}
	while (*ptr) {
		if (iscntrl((unsigned char)(*ptr))) {
			return 0;
		}
		ptr ++;
		if (ptr - value + 1 > EINI_VALUE_BUF_SIZ) {
			return 0;
		}
	}
	return 1;
}

static int _eini_sec_query(struct eini_contex* ctx, const char* section)
{
	int idx = 0;
	if (NULL == ctx || !_eini_name_is_valid(section)) {
		return -1;
	}

	for (idx = 0; idx < ctx->seccnt; idx++)
	{
		if (0 == strcmp(section, ctx->secs[idx]->section)) {
			return idx;
		}
	}
	return -1;
}

static int _eini_parm_query(struct eini_section* sec, const char* key, int keyidx)
{
	int idx, lastparm;
	if (NULL == sec || !_eini_name_is_valid(key)) {
		return -1;
	}

	for (idx = 0, lastparm = -1; idx < sec->parmcnt; idx++) {
		if (strcmp(key, sec->parms[idx]->key)) {
			continue;
		}
		if (keyidx == 0) {
			return idx;
		}
		if (keyidx > 0) {
			keyidx --;
		} else {
			lastparm = idx;
		}
	}
	if (lastparm >= 0) {
		return lastparm;
	}
	return -1;
}

static int _eini_sec_new(struct eini_contex* ctx, int secidx, const char* section)
{
	struct eini_section* sec = NULL;
	int idx;

	if (NULL == ctx || secidx < 0 || secidx > ctx->seccnt || !_eini_name_is_valid(section)) {
		return -1;
	}
	if (ctx->seccnt + 1 >= EINI_MAX_SEC_NUM) {
		err("BUG!!!, the number of section more then %d, please modeify code", EINI_MAX_SEC_NUM);
		return -1;
	}

	idx = _eini_sec_query(ctx, section);
	if (idx >= 0) {
		err("Section %s is exist !", section);
		return -1;
	}

	sec = (struct eini_section*) calloc(1, sizeof(struct eini_section));
	if (NULL == sec) {
		err("Calloc for new section failed, %s", strerror(errno));
		return -1;
	}
	sec->section = strdup(section);
	if (NULL == sec->section) {
		err("Strdup for section failed, %s", strerror(errno));
		free(sec);
		return -1;
	}

	for (idx = ctx->seccnt; idx > secidx; idx--) {
		ctx->secs[idx] = ctx->secs[idx-1];
	}
	ctx->secs[idx] = sec;
	ctx->seccnt ++;
	
	dbg("New section %s at secidx %d", section, idx);
	return 0;
}

static int _eini_parm_new(struct eini_section* sec, int parmidx, const char* key, const char* value)
{
	struct eini_parameter* parm = NULL;
	int idx;

	if (NULL == sec || parmidx < 0 || parmidx > sec->parmcnt || !_eini_name_is_valid(key) || !_eini_value_is_valid(value)) {
		return -1;
	}
	if (sec->parmcnt + 1 > EINI_MAX_PARM_NUM_PER_SEC) {
		err("BUG!!!, the number of parameter more then %d, please modeify code", EINI_MAX_PARM_NUM_PER_SEC);
		return -1;
	}

	parm = (struct eini_parameter*) calloc(1, sizeof(struct eini_parameter));
	if (NULL == parm) {
		err("Calloc for new parameter failed, %s", strerror(errno));
		return -1;
	}
	parm->key = strdup(key);
	if (NULL == parm->key) {
		err("Calloc for new key %s failed, %s", key, strerror(errno));
		free(parm);
		return -1;
	}
	parm->value = strdup(value);
	if (NULL == parm->value) {
		err("Calloc for new value %s failed, %s", value, strerror(errno));
		free(parm->key);
		free(parm);
		return -1;
	}

	for (idx = sec->parmcnt; idx > parmidx; idx--) {
		sec->parms[idx] = sec->parms[idx - 1];
	}
	sec->parms[idx] = parm;
	sec->parmcnt ++;
	
	dbg("New parameter %s=%s at parmidx %d", key, value, idx);
	return 0;
}

static int _eini_parm_delete(struct eini_section* sec, int parmidx)
{
	struct eini_parameter* parm = NULL;

	if (NULL == sec || parmidx < 0 || parmidx >= sec->parmcnt) {
		return -1;
	}

	parm = sec->parms[parmidx];
	free(parm->key);
	free(parm->value);
	free(parm);

	for (; parmidx + 1 < sec->parmcnt; parmidx++) {
		sec->parms[parmidx] = sec->parms[parmidx + 1];
	}
	sec->parmcnt --;
	return 0;
}

static int _eini_sec_delete(struct eini_contex* ctx, int secidx)
{
	struct eini_section* sec = NULL;

	if (NULL == ctx || secidx < 0 || secidx >= ctx->seccnt) {
		return -1;
	}

	sec = ctx->secs[secidx];
	while (sec->parmcnt > 0) {
		_eini_parm_delete(sec, 0);
	}

	free(sec->section);
	free(sec);

	for (; secidx + 1 < ctx->seccnt; secidx++) {
		ctx->secs[secidx] = ctx->secs[secidx + 1];
	}
	ctx->seccnt --;
	return 0;
}

static int _eini_fgetline(FILE* fp, char *buff, int len)
{
	int count = 0, ch;
	if (NULL == fp || NULL == buff || len < 1) {
		return -1;
	}

	while (count < len - 1 && (ch = fgetc(fp)) >= 0) {
		if (ch == '\r' || ch == '\n') {
			if (count == 0) {
				continue;
			}
			break;
		}
		if (iscntrl(ch)) {
			continue;
		}
		buff[count] = ch;
		count ++;
	}

	buff[count] = 0;
	return count;
}

eini_handle eini_alloc(void)
{
	struct eini_contex* ctx = (struct eini_contex*) calloc(1, sizeof(struct eini_contex));
	if (NULL == ctx) {
		err("Calloc for eini contex failed, %s", strerror(errno));
		return NULL;
	}
	return ctx;
}

int eini_load_file(eini_handle h, const char* file)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	struct eini_section* sec = NULL;
	FILE* fp = NULL;
	int linelen;

	if (NULL == ctx || NULL == file) {
		return -1;
	}

	fp = fopen(file, "r");
	if (NULL == fp) {
		err("Open file %s failed, %s", file, strerror(errno));
		return -1;
	}

	while ((linelen = _eini_fgetline(fp, ctx->buff, sizeof(ctx->buff))) > 0)
	{
		char *name, *value, *ptr;

		//space line
		if (linelen == 0) {
			continue;
		}

		ptr = ctx->buff;
		while (*ptr && *ptr == ' ') ptr++;//skip head space
		if (*ptr == 0 || ptr[0] == ';' || ptr[0] == '#') {
			continue;
		}
		name = ptr;

		if (*name == '[') {
			//is section
			sec = NULL; //new section

			ptr = name + strlen(name) - 1;
			while (ptr > name && *ptr && *ptr == ' ') ptr--;//skip tail space
			if (ptr <= name || *ptr == 0 || *ptr != ']') {
				warn("section format error %s", name);
				sec = NULL;
				continue;
			}
			ptr[1] = 0;

			name ++;
			*ptr = 0;
			if (!_eini_name_is_valid(name)) {
				warn("Invalid section string %s", name);
				continue;
			}
			if (0 == _eini_sec_new(ctx, ctx->seccnt, name)) {
				sec = ctx->secs[ctx->seccnt - 1];
			} else {
				warn("BUG!!! _eini_sec_new failed");
			}
			continue;
		}

		//is parameter
		if (NULL == sec) {
			warn("Not found section for parameter %s", name);
			continue;
		}

		//find = and value
		ptr  = name;
		while (*ptr && *ptr != '=') ptr++;
		if (*ptr == 0) {
			continue;
		}
		*ptr = 0;
		value = ptr + 1;
		ptr --;

		//skip name tail space
		while (ptr > name && *ptr && *ptr == ' ') ptr--;
		if (ptr <= name || *ptr == 0) {
			continue;
		}
		ptr[1] = 0;

		//skip value head space
		ptr = value;
		while (*ptr && *ptr == ' ') ptr++;
		value = ptr;

		if (!_eini_name_is_valid(name) || !_eini_value_is_valid(value)) {
			warn("Invalid parameter %s=%s", name, value);
			continue;
		}
		if (0 != _eini_parm_new(sec, sec->parmcnt, name, value)) {
			warn("BUG!!! _eini_parm_new failed");
		}
	}
	
	fclose(fp);
	return 0;
}

int eini_count(eini_handle h, int secidx)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	
	if (NULL == ctx || secidx >= ctx->seccnt) {
		return -1;
	}

	if (secidx < 0) {
		return ctx->seccnt;
	}

	return ctx->secs[secidx]->parmcnt;
}

int eini_query(eini_handle h, int secidx, const char* name, int keyidx)
{
	struct eini_contex* ctx = (struct eini_contex*) h;

	if (NULL == ctx || secidx >= ctx->seccnt) {
		return -1;
	}

	if (secidx < 0) {
		return _eini_sec_query(ctx, name);
	}

	return _eini_parm_query(ctx->secs[secidx], name, keyidx);
}

int eini_new(eini_handle h, int secidx, int parmidx, const char* name, const char* value)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	struct eini_section* sec = NULL;

	if (NULL == ctx || secidx < 0 || secidx > ctx->seccnt) {
		return -1;
	}

	if (parmidx < 0) {
		return _eini_sec_new(ctx, secidx, name);
	}

	if (secidx == ctx->seccnt) {
		return -1;
	}

	sec = ctx->secs[secidx];

	if (parmidx > sec->parmcnt) {
		return -1;
	}

	return _eini_parm_new(sec, parmidx, name, value);
}

int eini_get(eini_handle h, int secidx, int parmidx, char* namebuf, char* valuebuf)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	struct eini_section* sec = NULL;
	struct eini_parameter* parm = NULL;

	if (NULL == ctx || secidx < 0 || secidx >= ctx->seccnt) {
		return -1;
	}

	sec = ctx->secs[secidx];

	if (parmidx < 0) {
		if (namebuf) {
			strcpy(namebuf, sec->section);
		}
		return 0;
	}

	if (parmidx >= sec->parmcnt) {
		return -1;
	}

	parm = sec->parms[parmidx];

	if (namebuf) {
		strcpy(namebuf, parm->key);
	}
	if (valuebuf) {
		strcpy(valuebuf, parm->value);
	}
	return 0;
}

int eini_set(eini_handle h, int secidx, int parmidx, const char* name, const char* value)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	struct eini_section* sec = NULL;
	struct eini_parameter* parm = NULL;
	char* name1 = NULL;
	char* value1 = NULL;

	if (NULL == ctx || secidx < 0 || secidx >= ctx->seccnt) {
		return -1;
	}

	sec = ctx->secs[secidx];

	if (parmidx < 0) {
		if (name) {
			if (!_eini_name_is_valid(name)) {
				return -1;
			}
			name1 = strdup(name);
			if (NULL == name1) {
				err("Strdup for new section %s failed, %s", name, strerror(errno));
				return -1;
			}
		}
		if (name1) {
			free(sec->section);
			sec->section = name1;
		}
		return 0;
	}

	if (parmidx >= sec->parmcnt) {
		return -1;
	}

	if (name) {
		if (!_eini_name_is_valid(name)) {
			return -1;
		}
		name1 = strdup(name);
		if (NULL == name1) {
			err("Calloc for new key %s failed, %s", name, strerror(errno));
			return -1;
		}
	}

	if (value) {
		if (!_eini_value_is_valid(value)) {
			return -1;
		}
		value1 = strdup(value);
		if (NULL == value1) {
			err("Calloc for new value %s failed, %s", value, strerror(errno));
			if (name1) {
				free(name1);
			}
			return -1;
		}	
	}

	parm = sec->parms[parmidx];
	if (name1) {
		free(parm->key);
		parm->key = name1;
	}
	if (value1) {
		free(parm->value);
		parm->value = value1;
	}
	return 0;
}

int eini_delete(eini_handle h, int secidx, int parmidx)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	struct eini_section* sec = NULL;

	if (NULL == ctx || secidx < 0 || secidx >= ctx->seccnt) {
		return -1;
	}

	if (parmidx < 0) {
		return _eini_sec_delete(ctx, secidx);
	}

	sec = ctx->secs[secidx];

	if (parmidx >= sec->parmcnt) {
		return -1;
	}

	return _eini_parm_delete(sec, parmidx);
}

int eini_save_file(eini_handle h, const char* file)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	FILE *fp = NULL;
	int secidx;

	if (NULL == ctx || NULL == file) {
		return -1;
	}

	fp = fopen(file, "w");
	if (NULL == fp) {
		err("Open %s failed, %s", file, strerror(errno));
		return -1;
	}

	fprintf(fp, "; \n");
	fprintf(fp, "; YOU CAN EDIT THIS FILE\n");
	fprintf(fp, "; It may be generated by einifile library\n");
	fprintf(fp, "; Not support write comments to file\n");
	fprintf(fp, "; \n\n");

	for (secidx = 0; secidx < ctx->seccnt; secidx++) {
		struct eini_section* sec = ctx->secs[secidx];
		int parmidx;
		fprintf(fp, "[%s]\n", sec->section);
		for (parmidx = 0; parmidx < sec->parmcnt; parmidx++) {
			struct eini_parameter* parm = sec->parms[parmidx];
			fprintf(fp, "%s=%s\n", parm->key, parm->value);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
	return 0;
}

void eini_free(eini_handle h)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	if (ctx) {
		while (ctx->seccnt > 0) {
			_eini_sec_delete(ctx, 0);
		}
		free(ctx);
	}
}

int eini_ext_new(eini_handle h, const char* sec_key, const char* value)
{
	char section[EINI_NAME_BUF_SIZ] = "";
	char key[EINI_NAME_BUF_SIZ] = "";
	const char* ptr = NULL;
	int secidx, parmidx;

	if (NULL == h || NULL == sec_key) {
		return -1;
	}

	ptr = strstr(sec_key, ".");
	if (NULL == ptr) {
		secidx = eini_count(h, -1);
		if (secidx < 0) {
			return secidx;
		}
		return eini_new(h, secidx, -1, sec_key, NULL);
	}

	if (ptr - sec_key + 1 > EINI_NAME_BUF_SIZ || strlen(sec_key) - (ptr - sec_key) - 1 + 1 > EINI_NAME_BUF_SIZ) {
		return -1;
	}

	strncpy(section, sec_key, ptr - sec_key);
	strncpy(key, ptr + 1, EINI_NAME_BUF_SIZ);
	
	secidx = eini_query(h, -1, section, 0);
	if (secidx < 0) {
		secidx = eini_count(h, -1);
		if (secidx < 0 || eini_new(h, secidx, -1, section, NULL) < 0) {
			return -1;
		}
	}
	parmidx = eini_count(h, secidx);
	if (parmidx < 0) {
		return parmidx;
	}

	return eini_new(h, secidx, parmidx, key, value);
}

int eini_ext_get(eini_handle h, const char* sec_key, char* valuebuf)
{
	char section[EINI_NAME_BUF_SIZ] = "";
	char key[EINI_NAME_BUF_SIZ] = "";
	const char* ptr = NULL;
	int secidx, parmidx, keyidx;

	if (NULL == h || NULL == sec_key) {
		return -1;
	}

	ptr = strstr(sec_key, ".");
	if (NULL == ptr) {
		err("sec_key format error, SEC.KEY[.KEYIDX]");
		return -1;
	}

	if (ptr - sec_key + 1 > EINI_NAME_BUF_SIZ) {
		return -1;
	}
	strncpy(section, sec_key, ptr - sec_key);

	sec_key = ptr + 1;
	ptr = strstr(sec_key, ".");
	if (ptr) {
		if (ptr - sec_key + 1 > EINI_NAME_BUF_SIZ) {
			return -1;
		}
		strncpy(key, sec_key, ptr - sec_key);
		keyidx = atoi(ptr+1);
	} else {
		if (strlen(sec_key) + 1 > EINI_NAME_BUF_SIZ) {
			return -1;
		}
		strncpy(key, sec_key, EINI_NAME_BUF_SIZ);
		keyidx = -1;
	}
	
	secidx = eini_query(h, -1, section, 0);
	if (secidx < 0) {
		err("Not found section %s", section);
		return secidx;
	}
	parmidx = eini_query(h, secidx, key, keyidx);
	if (parmidx < 0) {
		err("Not found key %s", key);
		return parmidx;
	}

	return eini_get(h, secidx, parmidx, NULL, valuebuf);
}

int eini_ext_set(eini_handle h, const char* sec_key, const char* value)
{
	char section[EINI_NAME_BUF_SIZ] = "";
	char key[EINI_NAME_BUF_SIZ] = "";
	const char* ptr = NULL;
	int secidx, parmidx, keyidx;

	if (NULL == h || NULL == sec_key) {
		return -1;
	}

	ptr = strstr(sec_key, ".");
	if (NULL == ptr) {
		err("sec_key format error, SEC.KEY[.KEYIDX]");
		return -1;
	}

	if (ptr - sec_key + 1 > EINI_NAME_BUF_SIZ) {
		return -1;
	}
	strncpy(section, sec_key, ptr - sec_key);

	sec_key = ptr + 1;
	ptr = strstr(sec_key, ".");
	if (ptr) {
		if (ptr - sec_key + 1 > EINI_NAME_BUF_SIZ) {
			return -1;
		}
		strncpy(key, sec_key, ptr - sec_key);
		keyidx = atoi(ptr+1);
	} else {
		if (strlen(sec_key) + 1 > EINI_NAME_BUF_SIZ) {
			return -1;
		}
		strncpy(key, sec_key, EINI_NAME_BUF_SIZ);
		keyidx = -1;
	}
	
	secidx = eini_query(h, -1, section, 0);
	if (secidx < 0) {
		err("Not found section %s", section);
		return secidx;
	}
	parmidx = eini_query(h, secidx, key, keyidx);
	if (parmidx < 0) {
		err("Not found key %s", key);
		return parmidx;
	}

	return eini_set(h, secidx, parmidx, NULL, value);
}

int eini_ext_delete(eini_handle h, const char* sec_key)
{
	char section[EINI_NAME_BUF_SIZ] = "";
	char key[EINI_NAME_BUF_SIZ] = "";
	const char* ptr = NULL;
	int secidx, parmidx;

	if (NULL == h || NULL == sec_key) {
		return -1;
	}

	ptr = strstr(sec_key, ".");
	if (NULL == ptr) {
		secidx = eini_query(h, -1, sec_key, 0);
		if (secidx < 0) {
			return secidx;
		}
		return eini_delete(h, secidx, -1);
	}

	if (ptr - sec_key + 1 > EINI_NAME_BUF_SIZ || strlen(sec_key) - (ptr - sec_key) - 1 + 1 > EINI_NAME_BUF_SIZ) {
		return -1;
	}

	strncpy(section, sec_key, ptr - sec_key);
	strncpy(key, ptr + 1, EINI_NAME_BUF_SIZ);
	
	secidx = eini_query(h, -1, section, 0);
	if (secidx < 0) {
		err("Not found section %s", section);
		return secidx;
	}
	parmidx = eini_query(h, secidx, key, -1);
	if (parmidx < 0) {
		return parmidx;
	}

	return eini_delete(h, secidx, parmidx);
}

#if defined(__WIN32__)
/* 
 * for vsscanf function
 * copy from linux-3.4.y
 **/
/**
 * div_u64_rem - unsigned 64bit divide with 32bit divisor with remainder
 *
 * This is commonly provided by 32bit archs to provide an optimized 64bit
 * divide.
 */
static unsigned long long div_u64_rem(unsigned long long dividend, unsigned int divisor, unsigned int *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

/**
 * div_u64 - unsigned 64bit divide with 32bit divisor
 *
 * This is the most common 64bit divide and should be used if possible,
 * as many 32bit archs can optimize this variant better than a full 64bit
 * divide.
 */
static unsigned long long div_u64(unsigned long long dividend, unsigned int divisor)
{
	unsigned int remainder;
	return div_u64_rem(dividend, divisor, &remainder);
}

#define KSTRTOX_OVERFLOW	(1U << 31)

static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
	if (*base == 0) {
		if (s[0] == '0') {
			if (_tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
			else
				*base = 8;
		} else
			*base = 10;
	}
	if (*base == 16 && s[0] == '0' && _tolower(s[1]) == 'x')
		s += 2;
	return s;
}

#define unlikely(x) (x)

/*
 * Convert non-negative integer string representation in explicitly given radix
 * to an integer.
 * Return number of characters consumed maybe or-ed with overflow bit.
 * If overflow occurs, result integer (incorrect) is still returned.
 *
 * Don't you dare use this function.
 */
static unsigned int _parse_integer(const char *s, unsigned int base, unsigned long long *p)
{
	unsigned long long res;
	unsigned int rv;
	int overflow;

	res = 0;
	rv = 0;
	overflow = 0;
	while (*s) {
		unsigned int val;

		if ('0' <= *s && *s <= '9')
			val = *s - '0';
		else if ('a' <= _tolower(*s) && _tolower(*s) <= 'f')
			val = _tolower(*s) - 'a' + 10;
		else
			break;

		if (val >= base)
			break;
		/*
		 * Check for overflow only if we are within range of
		 * it in the max base we support (16)
		 */
		if (unlikely(res & (~0ull << 60))) {
			if (res > div_u64(ULLONG_MAX - val, base))
				overflow = 1;
		}
		res = res * base + val;
		rv++;
		s++;
	}
	*p = res;
	if (overflow)
		rv |= KSTRTOX_OVERFLOW;
	return rv;
}

/**
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
static unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
	unsigned long long result;
	unsigned int rv;

	cp = _parse_integer_fixup_radix(cp, &base);
	rv = _parse_integer(cp, base, &result);
	/* FIXME */
	cp += (rv & ~KSTRTOX_OVERFLOW);

	if (endp)
		*endp = (char *)cp;

	return result;
}

/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
static unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	return (unsigned long)simple_strtoull(cp, endp, base);
}

/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
static long simple_strtol(const char *cp, char **endp, unsigned int base)
{
	if (*cp == '-')
		return -(long)simple_strtoul(cp + 1, endp, base);

	return simple_strtoul(cp, endp, base);
}

/**
 * simple_strtoll - convert a string to a signed long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
static long long simple_strtoll(const char *cp, char **endp, unsigned int base)
{
	if (*cp == '-')
		return -(long long)simple_strtoull(cp + 1, endp, base);

	return simple_strtoull(cp, endp, base);
}

static int skip_atoi(const char **s)
{
	int i = 0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';

	return i;
}

/**
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *
 * Returns a pointer to the first non-whitespace character in @str.
 */
static char *skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}

/**
 * vsscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	format of buffer
 * @args:	arguments
 */
//FIXME not support like %[A-Z] or %[^A-Z] format
static int vsscanf(const char *buf, const char *fmt, va_list args)
{
	const char *str = buf;
	char *next;
	char digit;
	int num = 0;
	unsigned char qualifier;
	unsigned char base;
	short field_width;
	int is_sign;

	while (*fmt && *str) {
		/* skip any white space in format */
		/* white space in format matchs any amount of
		 * white space, including none, in the input.
		 */
		if (isspace(*fmt)) {
			fmt = skip_spaces(++fmt);
			str = skip_spaces(str);
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;

		/* skip this conversion.
		 * advance both strings to next white space
		 */
		if (*fmt == '*') {
			while (!isspace(*fmt) && *fmt != '%' && *fmt)
				fmt++;
			while (!isspace(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || _tolower(*fmt) == 'l' ||
		    _tolower(*fmt) == 'z') {
			qualifier = *fmt++;
			if (unlikely(qualifier == *fmt)) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}

		if (!*fmt || !*str)
			break;

		base = 10;
		is_sign = 0;

		switch (*fmt++) {
		case 'c':
		{
			char *s = (char *)va_arg(args, char*);
			if (field_width == -1)
				field_width = 1;
			do {
				*s++ = *str++;
			} while (--field_width > 0 && *str);
			num++;
		}
		continue;
		case 's':
		{
			char *s = (char *)va_arg(args, char *);
			if (field_width == -1)
				field_width = SHRT_MAX;
			/* first, skip leading white space in buffer */
			str = skip_spaces(str);

			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width--)
				*s++ = *str++;
			*s = '\0';
			num++;
		}
		continue;
		case 'n':
			/* return number of characters read so far */
		{
			int *i = (int *)va_arg(args, int*);
			*i = str - buf;
		}
		continue;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		case 'i':
			base = 0;
		case 'd':
			is_sign = 1;
		case 'u':
			break;
		case '%':
			/* looking for '%' in str */
			if (*str++ != '%')
				return num;
			continue;
		default:
			/* invalid format; stop here */
			return num;
		}

		/* have some sort of integer conversion.
		 * first, skip white space in buffer.
		 */
		str = skip_spaces(str);

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
		    || (base == 16 && !isxdigit(digit))
		    || (base == 10 && !isdigit(digit))
		    || (base == 8 && (!isdigit(digit) || digit > '7'))
		    || (base == 0 && !isdigit(digit)))
			break;

		switch (qualifier) {
		case 'H':	/* that's 'hh' in format */
			if (is_sign) {
				signed char *s = (signed char *)va_arg(args, signed char *);
				*s = (signed char)simple_strtol(str, &next, base);
			} else {
				unsigned char *s = (unsigned char *)va_arg(args, unsigned char *);
				*s = (unsigned char)simple_strtoul(str, &next, base);
			}
			break;
		case 'h':
			if (is_sign) {
				short *s = (short *)va_arg(args, short *);
				*s = (short)simple_strtol(str, &next, base);
			} else {
				unsigned short *s = (unsigned short *)va_arg(args, unsigned short *);
				*s = (unsigned short)simple_strtoul(str, &next, base);
			}
			break;
		case 'l':
			if (is_sign) {
				long *l = (long *)va_arg(args, long *);
				*l = simple_strtol(str, &next, base);
			} else {
				unsigned long *l = (unsigned long *)va_arg(args, unsigned long *);
				*l = simple_strtoul(str, &next, base);
			}
			break;
		case 'L':
			if (is_sign) {
				long long *l = (long long *)va_arg(args, long long *);
				*l = simple_strtoll(str, &next, base);
			} else {
				unsigned long long *l = (unsigned long long *)va_arg(args, unsigned long long *);
				*l = simple_strtoull(str, &next, base);
			}
			break;
		case 'Z':
		case 'z':
		{
			size_t *s = (size_t *)va_arg(args, size_t *);
			*s = (size_t)simple_strtoul(str, &next, base);
		}
		break;
		default:
			if (is_sign) {
				int *i = (int *)va_arg(args, int *);
				*i = (int)simple_strtol(str, &next, base);
			} else {
				unsigned int *i = (unsigned int *)va_arg(args, unsigned int*);
				*i = (unsigned int)simple_strtoul(str, &next, base);
			}
			break;
		}
		num++;

		if (!next)
			break;
		str = next;
	}

	/*
	 * Now we've come all the way through so either the input string or the
	 * format ended. In the former case, there can be a %n at the current
	 * position in the format that needs to be filled.
	 */
	if (*fmt == '%' && *(fmt + 1) == 'n') {
		int *p = (int *)va_arg(args, int *);
		*p = str - buf;
	}

	return num;
}
#endif

int eini_ext_scanf(eini_handle h, const char* sec_key, const char* fmt, ...)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	va_list argp;
	int ret;

	if (NULL == ctx || NULL == sec_key || NULL == fmt) {
		return -1;
	}

	ret = eini_ext_get(h, sec_key, ctx->buff);
	if (ret < 0) {
		return ret;
	}

	va_start(argp, fmt);
	ret = vsscanf(ctx->buff, fmt, argp);
	va_end(argp);

	return ret;
}

int eini_ext_printf(eini_handle h, const char* sec_key, const char* fmt, ...)
{
	struct eini_contex* ctx = (struct eini_contex*) h;
	va_list argp;
	int ret;

	if (NULL == ctx || NULL == sec_key || NULL == fmt) {
		return -1;
	}

    va_start(argp, fmt);
    ret = vsnprintf(ctx->buff, sizeof(ctx->buff), fmt, argp);
    va_end(argp);

	if (ret <= 0) {
		err("vsnprintf failed, %s", strerror(errno));
		return -1;
	}

	return eini_ext_set(ctx, sec_key, ctx->buff);
}


