#include "conf.h"
#include "conf_esp.h"
#include "sdkconfig.h"

#include <stdlib.h>
#include <string.h>

static char s_password_buf[65];
static char s_admin_password_buf[65];
static const char *s_password = "";
static const char *s_admin_password = "";
static const char *s_cert = "";
static const char *s_key = "";
static char *s_cert_owned;
static char *s_key_owned;

static void set_owned_str(char *buf, size_t buflen, const char **slot, const char *src)
{
	if (!src)
		src = "";
	strncpy(buf, src, buflen - 1);
	buf[buflen - 1] = 0;
	*slot = buf;
}

void umurmur_conf_apply(const char *password, const char *admin_password,
			const char *cert_pem, const char *key_pem)
{
	/* Copy passwords — caller buffers (e.g. app_main stack) must not outlive us. */
	set_owned_str(s_password_buf, sizeof(s_password_buf), &s_password, password);
	set_owned_str(s_admin_password_buf, sizeof(s_admin_password_buf),
		      &s_admin_password, admin_password);

	free(s_cert_owned);
	free(s_key_owned);
	s_cert_owned = NULL;
	s_key_owned = NULL;
	s_cert = "";
	s_key = "";

	if (cert_pem && cert_pem[0]) {
		s_cert_owned = strdup(cert_pem);
		if (s_cert_owned)
			s_cert = s_cert_owned;
	}
	if (key_pem && key_pem[0]) {
		s_key_owned = strdup(key_pem);
		if (s_key_owned)
			s_key = s_key_owned;
	}
}

void Conf_init(const char *conffile)
{
	(void)conffile;
}

void Conf_deinit(void)
{
	free(s_cert_owned);
	free(s_key_owned);
	s_cert_owned = NULL;
	s_key_owned = NULL;
}

bool_t Conf_ok(const char *conffile)
{
	(void)conffile;
	return true;
}

const char *getStrConf(param_t param)
{
	switch (param) {
	case CERTIFICATE:
		return s_cert;
	case KEY:
		return s_key;
	case PASSPHRASE:
		return s_password;
	case ADMIN_PASSPHRASE:
		return s_admin_password;
	case WELCOMETEXT:
		return CONFIG_UMURMUR_WELCOME_TEXT;
	case DEFAULT_CHANNEL:
		return CONFIG_UMURMUR_CHANNEL_NAME;
	default:
		/* BINDADDR*, CAPATH, LOGFILE, BANFILE, USERNAME, GROUPNAME: unused on ESP */
		return NULL;
	}
}

int getIntConf(param_t param)
{
	switch (param) {
	case BINDPORT:
	case BINDPORT6:
		return CONFIG_UMURMUR_BIND_PORT;
	case MAX_BANDWIDTH:
		return CONFIG_UMURMUR_MAX_BANDWIDTH;
	case MAX_CLIENTS:
		return CONFIG_UMURMUR_MAX_USERS;
	case OPUS_THRESHOLD:
		return 100; /* % of clients that must support Opus (umurmur default) */
	default:
		/* BAN_LENGTH unused while ENABLE_BAN is false */
		return 0;
	}
}

bool_t getBoolConf(param_t param)
{
	switch (param) {
	case ALLOW_TEXTMESSAGE:
	case SHOW_ADDRESSES:
		return true;
	default:
		/* ENABLE_BAN / SYNC_BANFILE: off (no banfile; NVS ban list is future work) */
		return false;
	}
}

/* Single root channel (name from menuconfig). */
int Conf_getNextChannel(conf_channel_t *chdesc, int index)
{
	if (index != 0)
		return -1;
	*chdesc = (conf_channel_t){
		.parent = "",
		.name = CONFIG_UMURMUR_CHANNEL_NAME,
		.description = "",
		.password = NULL,
		.noenter = false,
		.silent = false,
		.allow_temp = false,
		.position = 0,
	};
	return 0;
}

int Conf_getNextChannelLink(conf_channel_link_t *chlink, int index)
{
	(void)chlink;
	(void)index;
	return -1;
}
