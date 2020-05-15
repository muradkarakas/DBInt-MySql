#pragma once

#include "..\DBInt\db-interface.h"


#define MYSQL_INTERFACE_API __declspec(dllexport)

/* DDL's PRIVATE FUNCTIONS  */
void										oracleExecuteUpdateInsertStatements(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
void										_mysqlBind(
												DBInt_Connection* mkConnection,
												DBInt_Statement* stm,
												enum enum_field_types bufferType,
												char* bindVariableName,
												void* bindVariableValue,
												size_t valueLength);

/* DDL's PUBLIC FUNCTIONS  */
MYSQL_INTERFACE_API void					mysqlInitConnection(DBInt_Connection* conn);
MYSQL_INTERFACE_API DBInt_Connection*		mysqlCreateConnection(HANDLE heapHandle, DBInt_SupportedDatabaseType dbType, const char* hostName, const char* dbName, const char* userName, const char* password);
MYSQL_INTERFACE_API void					mysqlDestroyConnection(DBInt_Connection* mkConnection);
MYSQL_INTERFACE_API int						mysqlIsConnectionOpen(DBInt_Connection* mkConnection);
MYSQL_INTERFACE_API int						mysqlIsEof(DBInt_Statement* stm);
MYSQL_INTERFACE_API void					mysqlFirst(DBInt_Connection* mkConnection, DBInt_Statement* stm);
MYSQL_INTERFACE_API void					mysqlLast(DBInt_Connection* mkConnection, DBInt_Statement* stm);
MYSQL_INTERFACE_API BOOL					mysqlNext(DBInt_Statement* stm);
MYSQL_INTERFACE_API BOOL					mysqlPrev(DBInt_Statement* stm);
MYSQL_INTERFACE_API DBInt_Statement*		mysqlCreateStatement(DBInt_Connection* mkConnection);
MYSQL_INTERFACE_API void					mysqlFreeStatement(DBInt_Connection* mkConnection, DBInt_Statement* stm);
MYSQL_INTERFACE_API void					mysqlSeek(DBInt_Connection* mkConnection, DBInt_Statement* stm, int rowNum);
MYSQL_INTERFACE_API int						mysqlGetLastError(DBInt_Connection* mkConnection);
MYSQL_INTERFACE_API const char*				mysqlGetLastErrorText(DBInt_Connection* mkConnection);
MYSQL_INTERFACE_API BOOL					mysqlCommit(DBInt_Connection* mkConnection);
/*	CALLER MUST RELEASE RETURN VALUE  */
MYSQL_INTERFACE_API char*					mysqlGetPrimaryKeyColumn(DBInt_Connection* mkDBConnection, const char* schemaName, const char* tableName, int position);
MYSQL_INTERFACE_API BOOL					mysqlRollback(DBInt_Connection* mkConnection);
MYSQL_INTERFACE_API void					mysqlRegisterString(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* bindVariableName, int maxLength);
MYSQL_INTERFACE_API unsigned int			mysqlGetAffectedRows(DBInt_Connection* mkConnection, DBInt_Statement* stm);
MYSQL_INTERFACE_API void					mysqlBindString(DBInt_Connection* mkDBConnection, DBInt_Statement* stm, char* bindVariableName, char* bindVariableValue, size_t valueLength);
MYSQL_INTERFACE_API void					mysqlBindNumber(DBInt_Connection* mkDBConnection, DBInt_Statement* stm, char* bindVariableName, char* bindVariableValue, size_t valueLength);
MYSQL_INTERFACE_API void					mysqlBindLob(DBInt_Connection* mkDBConnection, DBInt_Statement* stm, const char* imageFileName, char* bindVariableName);
MYSQL_INTERFACE_API void					mysqlExecuteSelectStatement(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
MYSQL_INTERFACE_API void					mysqlExecuteDescribe(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
/*  CALLER MUST RELEASE RETURN VALUE */
MYSQL_INTERFACE_API char*					mysqlExecuteInsertStatement(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
MYSQL_INTERFACE_API void					mysqlExecuteDeleteStatement(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
MYSQL_INTERFACE_API void					mysqlExecuteUpdateStatement(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
MYSQL_INTERFACE_API void					mysqlExecuteAnonymousBlock(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
MYSQL_INTERFACE_API void					mysqlPrepare(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* sql);
MYSQL_INTERFACE_API unsigned int			mysqlGetColumnCount(DBInt_Connection* mkConnection, DBInt_Statement* stm);
MYSQL_INTERFACE_API const char*				mysqlGetColumnValueByColumnName(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* columnName);
MYSQL_INTERFACE_API void *					mysqlGetLob(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* columnName, DWORD* sizeOfValue);

MYSQL_INTERFACE_API 
SODIUM_DATABASE_COLUMN_TYPE
mysqlGetColumnType(
	DBInt_Connection * mkConnection, 
	DBInt_Statement * stm, 
	const char * columnName);

MYSQL_INTERFACE_API unsigned int			mysqlGetColumnSize(DBInt_Connection* mkConnection, DBInt_Statement* stm, const char* columnName);
MYSQL_INTERFACE_API const char*				mysqlGetColumnNameByIndex(DBInt_Connection* mkDBConnection, DBInt_Statement* stm, unsigned int index);
/* Caller must release return value */
MYSQL_INTERFACE_API char*					mysqlGetDatabaseName(DBInt_Connection* conn);

