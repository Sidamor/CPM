#ifndef _SQLCTL_TEST_H
#define _SQLCTL_TEST_H

#include "sqlite3.h"


class CSqlCtlTest
{
public:
    CSqlCtlTest(void);
    ~CSqlCtlTest(void);

	bool OpenDB(const char* DBName);
    void error(const char* error);                      //打印错误信息
    void create_database(char *name);                   //创建数据库
    void optimize_database(char *name);                 //数据库优化
    void create_table(const char *table);               //创建数据表
    bool insert_into_table(char *table, char *record);  //插入记录
    bool update_into_table(char *table, char *record); 

    bool select_from_table(char *table, char *column, char *condition, char *result);
	bool select_from_tableEx(char *sql, sqlite3_callback FuncSelectCallBack, void *result);

    void print_table(char *table);                      //打印表数据
    bool delete_record(char *table,char *expression);   //删除记录
    void close_database(char *tablename);               //关闭数据库
    void begin_transaction(void);                       //开始事务
    void commit_transaction(void);                      //提交事务
    static int loadData(void *data, int n_column, char **column_value, char **column_name);

	bool ExecSql(char *sql, sqlite3_callback FuncSelectCallBack, void * userData);
private:
    sqlite3* db;
    char* errmsg;
    char **strTable;
};


#endif
