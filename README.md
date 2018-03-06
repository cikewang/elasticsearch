# elasticsearch
Elasticsearch PHP extensions


## Requirement
- PHP 7.1 +


## Install
### Compile elasticsearch in Linux
```
$/path/to/phpize
$./configure --with-php-config=/path/to/php-config
$make && make install
```

## Class and methods
### Class Elasticsearch
-----
_**Description**_: Creates a ElasticSearch client

##### *Example*

~~~
$es = new ElasticSearch();
~~~

### setEsConfig Methods
-----
_**Description**_: Set ElasticSearch host and port.

##### *Example*

~~~
$es->setEsConfig("http://172.16.16.221", 9200);
~~~


### search Methods
-----
_**Description**_: search.

##### *Example*

~~~
$data = "{\"query\": {\"match_all\": {}}, \"from\":0, \"size\":1}";
$info = $es->search("user_index/user", $data);
~~~

### update Methods
-----
_**Description**_: update.

##### *Example*

~~~
$update_data = "{\"doc\" : {\"custom_id\":60}}";
//$info = $es->update("user_index/user", $id, $update_data);
~~~

### update Methods
-----
_**Description**_: update.

##### *Example*

~~~
$update_data = "{\"doc\" : {\"custom_id\":60}}";
//$info = $es->update("user_index/user", $id, $update_data);
~~~

