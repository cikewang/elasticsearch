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

### setEsConfig method
-----
_**Description**_: Set ElasticSearch host and port.

##### *Example*

~~~
$es->setEsConfig("http://172.16.16.221", 9200);
~~~



### index method
-----
_**Description**_: index.

##### *Example*

~~~
// Automatic generation _id
$data = "{\"user_id\": 1,\"username\": "LiBo"}";
$info = $es->index("user_index/user", $data);


// Specify the _id
$id = "AVyqzrcLD0y03jdznsAG";
$data = "{\"user_id\": 1,\"username\": "LiBo"}";
$info = $es->index("user_index/user", $data, $id);
~~~


### get method
-----
_**Description**_: get.

##### *Example*

~~~
$id = "AVyqzrcLD0y03jdznsAG";
$info = $es->get("user_index/user", $id);
~~~

### search method
-----
_**Description**_: search.

##### *Example*

~~~
$data = "{\"query\": {\"match_all\": {}}, \"from\":0, \"size\":1}";
$info = $es->search("user_index/user", $data);
~~~

### update method
-----
_**Description**_: update.

##### *Example*

~~~
$update_data = "{\"doc\" : {\"custom_id\":60}}";
$info = $es->update("user_index/user", $id, $update_data);
~~~

### delete method
-----
_**Description**_: delete.

##### *Example*

~~~
$id = "AVyqzrcLD0y03jdznsAG";
$info = $es->delete("user_index/user", $id);
~~~

