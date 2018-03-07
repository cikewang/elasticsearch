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
   Return a string to confirm that the module is compiled in */
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
}
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
	size_t size;
} response_info;

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

//const char *method, const char *url, const char *data
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

zend_class_entry *es_class;

/**
 * set elasticsearch configuration info
 */
ZEND_METHOD(ElasticSearch, setEsConfig)
{
	zval *host;
	zend_long *port;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|l", &host, &port) == FAILURE) {
		return;
	}

	zend_update_property(es_class, getThis(), ZEND_STRL("_host"), host);
	zend_update_property_long(es_class, getThis(), ZEND_STRL("_port"), port);

	zend_string *_host = zval_get_string(host);
	ELASTICSEARCH_G(global_host) = ZSTR_VAL(_host);
	ELASTICSEARCH_G(global_port) = port;

	zend_string *url;
	url = strpprintf(0, "%s:%d", ZSTR_VAL(_host), port);
	ELASTICSEARCH_G(global_url) = ZSTR_VAL(url);
}

/**
 * get elasticsearch configuration info
 */
ZEND_METHOD(ElasticSearch, getEsConfig)
{
	zval *host, *port, *rh, *rp;
	zend_string *host_string, *url;
	zend_long *port_long;

	host = zend_read_property(es_class, getThis(), ZEND_STRL("_host"), 0, rh);
	port = zend_read_property(es_class, getThis(), ZEND_STRL("_port"), 0, rp);

	/* 将 zval 转成 zend_string 类型  */
	host_string = zval_get_string(host);
	/* 将 zval 转成 zend_long 类型  */
	port_long = zval_get_long(port);

	url = strpprintf(0, "%s:%d", ZSTR_VAL(host_string), port_long);
	RETVAL_STR(url);
}

/**
 * get host info
 */
ZEND_METHOD(ElasticSearch, getHost)
{
	zval *self = getThis();
	zval *name;
	zval *rv;
	name = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_host"), 0, rv);
	RETURN_STR(name->value.str);
}

/**
 * ES index method
 */
ZEND_METHOD(ElasticSearch, index)
{
	zend_string *url_string, *index_type, *data, *id = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS|S", &index_type, &data, &id) == FAILURE) {
		return;
	}

	if (!id) {
		url_string =  strpprintf(0, "%s/%s",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type));
		curl_es("POST", ZSTR_VAL(url_string), ZSTR_VAL(data));
	} else {
		url_string =  strpprintf(0, "%s/%s/%s",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type), ZSTR_VAL(id));
		curl_es("PUT", ZSTR_VAL(url_string), ZSTR_VAL(data));
	}

	printf("%s \r\n", ZSTR_VAL(url_string));

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}

/**
 * ES get method
 */
ZEND_METHOD(ElasticSearch, get)
{
	zend_string *url_string, *index_type, *id;
	char *data = "NULL";

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &index_type, &id) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/%s",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type), ZSTR_VAL(id));

	curl_es("GET", ZSTR_VAL(url_string), data);

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}

/**
 * ES search method
 */
ZEND_METHOD(ElasticSearch, search)
{
	zend_string *url_string, *index_type, *data;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &index_type, &data) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/_search",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type));

	curl_es("GET", ZSTR_VAL(url_string), ZSTR_VAL(data));

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}

/**
 * ES update method
 */
ZEND_METHOD(ElasticSearch, update)
{
	zend_string *url_string, *index_type, *id, *data;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SSS", &index_type, &id, &data) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/%s/_update",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type), ZSTR_VAL(id));

	curl_es("POST", ZSTR_VAL(url_string), ZSTR_VAL(data));

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}

/**
 * ES delete method
 */
ZEND_METHOD(ElasticSearch, delete)
{
	zend_string *url_string, *index_type, *id;
	char *data = "NULL";

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &index_type, &id) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/%s",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type), ZSTR_VAL(id));

	curl_es("DELETE", ZSTR_VAL(url_string), data);

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}


zend_function_entry es_methods[] = {
		PHP_ME(ElasticSearch, setEsConfig, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, getEsConfig, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, getHost, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, index, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, get, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, search, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, update, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(ElasticSearch, delete, NULL, ZEND_ACC_PUBLIC)
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
	INIT_CLASS_ENTRY(es, "ElasticSearch", es_methods);

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
	PHP_FE(confirm_elasticsearch_compiled,	NULL)		/* For testing, remove later. */
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
