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
$info = $es->index("your_index/your_type", $data);


// Specify the _id
$id = "AVyqzrcLD0y03jdznsAG";
$data = "{\"user_id\": 1,\"username\": "LiBo"}";
$info = $es->index("your_index/your_type", $data, $id);
~~~

### bulk method
-----
_**Description**_: bulk.

##### *Example*

~~~
$data = ... ;  // ES JSON FORMAT
$info = $es->bulk("your_index/your_type", $data);
~~~

### get method
-----
_**Description**_: get.

##### *Example*

~~~
$id = "AVyqzrcLD0y03jdznsAG";
$info = $es->get("your_index/your_type", $id);
~~~


### mget method
-----
_**Description**_: mget.

##### *Example*

~~~
$data = ... ;  // ES JSON FORMAT
$info = $es->mget("your_index/your_type", $data);
~~~


### search method
-----
_**Description**_: search.

##### *Example*

~~~
$data = "{\"query\": {\"match_all\": {}}, \"from\":0, \"size\":1}";
$info = $es->search("your_index/your_type", $data);
~~~

### update method
-----
_**Description**_: update.

##### *Example*

~~~
$update_data = "{\"doc\" : {\"custom_id\":60}}";
$info = $es->update("your_index/your_type", $id, $update_data);
~~~

### delete method
-----
_**Description**_: delete.

##### *Example*

~~~
$id = "AVyqzrcLD0y03jdznsAG";
$info = $es->delete("your_index/your_type", $id);
~~~

### count method
-----
_**Description**_: count.

##### *Example*

~~~
$data = ... ;  // ES JSON FORMAT
$info = $es->count("your_index/your_type", $data);
~~~

### setQueryConvertESFormat method
-----
_**Description**_: setQueryConvertESFormat.

##### *Example*

~~~
where = "user_id in (164, 800010264)";  # support : =、 !=、 >、 >=、 <、 <=、 in; 
$order_by = "user_id DESC"; OR $order_by = "user_id DESC, user_name ASC";
$group_by = "";  						#Temporary dissupport
$limit = "0, 10"; 
$es_format_arr = $es->setQueryConvertESFormat($where, $order_by, $group_by, $limit);

print_r($es_format_arr);
Array
(
    [query] => Array
        (
            [bool] => Array
                (
                    [must] => Array
                        (
                            [0] => Array
                                (
                                    [bool] => Array
                                        (
                                            [should] => Array
                                                (
                                                    [0] => Array
                                                        (
                                                            [match] => Array
                                                                (
                                                                    [user_id] => 164
                                                                )

                                                        )

                                                    [1] => Array
                                                        (
                                                            [match] => Array
                                                                (
                                                                    [user_id] => 800010264
                                                                )

                                                        )

                                                )

                                        )

                                )

                        )

                )

        )

    [sort] => Array
        (
            [0] => Array
                (
                    [user_id] => Array
                        (
                            [order] => DESC
                        )

                )

            [1] => Array
                (
                    [aa] => Array
                        (
                            [order] => desc
                        )

                )

        )

    [from] => 1
    [size] => 2
)

~~~