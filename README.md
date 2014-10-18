Overview
===============
**c-block-mysql** is a c-block wrapper of libmysql.

Dependences
==================
- [c-block](https://github.com/huxingyi/c-block)  
- [libuv](https://github.com/joyent/libuv)  
- [libmysql](http://dev.mysql.com/downloads/connector/c/)  

Link libmysql in MinGW
========================
Download [mysql-connector-c-6.1.5-win32.msi](http://dev.mysql.com/get/Downloads/Connector-C/mysql-connector-c-6.1.5-win32.msi) and install.  
Download http://sourceforge.net/projects/mingw/files/MinGW/Extension/pexports/pexports-0.46/pexports-0.46-mingw32-bin.tar.xz/download and extract pexports.exe to C:\MinGW\bin  
```sh
;The following commands are run in msys environment
;Copy "C:\Program Files\MySQL\MySQL Connector C 6.1\lib\libmysql.dll" to /e/project/c-block-mysql/mingw
$ cd /e/project/c-block-mysql/mingw
$ pexports libmysql.dll>libmysql.def
$ dlltool -k --input-def libmysql.def --dllname libmysql.dll --output-lib libmysql.a
```
```sh
e:\project\c-block-mysql/src/c-block-mysql.c:82: undefined reference to `mysql_real_connect@32'
e:\project\c-block-mysql/src/c-block-mysql.c:87: undefined reference to `mysql_options@12'
e:\project\c-block-mysql/src/c-block-mysql.c:92: undefined reference to `mysql_free_result@4'
e:\project\c-block-mysql/src/c-block-mysql.c:96: undefined reference to `mysql_real_query@12'
e:\project\c-block-mysql/src/c-block-mysql.c:100: undefined reference to `mysql_store_result@4'
e:\project\c-block-mysql/src/c-block-mysql.c:77: undefined reference to `mysql_init@4'
e:\project\c-block-mysql/src/c-block-mysql.c:102: undefined reference to `mysql_field_count@4'
src/test.o: In function `main':
e:\project\c-block-mysql/src/test.c:22: undefined reference to `mysql_server_init@12'
collect2.exe: error: ld returned 1 exit status
make: *** [test.exe] Error 1
```
Edit libmysql.def, replace mysql_real_connect with mysql_real_connect@32 ...  
repeat step: dlltool ...

If you encounter this problem, please follow the steps below.
```sh
test.exe - Entry Point Not Found
The procedure entry point InitializeConditionVariable could not be located in the dynamic link library KERNEL32.dll. 
```

Download http://downloads.mysql.com/archives/get/file/mysql-connector-c-6.0.2-win32.msi and install.

```sh
;Copy "C:\Program Files\MySQL\MySQL Connector C 6.0.2\lib\opt\libmysql.dll" to /e/project/c-block-mysql/mingw
$ cd /e/project/c-block-mysql/mingw
$ pexports libmysql.dll>libmysql.def
$ dlltool -k --input-def libmysql.def --dllname libmysql.dll --output-lib libmysql.a
```
Edit libmysql.def, replace mysql_real_connect with mysql_real_connect@32 ...  
repeat step: dlltool ...

References:  
1. [MySQL and MinGW](http://gluescript.sourceforge.net/?q=node/39)  
2. [Re: Using MinGW with MySQL on Windows](http://forums.mysql.com/read.php?167,295483,297733)  
