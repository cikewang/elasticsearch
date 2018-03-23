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

#include <string.h>
#include <curl/curl.h>
#include "ext/standard/info.h"
#include "ext/pcre/php_pcre.h"
#include "ext/standard/php_string.h"
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
	zval *host;
	zend_long port;

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
/* }}} */

/* {{{ proto public Elasticsearch::getEsConfig() */
PHP_METHOD(Elasticsearch, getEsConfig)
{
	zval *host, *port, *rh, *rp;
	zend_string *host_string, *url;
	zend_long *port_long;

	host = zend_read_property(es_class, getThis(), ZEND_STRL("_host"), 0, rh);
	port = zend_read_property(es_class, getThis(), ZEND_STRL("_port"), 0, rp);

	host_string = zval_get_string(host);
	port_long = zval_get_long(port);

	url = strpprintf(0, "%s:%d", ZSTR_VAL(host_string), port_long);
	RETVAL_STR(url);
}
/* }}} */

/* {{{ proto public Elasticsearch::getHost() */
PHP_METHOD(Elasticsearch, getHost)
{
	zval *self = getThis();
	zval *name;
	zval *rv;
	name = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("_host"), 0, rv);
	RETURN_STR(name->value.str);
}
/* }}} */

/* {{{ proto public Elasticsearch::index($index_type, $data, $id) */
PHP_METHOD(Elasticsearch, index)
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

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}
/* }}} */

/* {{{ proto public Elasticsearch::bulk($index_type, $data) */
PHP_METHOD(Elasticsearch, bulk)
{
	zend_string *url_string, *index_type, *data;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &index_type, &data) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/_bulk",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type));

	curl_es("POST", ZSTR_VAL(url_string), ZSTR_VAL(data));

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}
/* }}} */

/* {{{ proto public Elasticsearch::get($index_type, $id) */
PHP_METHOD(Elasticsearch, get)
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
/* }}} */

/* {{{ proto public Elasticsearch::mget($index_type, $data) */
PHP_METHOD(Elasticsearch, mget)
{
	zend_string *url_string, *index_type, *data;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &index_type, &data) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/_mget", ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type));

	curl_es("GET", ZSTR_VAL(url_string), ZSTR_VAL(data));

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}
/* }}} */

/* {{{ proto public Elasticsearch::search($index_type, $data) */
PHP_METHOD(Elasticsearch, search)
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
/* }}} */

/* {{{ proto public Elasticsearch::update($index_type, $id, $data) */
PHP_METHOD(Elasticsearch, update)
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
/* }}} */

/* {{{ proto public Elasticsearch::delete($index_type, $id) */
PHP_METHOD(Elasticsearch, delete)
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
/* }}} */

/* {{{ proto public Elasticsearch::count($index_type, $data) */
PHP_METHOD(Elasticsearch, count)
{
	zend_string *url_string, *index_type, *data;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &index_type, &data) == FAILURE) {
		return;
	}

	/* Join the URL and send the curl request. */
	url_string =  strpprintf(0, "%s/%s/_count",ELASTICSEARCH_G(global_url), ZSTR_VAL(index_type));

	curl_es("GET", ZSTR_VAL(url_string), ZSTR_VAL(data));

	zend_string *strg;
	strg = strpprintf(0, "%s", response_info.body);
	RETURN_STR(strg);
}
/* }}} */

/* {{{ proto public Elasticsearch::count($index_type, $data) */
PHP_METHOD(Elasticsearch, setQueryConvertESFormat)
{
	zend_string *where, *order_by = NULL, *group_by = NULL, *limit = NULL;
	zend_long *return_format;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|SSS", &where, &order_by, &group_by, &limit) == FAILURE) {
		RETURN_FALSE;
	}

	// 转小写变量
	zend_string *lower_where;

	// 字符串分割为数组 变量
	zend_string *delimiter_str;
	zend_long explode_limit = ZEND_LONG_MAX;
	char *delimiter = "and";

	// FOREACH 变量
	zend_string *string_key;
	zend_long num_key;
	zval *entry;

	// 转成小写
	lower_where = php_string_tolower(where);
	// 初始化 delimiter_str
	delimiter_str = zend_string_init(delimiter, strlen(delimiter), 0);
	// 字符串截断为数组
	array_init(return_value);
	php_explode(delimiter_str, lower_where, return_value, explode_limit);

	// 正则替换 变量定义
	zval replace_str_zval;
	char *repl_regex = "/\\s+/";
	zend_string *repl_regex_str = zend_string_init(repl_regex, strlen(repl_regex), 0);
	char *space = " ";
	zend_string *space_str = zend_string_init(space, strlen(space), 0);
	ZVAL_STR(&replace_str_zval, space_str);
	int replace_limit = -1;
	int replace_count = 0;

	// 正则匹配  变量定义
	zval subpats;
	zval match_return_value;
	char *match_regex = "/(\\S*)\\s?([in|not in|>|>=|<|<=|!=]+)\\s?\\(?([\\S\\s]*)\\)?/";
	zend_string		 *match_regex_str = zend_string_init(match_regex, strlen(match_regex), 0);			/* Regular expression */
	zend_string		 *rep_value;		/* String to match against */
	pcre_cache_entry *pce;				/* Compiled regular expression */
	zend_long		 flags = 0;		/* Match control flags */
	zend_long		 start_offset = 0;	/* Where the new search starts */

	// 数组
	zend_string *key_str, *condition_str, *value_str;
	zval *key_zval, *condition_zval, *value_zval;
	char *key_char, *condition, *value_char;
	char *equal_symbol = "=", *not_equal_symbol = "!=";
	char *in_symbol = "in", *not_in_symbol = "not in";
	char *great_than_symbol = ">", *great_than_or_equal_symbol = ">=";
	char *less_than_symbol = "<", *less_than_or_equal_symbol = "<=";
	char *great_than = "gt", *great_than_or_equal = "gte";
	char *less_than = "lt", *less_than_or_equal = "lte";

	// 初始化
	zval es_arr, bool_arr, must_arr, must_not_arr, bool_where;
	array_init(&es_arr);
	array_init(&bool_where);
	array_init(&must_arr);
	array_init(&must_not_arr);
	array_init(&bool_arr);

	char *match_char_key = "match";
	char *must_arr_key = "must";
	char *must_not_arr_key = "must_not";
	char *bool_char_key = "bool";
	char *query_char_key = "query";
	char *range_char_key = "range";
	char *should_char_key = "should";

	zval not_equal_arr;
	zval great_than_arr, great_than_or_equal_arr;
	zval less_than_arr, less_than_or_equal_arr;

	array_init(&not_equal_arr);
	array_init(&great_than_arr);
	array_init(&great_than_or_equal_arr);
	array_init(&less_than_arr);
	array_init(&less_than_or_equal_arr);

	// in FOREACH
	char *comma = ",";
	zend_string *comma_str = zend_string_init(comma, strlen(comma), 0);

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(return_value), num_key, string_key, entry) {
		zval fields_arr, tmp_arr, range_arr;
		array_init(&fields_arr);
		array_init(&tmp_arr);
		array_init(&range_arr);

		// 删除字符串两边空字符
		zend_string *subject_str = php_trim(zval_get_string(entry), NULL, 0, 3);
		// 正则替换
		rep_value = php_pcre_replace(repl_regex_str, subject_str, ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), &replace_str_zval, 0, replace_limit, &replace_count);
		// 删除右括号
		zend_string *match_subject_str = php_trim(rep_value, ")", 1, 3);

		// 正则匹配
		if ((pce = pcre_get_compiled_regex_cache(match_regex_str)) == NULL) {
			RETURN_FALSE;
		}

		php_pcre_match_impl(pce, ZSTR_VAL(match_subject_str), ZSTR_LEN(match_subject_str), &match_return_value, &subpats, 0, 0, 0, 0);

		// 从数组中获取值
		key_str = php_trim(zval_get_string(zend_hash_index_find(Z_ARRVAL(subpats), 1)), NULL, 0, 3);
		key_char = ZSTR_VAL(key_str);
		condition_str = php_trim(zval_get_string(zend_hash_index_find(Z_ARRVAL(subpats), 2)), NULL, 0, 3);
		condition = ZSTR_VAL(condition_str);
		value_zval = zend_hash_index_find(Z_ARRVAL(subpats), 3);
		value_str = php_trim(zval_get_string(value_zval), NULL, 0, 3);
		value_char = ZSTR_VAL(value_str);

		// 获取数组指定 index
		if (strcmp(condition, equal_symbol) == 0) {
			zend_hash_update(Z_ARRVAL(fields_arr), key_str, value_zval);
			zend_hash_str_update(Z_ARRVAL(tmp_arr), match_char_key, strlen(match_char_key), &fields_arr);
			zend_hash_next_index_insert(Z_ARRVAL(bool_where), &tmp_arr);

		} else if (strcmp(condition, not_equal_symbol) == 0) {
			zend_hash_update(Z_ARRVAL(fields_arr), key_str, value_zval);
			zend_hash_str_update(Z_ARRVAL(tmp_arr), match_char_key, strlen(match_char_key), &fields_arr);
			zend_hash_next_index_insert(Z_ARRVAL(not_equal_arr), &tmp_arr);

		} else if (strcmp(condition, in_symbol) == 0) {
			zval in_explode_arr, in_arr, should_arr, in_bool_arr;
			array_init(&in_explode_arr);
			array_init(&in_arr);
			array_init(&should_arr);
			array_init(&in_bool_arr);

			zend_string *in_string_key;
			zend_long in_num_key;
			zval *in_entry;

			php_explode(comma_str, zval_get_string(value_zval), &in_explode_arr, explode_limit);

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(in_explode_arr), in_num_key, in_string_key, in_entry) {
				zval in_fields_arr, in_tmp_arr;
				array_init(&in_fields_arr);
				array_init(&in_tmp_arr);

				// 删除空格
				zval in_value_zval;
				zend_string *in_value_empty_str = php_trim(zval_get_string(in_entry), NULL, 0, 3);
				zend_string *in_value_single_quotes_str = php_trim(in_value_empty_str, "'", 1, 3);
				zend_string *in_value_str = php_trim(in_value_single_quotes_str, "\"", 1, 3);
				ZVAL_STR(&in_value_zval, in_value_str);

				zend_hash_update(Z_ARRVAL(in_fields_arr), key_str, &in_value_zval);
				zend_hash_str_update(Z_ARRVAL(in_tmp_arr), match_char_key, strlen(match_char_key), &in_fields_arr);
				zend_hash_next_index_insert(Z_ARRVAL(in_arr), &in_tmp_arr);
			}ZEND_HASH_FOREACH_END();

			// should arr
			if (0 < zend_hash_num_elements(Z_ARRVAL(in_arr))) {
				zend_hash_str_update(Z_ARRVAL(should_arr), should_char_key, strlen(should_char_key), &in_arr);
			}

			// bool arr
			if (0 < zend_hash_num_elements(Z_ARRVAL(should_arr))) {
				zend_hash_str_update(Z_ARRVAL(in_bool_arr), bool_char_key, strlen(bool_char_key), &should_arr);
				zend_hash_next_index_insert(Z_ARRVAL(bool_where), &in_bool_arr);
			}

		} else if (strcmp(condition, not_in_symbol) == 0) {
			printf("not_in_symbol \r\n");

		} else if (strcmp(condition, great_than_symbol) == 0) {
			zend_hash_str_add(Z_ARRVAL(great_than_arr), great_than, strlen(great_than), value_zval);
			zend_hash_update(Z_ARRVAL(tmp_arr), key_str, &great_than_arr);
			zend_hash_str_update(Z_ARRVAL(range_arr), range_char_key, strlen(range_char_key), &tmp_arr);
			zend_hash_next_index_insert(Z_ARRVAL(bool_where), &range_arr);

		} else if (strcmp(condition, great_than_or_equal_symbol) == 0) {
			zend_hash_str_update(Z_ARRVAL(great_than_or_equal_arr), great_than_or_equal, strlen(great_than_or_equal), value_zval);
			zend_hash_update(Z_ARRVAL(tmp_arr), key_str, &great_than_or_equal_arr);
			zend_hash_str_update(Z_ARRVAL(range_arr), range_char_key, strlen(range_char_key), &tmp_arr);
			zend_hash_next_index_insert(Z_ARRVAL(bool_where), &range_arr);

		} else if (strcmp(condition, less_than_symbol) == 0) {
			zend_hash_str_update(Z_ARRVAL(less_than_arr), less_than, strlen(less_than), value_zval);
			zend_hash_update(Z_ARRVAL(tmp_arr), key_str, &less_than_arr);
			zend_hash_str_update(Z_ARRVAL(range_arr), range_char_key, strlen(range_char_key), &tmp_arr);
			zend_hash_next_index_insert(Z_ARRVAL(bool_where), &range_arr);

		} else if (strcmp(condition, less_than_or_equal_symbol) == 0) {
			zend_hash_str_update(Z_ARRVAL(less_than_or_equal_arr), less_than_or_equal, strlen(less_than_or_equal), value_zval);
			zend_hash_update(Z_ARRVAL(tmp_arr), key_str, &less_than_or_equal_arr);
			zend_hash_str_update(Z_ARRVAL(range_arr), range_char_key, strlen(range_char_key), &tmp_arr);
			zend_hash_next_index_insert(Z_ARRVAL(bool_where), &range_arr);
		}

	}ZEND_HASH_FOREACH_END();

	if (0 < zend_hash_num_elements(Z_ARRVAL(bool_where))) {
		zend_hash_str_update(Z_ARRVAL(must_arr), must_arr_key, strlen(must_arr_key), &bool_where);
	}

	if (0 < zend_hash_num_elements(Z_ARRVAL(not_equal_arr))) {
		zend_hash_str_update(Z_ARRVAL(must_not_arr), must_not_arr_key, strlen(must_not_arr_key), &not_equal_arr);
	}

	if (0 < zend_hash_num_elements(Z_ARRVAL(must_arr)) || 0 < zend_hash_num_elements(Z_ARRVAL(must_not_arr))) {
		zend_hash_merge(Z_ARRVAL(must_arr), Z_ARRVAL(must_not_arr), NULL, 0);
	}

	if (0 < zend_hash_num_elements(Z_ARRVAL(must_arr))) {
		zend_hash_str_update_ind(Z_ARRVAL(bool_arr), bool_char_key, strlen(bool_char_key), &must_arr);
	}

	if (0 < zend_hash_num_elements(Z_ARRVAL(bool_arr))) {
		zend_hash_str_update(Z_ARRVAL(es_arr), query_char_key, strlen(query_char_key), &bool_arr);
	}

	// order by
	if (order_by) {
		zend_string *subject_str = php_trim(order_by, NULL, 0, 3);
		rep_value = php_pcre_replace(repl_regex_str, subject_str, ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), &replace_str_zval, 0, replace_limit, &replace_count);

		char *sort_char_key = "sort";
		zend_string *order_by_string_key;
		zend_long order_by_num_key;
		zval *order_by_entry;
		zval order_by_arr, sort_arr;
		array_init(&order_by_arr);
		array_init(&sort_arr);

		char *order_fields_char = "order";
		php_explode(comma_str, rep_value, &order_by_arr, explode_limit);

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(order_by_arr), order_by_num_key, order_by_string_key, order_by_entry) {
			zval order_by_info_arr, order_fields_arr, fields_arr;
			array_init(&order_by_info_arr);
			array_init(&order_fields_arr);
			array_init(&fields_arr);

			zend_string *order_by_str = php_trim(zval_get_string(order_by_entry), NULL, 0, 3);
			php_explode(space_str, order_by_str, &order_by_info_arr, explode_limit);

			key_str = php_trim(zval_get_string(zend_hash_index_find(Z_ARRVAL(order_by_info_arr), 0)), NULL, 0, 3);
			value_zval = zend_hash_index_find(Z_ARRVAL(order_by_info_arr), 1);

			zend_hash_str_update(Z_ARRVAL(order_fields_arr), order_fields_char, strlen(order_fields_char), value_zval);
			zend_hash_update(Z_ARRVAL(fields_arr), key_str, &order_fields_arr);
			zend_hash_next_index_insert(Z_ARRVAL(sort_arr), &fields_arr);

		} ZEND_HASH_FOREACH_END();

		if (0 < zend_hash_num_elements(Z_ARRVAL(sort_arr)) ) {
			zend_hash_str_update(Z_ARRVAL(es_arr), sort_char_key, strlen(sort_char_key), &sort_arr);
		}
	}


	// limit
	if (limit) {
		char *from_char = "from";
		char *size_char = "size";
		zval limit_arr;
		array_init(&limit_arr);
		zval *from_zval, *size_zval;

		zend_string *subject_str = php_trim(limit, NULL, 0, 3);
		php_explode(comma_str, subject_str, &limit_arr, explode_limit);

		from_zval = zend_hash_index_find(Z_ARRVAL(limit_arr), 0);
		size_zval = zend_hash_index_find(Z_ARRVAL(limit_arr), 1);

		zend_hash_str_update(Z_ARRVAL(es_arr), from_char, strlen(from_char), from_zval);
		zend_hash_str_update(Z_ARRVAL(es_arr), size_char, strlen(size_char), size_zval);
	}

	RETURN_ARR(Z_ARRVAL(es_arr));

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
		PHP_ME(Elasticsearch, setQueryConvertESFormat, NULL, ZEND_ACC_PUBLIC)
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
