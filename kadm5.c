/*
 *
 * php4-kadm5: remote administration of Kerberos Administration Servers
 * Copyright (C) 2003 GONICUS GmbH.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	 02111-1307	 USA
 *
 * Author:	Holger Burbach <holger.burbach@GONICUS.de>
 *			GONICUS GmbH
 *			Moehnestrasse 11-17
 *			D-59755 Arnsberg
 *			http://www.GONICUS.de
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef PACKAGE_BUGREPORT
#undef PACKAGE_STRING
#undef PACKAGE_NAME
#undef PACKAGE_VERSION
#undef PACKAGE_TARNAME

#include "kadm5/admin.h"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_kadm5.h"

#define HANDLE_ID "KADM5 handle"
#define BUFSIZE	  2048

#define OP_PRINCIPAL			"principal"
#define OP_PRINC_EXPIRE_TIME	"princ_expire_time"
#define OP_PW_EXPIRATION		"pw_expiration"
#define OP_LAST_PW_CHANGE		"last_pwd_change"
#define OP_ATTRIBUTES			"attributes"
#define OP_MAX_LIFE				"max_life"
#define OP_MOD_TIME				"mod_time"
#define OP_MOD_NAME				"mod_name"
#define OP_KVNO					"kvno"
#define OP_MKVNO				"mkvno"
#define OP_AUX_ATTRIBUTES		"aux_attributes"
#define OP_POLICY				"policy"
#define OP_CLEARPOLICY			"clearpolicy"
#define OP_RANDKEY				"randkey"
#define OP_MAX_RLIFE			"max_renewable_life"
#define OP_LAST_SUCCESS			"last_success"
#define OP_LAST_FAILED			"last_failed"
#define OP_FAIL_AUTH_COUNT		"fail_auth_count"

#define OP_PW_MAX_LIFE			"pw_max_life"
#define OP_PW_MIN_LIFE			"pw_min_life"
#define OP_PW_MIN_LENGTH		"pw_min_length"
#define OP_PW_MIN_CLASSES		"pw_min_classes"
#define OP_PW_HISTORY_NUM		"pw_history_num"
#define OP_REF_COUNT			"pw_refcnt"


/* True global resources - no need for thread safety here */
static int le_handle;


/* {{{ kadm5_functions[]
 *
 * Every user visible function must have an entry in kadm5_functions[].
 */
function_entry kadm5_functions[] = {
	PHP_FE(kadm5_init_with_password, NULL)
	PHP_FE(kadm5_destroy, NULL)
	PHP_FE(kadm5_flush, NULL)
	PHP_FE(kadm5_create_principal, NULL)
	PHP_FE(kadm5_delete_principal, NULL)
	PHP_FE(kadm5_modify_principal, NULL)
	PHP_FE(kadm5_chpass_principal, NULL)
	PHP_FE(kadm5_get_principals, NULL)
	PHP_FE(kadm5_get_principal, NULL)
	PHP_FE(kadm5_get_policies, NULL)
	{NULL, NULL, NULL}	/* Must be the last line in kadm5_functions[] */
};
/* }}} */


/* {{{ kadm5_module_entry
 */
zend_module_entry kadm5_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"kadm5",
	kadm5_functions,
	PHP_MINIT(kadm5),
	PHP_MSHUTDOWN(kadm5),
	NULL,
	NULL,
	PHP_MINFO(kadm5),
#if ZEND_MODULE_API_NO >= 20010901
	"0.2.3",
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */


#ifdef COMPILE_DL_KADM5
ZEND_GET_MODULE(kadm5)
#endif


/* {{{ _close_kadm5_handle
 */
static void _close_kadm5_handle(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	void *handle = (void *)rsrc->ptr;

	kadm5_destroy(handle);
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(kadm5)
{
	REGISTER_STRING_CONSTANT("KADM5_PRINCIPAL", 
							 OP_PRINCIPAL, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PRINC_EXPIRE_TIME", 
							 OP_PRINC_EXPIRE_TIME, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PW_EXPIRATION", 
							 OP_PW_EXPIRATION, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_LAST_PW_CHANGE", 
							 OP_LAST_PW_CHANGE,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_ATTRIBUTES", 
							 OP_ATTRIBUTES, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_MAX_LIFE", 
							 OP_MAX_LIFE, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_MOD_TIME", 
							 OP_MOD_TIME, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_MOD_NAME", 
							 OP_MOD_NAME, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_KVNO", 
							 OP_KVNO, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_MKVNO", 
							 OP_MKVNO, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_AUX_ATTRIBUTES", 
							 OP_AUX_ATTRIBUTES, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_POLICY", 
							 OP_POLICY, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_CLEARPOLICY", 
							 OP_CLEARPOLICY, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_RANDKEY", 
							 OP_RANDKEY, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_MAX_RLIFE", 
							 OP_MAX_RLIFE, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_LAST_SUCCES", 
							 OP_LAST_SUCCESS, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_LAST_FAILED", 
							 OP_LAST_FAILED, 
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_FAIL_AUTH_COUNT", 
							 OP_FAIL_AUTH_COUNT,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PW_MAX_LIFE",
							 OP_PW_MAX_LIFE,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PW_MIN_LIFE",
							 OP_PW_MIN_LIFE,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PW_MIN_LENGTH",
							 OP_PW_MIN_LENGTH,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PW_MIN_CLASSES",
							 OP_PW_MIN_CLASSES,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_PW_HISTORY_NUM",
							 OP_PW_HISTORY_NUM,
							 CONST_PERSISTENT | CONST_CS);
	REGISTER_STRING_CONSTANT("KADM5_REF_COUNT",
							 OP_REF_COUNT,
							 CONST_PERSISTENT | CONST_CS);

	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_POSTDATED",
						   KRB5_KDB_DISALLOW_POSTDATED,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_FORWARDABLE",
						   KRB5_KDB_DISALLOW_FORWARDABLE,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_TGT_BASED",
						   KRB5_KDB_DISALLOW_TGT_BASED,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_RENEWABLE",
						   KRB5_KDB_DISALLOW_RENEWABLE,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_PROXIABLE",
						   KRB5_KDB_DISALLOW_PROXIABLE,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_DUP_SKEY",
						   KRB5_KDB_DISALLOW_DUP_SKEY,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_ALL_TIX",
						   KRB5_KDB_DISALLOW_ALL_TIX,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_REQUIRES_PRE_AUTH",
						   KRB5_KDB_REQUIRES_PRE_AUTH,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_REQUIRES_HW_AUTH",
						   KRB5_KDB_REQUIRES_HW_AUTH,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_REQUIRES_PWCHANGE",
						   KRB5_KDB_REQUIRES_PWCHANGE,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_DISALLOW_SVR",
						   KRB5_KDB_DISALLOW_SVR,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_PWCHANGE_SERVICE",
						   KRB5_KDB_PWCHANGE_SERVICE,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_SUPPORT_DESMD5",
						   KRB5_KDB_SUPPORT_DESMD5,
						   CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("KRB5_KDB_NEW_PRINC",
						   KRB5_KDB_NEW_PRINC,
						   CONST_PERSISTENT | CONST_CS);

	/* register destructor */
	le_handle = zend_register_list_destructors_ex(_close_kadm5_handle, NULL, HANDLE_ID, module_number);

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(kadm5)
{
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(kadm5)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "kadm5 support", "enabled");
	php_info_print_table_end();
}
/* }}} */


/* {{{ kadm5_error
 *
 * Error handling for the KADM5 library funtions.
 */
static void kadm5_error( kadm5_ret_t error_code )
{
	switch( error_code ) {
	case KADM5_FAILURE:
		php_error(E_WARNING, "Operation failed for unspecified reason. (KADM5_FAILURE)");
		break;
	case KADM5_AUTH_GET:
		php_error(E_WARNING, "Operation requires 'get' privilege. (KADM5_AUTH_GET)");
		break;
	case KADM5_AUTH_ADD:
		php_error(E_WARNING, "Operation requires 'add' privilege. (KADM5_AUTH_ADD)");
		break;
	case KADM5_AUTH_MODIFY:
		php_error(E_WARNING, "Operation requires 'modify' privilege. (KADM5_AUTH_MODIFY)");
		break;
	case KADM5_AUTH_DELETE:
		php_error(E_WARNING, "Operation requires 'delete' privilege. (KADM5_AUTH_DELETE)");
		break;
	case KADM5_AUTH_INSUFFICIENT:
		php_error(E_WARNING, "Insufficient authorization for operation. (KADM5_AUTH_INSUFFICIENT)");
		break;
	case KADM5_BAD_DB:
		php_error(E_WARNING, "Database inconsistency detected. (KADM5_BAD_DB)");
		break;
	case KADM5_DUP:
		php_error(E_WARNING, "Principal or policy already exists. (KADM5_DUP)");
		break;
	case KADM5_RPC_ERROR:
		php_error(E_WARNING, "Communication failure with server. (KADM5_RPC_ERROR)");
		break;
	case KADM5_NO_SRV:
		php_error(E_WARNING, "No administration server found for realm. (KADM5_NO_SRV)");
		break;
	case KADM5_BAD_HIST_KEY:
		php_error(E_WARNING, "Password history principal key version mismatch. (KADM5_BAD_HIST_KEY)");
		break;
	case KADM5_NOT_INIT:
		php_error(E_WARNING, "Connection to server not initialized. (KADM5_NOT_INIT)");
		break;
	case KADM5_UNK_PRINC:
		php_error(E_WARNING, "Principal does not exist. (KADM5_UNK_PRINC)");
		break;
	case KADM5_UNK_POLICY:
		php_error(E_WARNING, "Policy does not exist. (KADM5_UNK_POLICY)");
		break;
	case KADM5_BAD_MASK:
		php_error(E_WARNING, "Invalid field mask for operation. (KADM5_BAD_MASK)");
		break;
	case KADM5_BAD_CLASS:
		php_error(E_WARNING, "Invalid number of character classes. (KADM5_BAD_CLASS)");
		break;
	case KADM5_BAD_LENGTH:
		php_error(E_WARNING, "Invalid password length. (KADM5_BAD_LENGTH)");
		break;
	case KADM5_BAD_POLICY:
		php_error(E_WARNING, "Illegal policy name. (KADM5_BAD_POLICY)");
		break;
	case KADM5_BAD_PRINCIPAL:
		php_error(E_WARNING, "Illegal principal name. (KADM5_BAD_PRINCIPAL)");
		break;
	case KADM5_BAD_AUX_ATTR:
		php_error(E_WARNING, "Invalid auxiliary attributes. (KADM5_BAD_AUX_ATTR)");
		break;
	case KADM5_BAD_HISTORY:
		php_error(E_WARNING, "Invalid password history count. (KADM5_BAD_HISTORY)");
		break;
	case KADM5_BAD_MIN_PASS_LIFE:
		php_error(E_WARNING, "Password minimum life is greater then password maximum life. (KADM5_BAD_MIN_PASS_LIFE)");
		break;
	case KADM5_PASS_Q_TOOSHORT:
		php_error(E_WARNING, "Password is too short. (KADM5_PASS_Q_TOOSHORT)");
		break;
	case KADM5_PASS_Q_CLASS:
		php_error(E_WARNING, "Password does not contain enough character classes. (KADM5_PASS_Q_CLASS)");
		break;
	case KADM5_PASS_Q_DICT:
		php_error(E_WARNING, "Password is in the password dictionary. (KADM5_PASS_Q_DICT)");
		break;
	case KADM5_PASS_REUSE:
		php_error(E_WARNING, "Cannot reuse password. (KADM5_PASS_REUSE)");
		break;
	case KADM5_PASS_TOOSOON:
		php_error(E_WARNING, "Current password's minimum life has not expired. (KADM5_PASS_TOOSOON)");
		break;
	case KADM5_POLICY_REF:
		php_error(E_WARNING, "Policy is in use. (KADM5_POLICY_REF)");
		break;
	case KADM5_INIT:
		php_error(E_WARNING, "Connection to server already initialized. (KADM5_INIT)");
		break;
	case KADM5_BAD_PASSWORD:
		php_error(E_WARNING, "Incorrect password. (KADM5_BAD_PASS)");
		break;
	case KADM5_PROTECT_PRINCIPAL:
		php_error(E_WARNING, "Cannot change protected principal. (KADM5_PROTECT_PRINCIPAL)");
		break;
	case KADM5_BAD_SERVER_HANDLE:
		php_error(E_WARNING, "Internal error! Bad Admin server handle. (KADM5_BAD_SERVER_HANDLE)");
		break;
	case KADM5_BAD_STRUCT_VERSION:
		php_error(E_WARNING, "Internal error! Bad API structure version. (KADM5_BAD_STRUCT_VERSION)");
		break;
	case KADM5_OLD_STRUCT_VERSION:
		php_error(E_WARNING, "Internal error! API structure version specified by application is no longer supported. (KADM5_OLD_STRUCT_VERSION)");
		break;
	case KADM5_NEW_STRUCT_VERSION:
		php_error(E_WARNING, "Internal error! API structure version specified by application is unknown to libraries. (KADM5_NEW_STRUCT_VERSION)");
		break;
	case KADM5_BAD_API_VERSION:
		php_error(E_WARNING, "Internal error! Bad API version. (KADM5_BAD_API_VERSION)");
		break;
	case KADM5_OLD_LIB_API_VERSION:
		php_error(E_WARNING, "Internal error! API version specified by application is no longer supported by libraries. (KADM5_OLD_LIB_API_VERSION)");
		break;
	case KADM5_OLD_SERVER_API_VERSION:
		php_error(E_WARNING, "Internal error! API version specified by application is no longer supported by server. (KADM5_OLD_SERVER_API_VERSION)");
		break;
	case KADM5_NEW_LIB_API_VERSION:
		php_error(E_WARNING, "Internal error! API version specified by application is unknown to libraries. (KADM5_NEW_LIB_API_VERSION)");
		break;
	case KADM5_NEW_SERVER_API_VERSION:
		php_error(E_WARNING, "Internal error! API version specified by application is unknown to server. (KADM5_NEW_SERVER_API_VERSION)");
		break;
	case KADM5_SECURE_PRINC_MISSING:
		php_error(E_WARNING, "Database error! Required principal missing. (KADM5_SECURE_PRINC_MISSING)");
		break;
	case KADM5_NO_RENAME_SALT:
		php_error(E_WARNING, "The salt type of the specified principal does not support renaming. (KADM5_NO_RENAME_SALT)");
		break;
	case KADM5_BAD_CLIENT_PARAMS:
		php_error(E_WARNING, "Illegal configuration parameter for remote KADM5 client. (KADM5_BAD_CLIENT_PARAMS)");
		break;
	case KADM5_BAD_SERVER_PARAMS:
		php_error(E_WARNING, "Illegal configuration parameter for local KADM5 client. (KADM5_BAD_SERVER_PARAMS)");
		break;
	case KADM5_AUTH_LIST:
		php_error(E_WARNING, "Operation requires 'list' privilege. (KADM5_AUTH_LIST)");
		break;
	case KADM5_AUTH_CHANGEPW:
		php_error(E_WARNING, "Operation requires 'change-password' privilege. (KADM5_AUTH_CHANGEPW)");
		break;
	case KADM5_BAD_TL_TYPE:
		php_error(E_WARNING, "Internal error! Illegal tagged data list element type. (KADM5_BAD_TL_TYPE)");
		break;
	case KADM5_MISSING_CONF_PARAMS:
		php_error(E_WARNING, "Required parameters in kdc.conf missing. (KADM5_MISSING_CONF_PARAMS)");
		break;
	case KADM5_BAD_SERVER_NAME:
		php_error(E_WARNING, "Bad krb5 admin server hostname. (KADM5_BAD_SERVER_NAME)");
		break;
	case KADM5_AUTH_SETKEY:
		php_error(E_WARNING, "Operation requires 'set-key' privilege. (KADM5_AUTH_SETKEY)");
		break;
	case KADM5_SETKEY_DUP_ENCTYPES:
		php_error(E_WARNING, "Multiple values for single or folded enctype. (KADM5_SETKEY_DUP_ENCTYPES)");
		break;
/*	default: */
	}
}
/* }}} */


/* {{{ proto resource kadm5_init_with_password(string admin_server, string realm, string principal, string password)
   Opens a conncetion to the KADM5 library and initializes any neccessary state information. */
PHP_FUNCTION(kadm5_init_with_password)
{
	char *admin_server, *realm, *princstr, *password;
	int admin_server_len, realm_len, princstr_len, password_len;
	kadm5_config_params params;
	void *handle = NULL;
	kadm5_ret_t rc;
	char **db_args = NULL;

	memset((char *) &params, 0, sizeof(params));

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssss", 
							  &admin_server, &admin_server_len,
							  &realm, &realm_len, &princstr, &princstr_len, 
							  &password, &password_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	params.realm = realm;
	params.mask |= KADM5_CONFIG_REALM;
	params.admin_server = admin_server;
	params.mask |= KADM5_CONFIG_ADMIN_SERVER;

	rc = kadm5_init_with_password(princstr, password,
								  KADM5_ADMIN_SERVICE,
								  &params,
								  KADM5_STRUCT_VERSION,
								  KADM5_API_VERSION_2,
								  db_args,
								  &handle);

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	if (handle == NULL) {
		php_error(E_WARNING, "Internal error! handle == NULL!");
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, handle, le_handle);
}
/* }}} */


/* {{{ proto int kadm5_destroy(resource handle)
   Closes the connection to the admin server and releases all related resources. */
PHP_FUNCTION(kadm5_destroy)
{
	zval **link;
	void *handle;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &link ) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, link, -1, HANDLE_ID, le_handle);

	zend_list_delete((*link)->value.lval);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int kadm5_flush(resource handle)
   Flush all changes to the Kerberos database, leaving the connection to the Kerberos admin server open. */
PHP_FUNCTION(kadm5_flush)
{
	zval **link;
	void *handle;
	kadm5_ret_t rc;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &link ) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, link, -1, HANDLE_ID, le_handle);

	rc = kadm5_flush(handle);

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int kadm5_create_principal(resource handle, string principal [, string password [, array options]])
   Creates a kerberos principal with the given parameters. */
PHP_FUNCTION(kadm5_create_principal)
{
	int i;
	zval *link;
	zval *options;
	void *handle;
	kadm5_ret_t rc;
	kadm5_principal_ent_rec princ;
	kadm5_policy_ent_rec defpol;
	krb5_context context;
	int randkey = 0;
	char dummybuf[256];
	long mask = 0;
	char *princstr;	  /* principal to create */
	int princstr_len;
	char *password;	  /* principal's password */
	int password_len;

	options = NULL;

	/* initializing dummybuf */
	for (i=0; i<256; i++) {
		dummybuf[i] = (i+1) % 256;
	}

	/* zero all fields in principal structure */
	memset(&princ, 0, sizeof(princ));

	if (ZEND_NUM_ARGS() == 2) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", 
								  &link, &princstr, &princstr_len) == FAILURE) {
			WRONG_PARAM_COUNT;
		}

		randkey = 1; /* no password given, randkey assumed */
	} else if (ZEND_NUM_ARGS() == 3) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zss", 
								  &link,
								  &princstr, &princstr_len, 
								  &password, &password_len) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
	} else if (ZEND_NUM_ARGS() == 4) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zssa",
								  &link,
								  &princstr, &princstr_len,
								  &password, &password_len,
								  &options) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
	} else {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	/* initializing krb5 context */

	rc = krb5_init_context(&context);

	if (rc) {
		php_error(E_WARNING, "Error while initializing krb5 library");
		RETURN_FALSE;
	}

	/* converting principal string to structure */

	rc = krb5_parse_name(context, princstr, &princ.principal);

	if (rc) {
		php_error(E_WARNING, "Error parsing principal %s.", princstr);
		krb5_free_context(context);
		RETURN_FALSE;
	}

	/* parsing options */
	if (options) {
		HashTable *options_hash;
		zval *key;
		zval **data;
		HashPosition pos;
		char  *string_key;
		uint  string_key_len;
		ulong  num_key;

		/* Allocate space for key */
		MAKE_STD_ZVAL(key);

		options_hash = HASH_OF(options);
		zend_hash_internal_pointer_reset_ex(options_hash, &pos);

		/* Iterate through hash */
		while (zend_hash_get_current_data_ex(options_hash, (void**)&data, &pos) == SUCCESS) {
			/* Set up the key */
			if (zend_hash_get_current_key_ex(options_hash, &string_key, &string_key_len, &num_key, 0, &pos) == HASH_KEY_IS_LONG) {
				Z_TYPE_P(key) = IS_LONG;
				Z_LVAL_P(key) = num_key;
			} else {
				Z_TYPE_P(key) = IS_STRING;
				Z_STRVAL_P(key) = string_key;
				Z_STRLEN_P(key) = string_key_len-1;
			}

			if (Z_TYPE_P(key) == IS_LONG) {
				php_error(E_WARNING, "Key (%d) is not a string-value. Ignored.", Z_LVAL_P(key));
			} else if (! strcmp(Z_STRVAL_P(key), OP_PRINC_EXPIRE_TIME)) {
				convert_to_long(*data);
				princ.princ_expire_time = Z_LVAL_PP(data);
				mask |= KADM5_PRINC_EXPIRE_TIME;
			} else if (! strcmp(Z_STRVAL_P(key), OP_PW_EXPIRATION)) {
				convert_to_long(*data);
				princ.pw_expiration = Z_LVAL_PP(data);
				mask |= KADM5_PW_EXPIRATION;
			} else if (! strcmp(Z_STRVAL_P(key), OP_MAX_LIFE)) {
				convert_to_long(*data);
				princ.max_life = Z_LVAL_PP(data);
				mask |= KADM5_MAX_LIFE;
			} else if (! strcmp(Z_STRVAL_P(key), OP_MAX_RLIFE)) {
				convert_to_long(*data);
				princ.max_renewable_life = Z_LVAL_PP(data);
				mask |= KADM5_MAX_RLIFE;
			} else if (! strcmp(Z_STRVAL_P(key), OP_KVNO)) {
				convert_to_long(*data);
				princ.kvno = Z_LVAL_PP(data);
				mask |= KADM5_KVNO;
			} else if (! strcmp(Z_STRVAL_P(key), OP_POLICY)) {
				convert_to_string(*data);
				princ.policy = Z_STRVAL_PP(data);
				mask |= KADM5_POLICY;
			} else if (! strcmp(Z_STRVAL_P(key), OP_CLEARPOLICY)) {
				princ.policy = NULL;
				mask |= KADM5_POLICY_CLR;
			} else if (! strcmp(Z_STRVAL_P(key), OP_RANDKEY)) {
				randkey = 1;
			} else if (! strcmp(Z_STRVAL_P(key), OP_ATTRIBUTES)) {
				convert_to_long(*data);
				princ.attributes = Z_LVAL_PP(data);
				mask |= KADM5_ATTRIBUTES;
			} else {
				convert_to_string(*data);
				php_error(E_WARNING, "Option (%s=%s) not implemented. Ignored.", Z_STRVAL_P(key), Z_STRVAL_PP(data));
			}

			zend_hash_move_forward_ex(options_hash, &pos);
		}
	}

	/*
	 * If -policy was not specified, and -clearpolicy was not
	 * specified, and the policy "default" exists, assign it.  If
	 * -clearpolicy was specified, then KADM5_POLICY_CLR should be
	 * unset, since it is never valid for kadm5_create_principal.
	 */
	if ((! (mask & KADM5_POLICY)) && (! (mask & KADM5_POLICY_CLR))) {
		if (! kadm5_get_policy(handle, "default", &defpol)) {
			php_error(E_WARNING, "No policy specified for %s; assigning \"default\"", princstr);
			princ.policy = "default";
			mask |= KADM5_POLICY;
			(void) kadm5_free_policy_ent(handle, &defpol);
		} else {
			php_error(E_WARNING, "No policy specified for %s; defaulting to no policy", princstr);
		}
	}
	mask &= ~KADM5_POLICY_CLR;

	if (randkey) {
		princ.attributes |= KRB5_KDB_DISALLOW_ALL_TIX; /* set notix */
		mask |= KADM5_ATTRIBUTES;
		password = dummybuf;
	}

	/* creating principal */

	mask |= KADM5_PRINCIPAL;
	rc = kadm5_create_principal(handle, &princ, mask, password);
	if (rc) {
		kadm5_error(rc);
		krb5_free_principal(context, princ.principal);
		krb5_free_context(context);
		RETURN_FALSE;
	}

	/* setting randkey if necessary */

	if (randkey) {
		rc = kadm5_randkey_principal(handle, princ.principal, NULL, NULL);
		if (rc) {
			kadm5_error(rc);
			krb5_free_principal(context, princ.principal);
			krb5_free_context(context);
			RETURN_FALSE;
		}
		princ.attributes &= ~KRB5_KDB_DISALLOW_ALL_TIX; /* clear notix */
		mask = KADM5_ATTRIBUTES;
		rc = kadm5_modify_principal(handle, &princ, mask);
		if (rc) {
			kadm5_error(rc);
			krb5_free_principal(context, princ.principal);
			krb5_free_context(context);
			RETURN_FALSE;
		}
	}

	krb5_free_principal(context, princ.principal);
	krb5_free_context(context);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int kadm5_chpass_principal(resource handle, string principal, string password)
   Changes the principal's password. */
PHP_FUNCTION(kadm5_chpass_principal)
{
	int i;
	krb5_principal princ;
	char *principal;
	char *princstr;
	int princstr_len;
	char *password;
	int password_len;
	zval *link;
	void *handle;
	krb5_context context;
	kadm5_ret_t rc; /* return code */


	memset(&princ, 0, sizeof(princ));

	if (ZEND_NUM_ARGS() != 3) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zss", &link,
							 &princstr, &princstr_len, &password, &password_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	rc = krb5_init_context(&context);

	if (rc) {
		php_error(E_WARNING, "Error while initializing krb5 library");
		RETURN_FALSE;
	}

	rc = krb5_parse_name(context, princstr, &princ);

	if (rc) {
		php_error(E_WARNING, "Error parsing principal.");
		krb5_free_context(context);
		RETURN_FALSE;
	}

	rc = kadm5_chpass_principal(handle, princ, password);

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	krb5_free_context(context);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int kadm5_delete_principal(resource handle, string principal)
   Deletes a kerberos principal. */
PHP_FUNCTION(kadm5_delete_principal)
{
	int i;
	krb5_principal princ;
	char *principal;
	char *princstr;
	int princstr_len;
	zval *link;
	void *handle;
	krb5_context context;
	kadm5_ret_t rc; /* return code */

	memset(&princ, 0, sizeof(princ));

	if( ZEND_NUM_ARGS() != 2 ) {
		WRONG_PARAM_COUNT;
	}

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", &link, &princstr, &princstr_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	rc = krb5_init_context(&context);

	if( rc ) {
		php_error(E_WARNING, "Error while initializing krb5 library");
		RETURN_FALSE;
	}

	rc = krb5_parse_name(context, princstr, &princ);

	if( rc ) {
		php_error(E_WARNING, "Error parsing principal.");
		krb5_free_context(context);
		RETURN_FALSE;
	}

	rc = kadm5_delete_principal(handle, princ);

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	krb5_free_context(context);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int kadm5_delete_principal(resource handle, string principal, array options)
   Modifies a kerberos principal with the given parameters. */
PHP_FUNCTION(kadm5_modify_principal)
{
	int i;
	zval *link;
	zval *options;
	void *handle;
	kadm5_ret_t rc;
	kadm5_principal_ent_rec princ;
	kadm5_policy_ent_rec defpol;
	krb5_context context;
	int randkey = 0;
	char dummybuf[256];
	long mask = 0;
	char *princstr;	  /* principal to create */
	int princstr_len;

	options = NULL;

	/* initializing dummybuf */
	for (i=0; i<256; i++) {
		dummybuf[i] = (i+1) % 256;
	}

	/* zero all fields in principal structure */
	memset(&princ, 0, sizeof(princ));

	if (ZEND_NUM_ARGS() == 3) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zsa", &link,
								  &princstr, &princstr_len, &options) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
	} else {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	/* initializing krb5 context */

	rc = krb5_init_context(&context);

	if (rc) {
		php_error(E_WARNING, "Error while initializing krb5 library");
		RETURN_FALSE;
	}

	/* converting principal string to structure */

	rc = krb5_parse_name(context, princstr, &princ.principal);

	if (rc) {
		php_error(E_WARNING, "Error parsing principal %s.", princstr);
		krb5_free_context(context);
		RETURN_FALSE;
	}

	/* parsing options */
	if (options) {
		HashTable *options_hash;
		zval *key;
		zval **data;
		HashPosition pos;
		char  *string_key;
		uint  string_key_len;
		ulong  num_key;

		/* Allocate space for key */
		MAKE_STD_ZVAL(key);

		options_hash = HASH_OF(options);
		zend_hash_internal_pointer_reset_ex(options_hash, &pos);

		/* Iterate through hash */
		while (zend_hash_get_current_data_ex(options_hash, (void**)&data, &pos) == SUCCESS) {
			/* Set up the key */
			if (zend_hash_get_current_key_ex(options_hash, &string_key, &string_key_len, &num_key, 0, &pos) == HASH_KEY_IS_LONG) {
				Z_TYPE_P(key) = IS_LONG;
				Z_LVAL_P(key) = num_key;
			} else {
				Z_TYPE_P(key) = IS_STRING;
				Z_STRVAL_P(key) = string_key;
				Z_STRLEN_P(key) = string_key_len-1;
			}

			if (Z_TYPE_P(key) == IS_LONG) {
				php_error(E_WARNING, "Key (%d) is not a string-value. Ignored.", Z_LVAL_P(key));
			} else if (!strcmp(Z_STRVAL_P(key), OP_PRINC_EXPIRE_TIME)) {
				convert_to_long(*data);
				princ.princ_expire_time = Z_LVAL_PP(data);
				mask |= KADM5_PRINC_EXPIRE_TIME;
			} else if (!strcmp(Z_STRVAL_P(key), OP_PW_EXPIRATION)) {
				convert_to_long(*data);
				princ.pw_expiration = Z_LVAL_PP(data);
				mask |= KADM5_PW_EXPIRATION;
			} else if (!strcmp(Z_STRVAL_P(key), OP_MAX_LIFE)) {
				convert_to_long(*data);
				princ.max_life = Z_LVAL_PP(data);
				mask |= KADM5_MAX_LIFE;
			} else if (!strcmp(Z_STRVAL_P(key), OP_MAX_RLIFE)) {
				convert_to_long(*data);
				princ.max_renewable_life = Z_LVAL_PP(data);
				mask |= KADM5_MAX_RLIFE;
			} else if (!strcmp(Z_STRVAL_P(key), OP_KVNO)) {
				convert_to_long(*data);
				princ.kvno = Z_LVAL_PP(data);
				mask |= KADM5_KVNO;
			} else if (!strcmp(Z_STRVAL_P(key), OP_POLICY)) {
				convert_to_string(*data);
				princ.policy = Z_STRVAL_PP(data);
				mask |= KADM5_POLICY;
			} else if (!strcmp(Z_STRVAL_P(key), OP_CLEARPOLICY)) {
				princ.policy = NULL;
				mask |= KADM5_POLICY_CLR;
			} else if (!strcmp(Z_STRVAL_P(key), OP_FAIL_AUTH_COUNT)) {
				convert_to_long(*data);
				princ.fail_auth_count = Z_LVAL_PP(data);
				mask |= KADM5_FAIL_AUTH_COUNT;
			} else if (!strcmp(Z_STRVAL_P(key), OP_ATTRIBUTES)) {
				convert_to_long(*data);
				princ.attributes = Z_LVAL_PP(data);
				mask |= KADM5_ATTRIBUTES;
			} else {
				convert_to_string(*data);
				php_error(E_WARNING, "Option (%s=%s) not implemented. Ignored.", Z_STRVAL_P(key), Z_STRVAL_PP(data));
			}

			zend_hash_move_forward_ex(options_hash, &pos);
		}
	}

	/* creating principal */

	rc = kadm5_modify_principal(handle, &princ, mask);

	if (rc) {
		kadm5_error(rc);
		krb5_free_principal(context, princ.principal);
		krb5_free_context(context);
		RETURN_FALSE;
	}

	krb5_free_principal(context, princ.principal);
	krb5_free_context(context);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto array kadm5_get_principals(resource handle)
   Gets all principals from the Kerberos database. */
PHP_FUNCTION(kadm5_get_principals)
{
	int i;
	char *exp;
	int exp_len;
	char **princs;
	int count;
	kadm5_ret_t rc;

	zval *link;
	void *handle;

	if (ZEND_NUM_ARGS() != 1) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &link) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	array_init(return_value);

	rc = kadm5_get_principals(handle, NULL, &princs, &count);

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	for (i=0; i<count; i++) {
		add_next_index_string(return_value, princs[i], 1);
	}

	kadm5_free_name_list(handle, princs, count);
}
/* }}} */


/* {{{ proto array kadm5_get_principal(resource handle, string principal)
   Gets the principal's entries from the Kerberos database. */
PHP_FUNCTION(kadm5_get_principal)
{
	int i;
	krb5_principal princ;
	char *principal;
	char *princstr;
	int princstr_len;
	char *mod_name;
	int count;
	zval *link;
	void *handle;
	krb5_context context;
	kadm5_principal_ent_rec ent;
	kadm5_ret_t rc; /* return code */
	zval *keys;
	zval *attributes;

	memset(&ent, 0, sizeof(ent));
	memset(&princ, 0, sizeof(princ));

	if (ZEND_NUM_ARGS() != 2) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", &link, &princstr, &princstr_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	array_init(return_value);

	rc = krb5_init_context(&context);

	if (rc) {
		php_error(E_WARNING, "Error while initializing krb5 library");
		RETURN_FALSE;
	}

	rc = krb5_parse_name(context, princstr, &princ);

	if (rc) {
		php_error(E_WARNING, "Error parsing principal.");
		krb5_free_context(context);
		RETURN_FALSE;
	}

	rc = kadm5_get_principal(handle, princ, &ent, KADM5_PRINCIPAL_NORMAL_MASK );

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	rc = krb5_unparse_name(context, ent.principal, &principal);

	if (rc) {
		php_error(E_WARNING, "Error unparsing principal name.");
		krb5_free_context(context);
		RETURN_FALSE;
	}

	rc = krb5_unparse_name(context, ent.mod_name, &mod_name);

	if (rc) {
		php_error(E_WARNING, "Error unparsing mod_name.");
		free(principal);
		krb5_free_context(context);
		RETURN_FALSE;
	}

	add_assoc_string(return_value, OP_PRINCIPAL, principal, 1);
	add_assoc_long(return_value, OP_PRINC_EXPIRE_TIME, ent.princ_expire_time);
	add_assoc_long(return_value, OP_LAST_PW_CHANGE, ent.last_pwd_change);
	add_assoc_long(return_value, OP_PW_EXPIRATION, ent.pw_expiration);
	add_assoc_long(return_value, OP_MAX_LIFE, ent.max_life);
	add_assoc_long(return_value, OP_MAX_RLIFE, ent.max_renewable_life);
	add_assoc_string(return_value, OP_MOD_NAME, mod_name, 1);
	add_assoc_long(return_value, OP_KVNO, ent.kvno);
	add_assoc_long(return_value, OP_MOD_TIME, ent.mod_date);
	add_assoc_long(return_value, OP_LAST_SUCCESS, ent.last_success);
	add_assoc_long(return_value, OP_LAST_FAILED, ent.last_failed);
	add_assoc_long(return_value, OP_FAIL_AUTH_COUNT, ent.fail_auth_count);
	add_assoc_string(return_value, OP_POLICY, ent.policy ? ent.policy : "", 1);
	add_assoc_long(return_value, OP_ATTRIBUTES, ent.attributes);

	free(principal);
	free(mod_name);
	krb5_free_context(context);
}
/* }}} */


/* {{{ proto array kadm5_get_policies(resource handle)
   Gets all policies from the Kerberos database. */
PHP_FUNCTION(kadm5_get_policies)
{
	int i;
	char *exp;
	int exp_len;
	char **policies;
	int count;
	kadm5_ret_t rc;
	zval *link;
	void *handle;

	if (ZEND_NUM_ARGS() != 1) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &link) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(handle, void *, &link, -1, HANDLE_ID, le_handle);

	array_init(return_value);

	rc = kadm5_get_policies(handle, NULL, &policies, &count);

	if (rc) {
		kadm5_error(rc);
		RETURN_FALSE;
	}

	for (i=0; i<count; i++) {
		add_next_index_string(return_value, policies[i], 1);
	}

	kadm5_free_name_list(handle, policies, count);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
