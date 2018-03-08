/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Bo Li <cikewang@139.com>                                                     |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include <curl/curl.h>
#include <string.h>

#include "php_elasticsearch.h"

/* If you declare any globals in php_elasticsearch.h uncomment this:*/
ZEND_DECLARE_MODULE_GLOBALS(elasticsearch)

/* True global resources - no need for thread safety here */
static int le_elasticsearch;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("elasticsearch.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_elasticsearch_globals, elasticsearch_globals)
    STD_PHP_INI_ENTRY("elasticsearch.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_elasticsearch_globals, elasticsearch_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_elasticsearch_compiled(string arg)
   Return a string to confirm that the module is compiled in
PHP_FUNCTION(confirm_elasticsearch_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "elasticsearch", arg);

	RETURN_STR(strg);
}*/
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_elasticsearch_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_elasticsearch_init_globals(zend_elasticsearch_globals *elasticsearch_globals)
{
	elasticsearch_globals->global_value = 0;
	elasticsearch_globals->global_string = NULL;
}
*/
/* }}} */


struct ResponseStruct {
	char *body;
	uint size;
} response_info;

/** {{{ static size_t WriteResponseCallback(void *contents, size_t size, size_t nmemb, void *userp) */
static size_t WriteResponseCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct ResponseStruct *mem = (struct ResponseStruct *)userp;

	mem->body = realloc(mem->body, mem->size + realsize + 1);
	if(mem->body == NULL) {
		/* out of memory! */
		printf("not enough response (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->body[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->body[mem->size] = 0;

	return realsize;
}
/* }}} */

/** {{{ static void curl_es(char *methed, char *url, char *data) */
static void curl_es(char *methed, char *url, char *data)
{
	CURL *curl;
	CURLcode res;

	response_info.body = malloc(1);  /* will be grown as needed by the realloc above */
	response_info.size = 0;    /* no data at this point */

	curl = curl_easy_init();
	if(curl) {
		/* set url */
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */
		//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		//  curl_easy_setopt(curl, CURLOPT_HTTPGET, strlen(data));
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, methed);

		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResponseCallback);

		/* we pass our 'reponse_info' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_info);

		/* send data */
		if (strlen(data) > 4) {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		}

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK) {
			php_error_docref(NULL, E_WARNING, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		/* always cleanup */
		curl_easy_cleanup(curl);
	} else {
		php_error_docref(NULL, E_WARNING, "curl_easy_init() failed: %s\n", curl_easy_strerror(res));
	}
}
/* }}} */

zend_class_entry *es_class;

/* {{{ proto public Elasticsearch::setEsConfig($host, $port) */
PHP_METHOD(Elasticsearch, setEsConfig)
{
	zval *host, *url;
	uint port, url_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|l", &host, &port) == FAILURE) {
		return;
	}

	zend_update_property(es_class, getThis(), ZEND_STRL("_host"), host);
	zend_update_property_long(es_class, getThis(), ZEND_STRL("_port"), port);

	url_len = spprintf(&url, 0 , "%s:%d", Z_STRVAL(*host), port);

	ELASTICSEARCH_G(global_host) = Z_STRVAL(*host);
	ELASTICSEARCH_G(global_port) = port;
	ELASTICSEARCH_G(global_url) = url;

}
/* }}} */

/* {{{ proto public Elasticsearch::getEsConfig() */
PHP_METHOD(Elasticsearch, getEsConfig)
{
	zval *host, *port;
	char *url;

	host = zend_read_property(es_class, getThis(), ZEND_STRL("_host"), 1 TSRMLS_CC);
	port = zend_read_property(es_class, getThis(), ZEND_STRL("_port"), 1 TSRMLS_CC);

	spprintf(&url, 0 , "%s:%d", Z_STRVAL(*host), Z_LVAL(*port));
	RETURN_STRING(url, 0);
}
/* }}} */

/* {{{ proto public Elasticsearch::getHost() */
PHP_METHOD(Elasticsearch, getHost)
{
//	zval *self = getThis();
//	zval *name;
//	zval *rv;
//	name = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_host"), 0, rv);
//	RETURN_STR(name->value.str);
}
/* }}} */

/* {{{ proto public Elasticsearch::index($index_type, $data, $id) */
PHP_METHOD(Elasticsearch, index)
{
	char *index_type, *data, *url, *id = NULL;
	uint index_type_len, data_len, id_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss|s", &index_type, &index_type_len, &data, &data_len, &id, &id_len) == FAILURE) {
		return;
	}

	if (!id) {
		spprintf(&url, 0 , "%s/%s", ELASTICSEARCH_G(global_url), index_type);
		curl_es("POST", url, data);
	} else {
		spprintf(&url, 0 , "%s/%s/%s", ELASTICSEARCH_G(global_url), index_type, id);
		curl_es("PUT", url, data);
	}

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::bulk($index_type, $data) */
PHP_METHOD(Elasticsearch, bulk)
{
	char *url, *index_type, *data;
	uint url_len, index_type_len, data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &index_type, &index_type_len, &data, &data_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/_bulk",ELASTICSEARCH_G(global_url), index_type);
	curl_es("POST", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::get($index_type, $id) */
PHP_METHOD(Elasticsearch, get)
{
	char *url, *index_type, *id, *data = "NULL";
	uint index_type_len, id_len;


	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &index_type, &index_type_len, &id, &id_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/%s",ELASTICSEARCH_G(global_url), index_type, id);
	curl_es("GET", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::mget($index_type, $data) */
PHP_METHOD(Elasticsearch, mget)
{
	char *url, *index_type, *data;
	uint index_type_len, data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &index_type, &index_type_len, &data, &data_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/_mget",ELASTICSEARCH_G(global_url), index_type);
	curl_es("GET", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::search($index_type, $data) */
PHP_METHOD(Elasticsearch, search)
{
	char *url, *index_type, *data;
	uint index_type_len, data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &index_type, &index_type_len, &data, &data_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/_search",ELASTICSEARCH_G(global_url), index_type);
	curl_es("GET", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::update($index_type, $id, $data) */
PHP_METHOD(Elasticsearch, update)
{
	char *url, *index_type, *id, *data;
	uint index_type_len, id_len, data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss", &index_type, &index_type_len, &id, &id_len, &data, &data_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/_update",ELASTICSEARCH_G(global_url), index_type, id);
	curl_es("POST", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::delete($index_type, $id) */
PHP_METHOD(Elasticsearch, delete)
{
	char *url, *index_type, *id, *data = "NULL";
	uint index_type_len, id_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &index_type, &index_type_len, &id, &id_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/%s",ELASTICSEARCH_G(global_url), index_type, id);
	curl_es("DELETE", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

/* {{{ proto public Elasticsearch::count($index_type, $data) */
PHP_METHOD(Elasticsearch, count)
{
	char *url, *index_type, *data;
	uint index_type_len, data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &index_type, &index_type_len, &data, &data_len) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	spprintf(&url, 0, "%s/%s/_count",ELASTICSEARCH_G(global_url), index_type);
	curl_es("GET", url, data);

	RETURN_STRING(response_info.body, 1);
}
/* }}} */

zend_function_entry es_methods[] = {
		PHP_ME(Elasticsearch, setEsConfig, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, getEsConfig, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, getHost, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, index, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, bulk, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, get, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, mget, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, search, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, update, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, delete, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(Elasticsearch, count, NULL, ZEND_ACC_PUBLIC)
		PHP_FE_END
};

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(elasticsearch)
{


	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/

	// Assign a zend_class_entry, which is used only when registered, so it is allocated on the stack.
	zend_class_entry es;

	// Initialize zend_class_entry.
	INIT_CLASS_ENTRY(es, "Elasticsearch", es_methods);

	// registered
	es_class = zend_register_internal_class(&es);

	// Declare member properties
	zend_declare_property_string(es_class, "_host", sizeof("_host") - 1, "http://127.0.0.1", ZEND_ACC_PRIVATE);
	zend_declare_property_long(es_class, "_port", sizeof("_port") - 1, 9200, ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */




/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(elasticsearch)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(elasticsearch)
{
#if defined(COMPILE_DL_ELASTICSEARCH) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(elasticsearch)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(elasticsearch)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "elasticsearch support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ elasticsearch_functions[]
 *
 * Every user visible function must have an entry in elasticsearch_functions[].
 */
const zend_function_entry elasticsearch_functions[] = {
	//PHP_FE(confirm_elasticsearch_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in elasticsearch_functions[] */
};
/* }}} */

/* {{{ elasticsearch_module_entry
 */
zend_module_entry elasticsearch_module_entry = {
	STANDARD_MODULE_HEADER,
	"elasticsearch",
	elasticsearch_functions,
	PHP_MINIT(elasticsearch),
	PHP_MSHUTDOWN(elasticsearch),
	PHP_RINIT(elasticsearch),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(elasticsearch),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(elasticsearch),
	PHP_ELASTICSEARCH_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_ELASTICSEARCH
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(elasticsearch)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
