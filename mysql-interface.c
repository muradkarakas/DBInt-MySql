#include "pch.h"

#include <stdlib.h>
#include <string.h>

#include "mysql-interface.h"

#include "..\SodiumShared\SodiumShared.h"

#include <mysql.h>

MYSQL_INTERFACE_API
void 
mysqlExecuteDeleteStatement(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	mysqlExecuteInsertStatement(mkConnection, stm, sql);
}

MYSQL_INTERFACE_API
void 
mysqlExecuteUpdateStatement(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	mysqlExecuteInsertStatement(mkConnection, stm, sql);
}

MYSQL_INTERFACE_API
char* 
mysqlExecuteInsertStatement(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	mkConnection->err = FALSE;
	mkConnection->errText = NULL;
	char* retval = mkMalloc(mkConnection->heapHandle, 15, __FILE__, __LINE__);

	if (stm->statement.mysql.paramCount > 0) {
		bool res = mysql_stmt_bind_param(stm->statement.mysql.statement, stm->statement.mysql.bindVariables);
		if (res == FALSE) {
			
		}
		else {
			mkConnection->err = TRUE;
			mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}
	}

	if (mkConnection->err == FALSE) {
		int result = mysql_stmt_execute(stm->statement.mysql.statement);
		if (result == 0) {
			uint64_t rowCount = mysql_stmt_affected_rows(stm->statement.mysql.statement);
			mkItoa(rowCount, retval);
		}
		else {
			mkConnection->err = TRUE;
			mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}
	}

	return retval;
}

MYSQL_INTERFACE_API
void 
mysqlBindString(
	DBInt_Connection* mkConnection,
	DBInt_Statement* stm, 
	char* bindVariableName, 
	char* bindVariableValue, 
	size_t valueLength
)
{
	mkConnection->err = FALSE;
	mkConnection->errText = NULL;

	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_type = MYSQL_TYPE_STRING;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer = mkMalloc(mkConnection->heapHandle, valueLength, __FILE__, __LINE__);

	memcpy_s((char*)stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer, valueLength, bindVariableValue, strlen(bindVariableValue));
		
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_length = (unsigned long) valueLength;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null = mkMalloc(mkConnection->heapHandle, sizeof(bool), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null) = FALSE;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length = mkMalloc(mkConnection->heapHandle, sizeof(unsigned long), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length) = (unsigned long) valueLength;

	stm->statement.mysql.bindedVariableCount++;	
}

MYSQL_INTERFACE_API
void 
mysqlBindNumber(
	DBInt_Connection* mkConnection,
	DBInt_Statement* stm, 
	char* bindVariableName, 
	char* bindVariableValue, 
	size_t valueLength
)
{
	mkConnection->err = FALSE;
	mkConnection->errText = NULL;
	
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_type = MYSQL_TYPE_LONG;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer = mkMalloc(mkConnection->heapHandle, valueLength, __FILE__, __LINE__);
	strncpy_s(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer, valueLength, bindVariableValue, strlen(bindVariableValue));
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_length = (unsigned long) valueLength;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null = mkMalloc(mkConnection->heapHandle, sizeof(bool), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null) = FALSE;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length = NULL; // length is ignored for numeric and temporal data types because the buffer_type value determines the length of the data value. 
	
	stm->statement.mysql.bindedVariableCount++;
}


MYSQL_INTERFACE_API
void
mysqlExecuteSelectStatement(
	DBInt_Connection* mkConnection,
	DBInt_Statement* stm,
	const char* sql
	)
{
	int result = mysql_stmt_execute(stm->statement.mysql.statement);
	if (result == 0) {

		MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);
		MYSQL_FIELD* fields = mysql_fetch_fields(mysqlres);
		stm->statement.mysql.fieldCount = mysql_num_fields(mysqlres);
		stm->statement.mysql.rs = mkMalloc(mkConnection->heapHandle, sizeof(MYSQL_BIND) * stm->statement.mysql.fieldCount, __FILE__, __LINE__);

		for (int i = 0; i < stm->statement.mysql.fieldCount; i++)
		{
			switch (fields[i].type)
			{
				case MYSQL_TYPE_NEWDECIMAL:
				case MYSQL_TYPE_DECIMAL:
				case MYSQL_TYPE_TINY:
				case MYSQL_TYPE_SHORT:
				case MYSQL_TYPE_LONG:
				case MYSQL_TYPE_FLOAT:
				case MYSQL_TYPE_DOUBLE:
				case MYSQL_TYPE_LONGLONG: {
					stm->statement.mysql.rs[i].buffer_type = MYSQL_TYPE_LONG;
					stm->statement.mysql.rs[i].buffer = mkMalloc(mkConnection->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].buffer_length = 60;
					stm->statement.mysql.rs[i].is_null = mkMalloc(mkConnection->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].length = mkMalloc(mkConnection->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].error = mkMalloc(mkConnection->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].is_unsigned = FALSE;
					break;
				}
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_VARCHAR: {
					unsigned long len = fields[i].length + 4;
					stm->statement.mysql.rs[i].buffer_type = MYSQL_TYPE_STRING;
					stm->statement.mysql.rs[i].buffer = mkMalloc(mkConnection->heapHandle, len, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].buffer_length = len;
					stm->statement.mysql.rs[i].is_null = mkMalloc(mkConnection->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].length = mkMalloc(mkConnection->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].error = mkMalloc(mkConnection->heapHandle, sizeof(bool), __FILE__, __LINE__);
					break;
				}
				default: {
					int a = 0;
					break;
				}
			}
		}

		/////
		/////
		/////

		if (stm->statement.mysql.fieldCount > 0) {

			bool res = mysql_stmt_bind_result(stm->statement.mysql.statement, stm->statement.mysql.rs);
			if (res == FALSE) {
				int status = mysql_stmt_store_result(stm->statement.mysql.statement);
				if (status == 0) {
					status = mysql_stmt_fetch(stm->statement.mysql.statement);
					if (status == 1) {
						mkConnection->err = TRUE;
						mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
					}
					else if (status == MYSQL_NO_DATA) {
						stm->statement.mysql.eof = TRUE;
					}
					else if (status == MYSQL_DATA_TRUNCATED) {
						// TODO:
					}
					else {
						/*uint64_t rowCount = mysql_stmt_affected_rows(stm->statement.mysql.statement);
						if (rowCount > 0) {
							if (rowCount == 1) {
								stm->statement.mysql.eof = TRUE;
							}
							else {
								stm->statement.mysql.eof = FALSE;
							}
						}*/
					}
				}
			}
			else {
				mkConnection->err = TRUE;
				mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
			}

		}
	}
	else {
		mkConnection->err = TRUE;
		mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
}

MYSQL_INTERFACE_API
BOOL
mysqlCommit(
	DBInt_Connection* mkConnection
	)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	char* sql = "commit";

	DBInt_Statement* stm = mysqlCreateStatement(mkConnection);
	mysqlPrepare(mkConnection, stm, sql);
	mysqlExecuteAnonymousBlock(mkConnection, stm, sql);

	// starting a new transaction
	//if (mkConnection->errText == NULL) {
	//	beginNewTransaction(mkConnection);
	//}

	// on success returns true
	return (mkConnection->errText != NULL);
}

MYSQL_INTERFACE_API
BOOL
mysqlRollback(
	DBInt_Connection* mkConnection
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	char* sql = "rollback";

	DBInt_Statement* stm = mysqlCreateStatement(mkConnection);
	mysqlPrepare(mkConnection, stm, sql);
	mysqlExecuteAnonymousBlock(mkConnection, stm, sql);

	// starting a new transaction
	//if (mkConnection->errText == NULL) {
	//	beginNewTransaction(mkConnection);
	//}

	// on success returns true
	return (mkConnection->errText != NULL);
}

MYSQL_INTERFACE_API
void
mysqlPrepare(
	DBInt_Connection* mkConnection,
	DBInt_Statement* stm,
	const char* sql
	)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;

	stm->statement.mysql.statement = mysql_stmt_init(mkConnection->connection.mysqlHandle);
	if (stm->statement.mysql.statement) {
		
		unsigned long sqlLength = (unsigned long) strnlen_s(sql, 2000);
		int result = mysql_stmt_prepare(stm->statement.mysql.statement, sql, sqlLength);
		if (result == 0) {
			
			stm->statement.mysql.paramCount = mysql_stmt_param_count(stm->statement.mysql.statement);
			if (stm->statement.mysql.paramCount > 0) {
				stm->statement.mysql.bindVariables = mkMalloc(mkConnection->heapHandle, sizeof(MYSQL_BIND) * stm->statement.mysql.paramCount, __FILE__, __LINE__);
			}

			//
			//
			//

			

			/////
			/////
			/////
		}
		else {
			mkConnection->err = TRUE;
			mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}

		///
		///
		///
			
		
	}
	else {
		mkConnection->err = TRUE;
		mkConnection->errText = "Out of memory";
	}
}


MYSQL_INTERFACE_API
void
mysqlFreeStatement(
	DBInt_Connection* mkConnection,
	DBInt_Statement* stm
	)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	/*
	if (stm == NULL || mkConnection == NULL) {
		return;
	}
	if (stm->statement.mysql.rs && stm->statement.mysql.fieldCount > 0) {
		if (stm->statement.mysql.rs->buffer) {
			mkFree(mkConnection->heapHandle, stm->statement.mysql.rs->buffer);
		}
		
		mkFree(mkConnection->heapHandle, stm->statement.mysql.rs);
		stm->statement.mysql.rs = NULL;
		stm->statement.mysql.fieldCount = 0;
	}
	if (stm->statement.mysql.statement) {
		mysql_stmt_close(stm->statement.mysql.statement);
		//mysql_free_result(stm->statement.mysql.statement);
		stm->statement.mysql.statement = NULL;
	}*/
}

MYSQL_INTERFACE_API 
void 
mysqlInitConnection(
	DBInt_Connection* conn
)
{
	//char* info = mysql_get_client_info();
}



MYSQL_INTERFACE_API
DBInt_Statement* 
mysqlCreateStatement(
	DBInt_Connection* mkConnection
)
{
	DBInt_Statement* retObj = (DBInt_Statement*)mkMalloc(mkConnection->heapHandle, sizeof(DBInt_Statement), __FILE__, __LINE__);
	retObj->statement.mysql.rs = NULL;
	retObj->statement.mysql.fieldCount = 0;
	retObj->statement.mysql.statement = NULL;
	retObj->statement.mysql.eof = FALSE;
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	return retObj;
}

MYSQL_INTERFACE_API
unsigned int 
mysqlGetColumnCount(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm
) 
{
	mkConnection->err = FALSE;
	mkConnection->errText = NULL;
	return stm->statement.mysql.fieldCount;
}

MYSQL_INTERFACE_API
const char* 
mysqlGetColumnNameByIndex(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	unsigned int index
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	return stm->statement.mysql.statement->fields[index].name;
}

MYSQL_INTERFACE_API 
SODIUM_DATABASE_COLUMN_TYPE
mysqlGetColumnType(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* columnName
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	
	MYSQL_FIELD* field;
	SODIUM_DATABASE_COLUMN_TYPE retVal = HTSQL_COLUMN_TYPE_NOTSET;

	MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);

	while ((field = mysql_fetch_field(mysqlres)))
	{
		if (_stricmp(field->name, columnName) == 0) {
			switch (field->type) {
				case MYSQL_TYPE_YEAR:
				case MYSQL_TYPE_TINY:
				case MYSQL_TYPE_LONG:
				case MYSQL_TYPE_INT24:
				case MYSQL_TYPE_LONGLONG:
				case MYSQL_TYPE_DECIMAL:
				case MYSQL_TYPE_NEWDECIMAL:
				case MYSQL_TYPE_FLOAT:
				case MYSQL_TYPE_DOUBLE:
				case MYSQL_TYPE_SHORT: {
					retVal = HTSQL_COLUMN_TYPE_NUMBER;
					break;
				}
				case MYSQL_TYPE_BIT: {
					retVal = HTSQL_COLUMN_TYPE_NUMBER;
					break;
				}
				case MYSQL_TYPE_DATETIME:
				case MYSQL_TYPE_TIME:
				case MYSQL_TYPE_DATE: {
					retVal = HTSQL_COLUMN_TYPE_DATE;
					break;
				}
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_STRING: {
					retVal = HTSQL_COLUMN_TYPE_TEXT;
					break;
				}
				case MYSQL_TYPE_BLOB: {
					retVal = HTSQL_COLUMN_TYPE_LOB;
					break;
				}
				case MYSQL_TYPE_NULL: {
					retVal = HTSQL_COLUMN_TYPE_NOTSET;
					break;
				}
				case MYSQL_TYPE_SET:
				case MYSQL_TYPE_ENUM: 
				case MYSQL_TYPE_GEOMETRY: {
					retVal = HTSQL_COLUMN_TYPE_OBJECT;
					break;
				}
			}
			break;
		}
	}

	return retVal;
}

MYSQL_INTERFACE_API
unsigned int 
mysqlGetColumnSize(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* columnName
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;

	unsigned int colSize = 0;

	return colSize;
}

MYSQL_INTERFACE_API
void 
mysqlExecuteDescribe(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	/*
	PGresult* res = PQdescribePrepared(mkConnection->connection.postgresqlHandle, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		mkConnection->errText = PQerrorMessage(mkConnection->connection.postgresqlHandle);
		PQclear(res);
	}
	else {
		stm->statement.postgresql.currentRowNum = 0;
		stm->statement.postgresql.resultSet = res;
	}
	POSTCHECK(mkConnection);*/
}

MYSQL_INTERFACE_API
int 
mysqlIsConnectionOpen(
	DBInt_Connection* mkConnection
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	if (mkConnection == NULL) {
		return FALSE;
	}
	if (mkConnection->connection.mysqlHandle == NULL) {
		return FALSE;
	}
	return TRUE;
}

MYSQL_INTERFACE_API
void 
mysqlDestroyConnection(
	DBInt_Connection* mkConnection
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;

	if (mkConnection) {
		if (mkConnection->connection.mysqlHandle) {
			mysql_close(mkConnection->connection.mysqlHandle);
			mkConnection->connection.mysqlHandle = NULL;
		}
	}
}

MYSQL_INTERFACE_API
const char* 
mysqlGetColumnValueByColumnName(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	const char* columnName
)
{
	mkConnection->errText = NULL;
	mkConnection->err = FALSE;
	char* retval = "";
	
	if (stm->statement.mysql.statement) {
		MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);
		/*int num_fields = mysql_num_fields(mysqlres);
		MYSQL_FIELD* fields = mysql_fetch_fields(mysqlres);*/
		MYSQL_FIELD* field;
		int i = 0;
		while ((field = mysql_fetch_field(mysqlres)))
		{
			if (_stricmp(field->name, columnName) == 0) {
				switch (field->type) {
					case MYSQL_TYPE_NEWDECIMAL:
					case MYSQL_TYPE_DECIMAL:
					case MYSQL_TYPE_TINY:
					case MYSQL_TYPE_SHORT:
					case MYSQL_TYPE_FLOAT:
					case MYSQL_TYPE_DOUBLE:
					case MYSQL_TYPE_INT24:
					case MYSQL_TYPE_YEAR:
					case MYSQL_TYPE_LONG:
					case MYSQL_TYPE_LONGLONG: {
						long* p = stm->statement.mysql.rs[i].buffer;
						retval = mkMalloc(mkConnection->heapHandle, 60, __FILE__, __LINE__);
						mkItoa(*p, retval);
						break;
					}
					case MYSQL_TYPE_VARCHAR:
					case MYSQL_TYPE_VAR_STRING:
					case MYSQL_TYPE_STRING: {
						retval = stm->statement.mysql.rs[i].buffer;
						break;
					} default: {
						break;
					}
				} 
				break;
			}
			i++;
		}
	}
	else {
		mkConnection->err = TRUE;
		mkConnection->errText = "You must call DBint_Prepere and DBInt_ExecuteSelectStatement sequencially. Then check if a valid result set available or not";
	}
	return retval;
}



MYSQL_INTERFACE_API 
DBInt_Connection* 
mysqlCreateConnection(
	HANDLE heapHandle, 
	DBInt_SupportedDatabaseType dbType,
	const char* hostName,
	const char* dbName, 
	const char* userName, 
	const char* password
)
{
	DBInt_Connection* mkConnection = (DBInt_Connection*)mkMalloc(heapHandle, sizeof(DBInt_Connection), __FILE__, __LINE__);
	mkConnection->dbType = SODIUM_MYSQL_SUPPORT;
	mkConnection->errText = NULL;
	mkConnection->heapHandle = heapHandle;
	mkConnection->err = FALSE;

	strcpy_s(mkConnection->hostName, HOST_NAME_LENGTH, hostName);

	MYSQL* conn = mysql_init(NULL);
	if (conn == NULL)
	{
		mkConnection->err = TRUE;
		mkConnection->errText = "Not enough memory to allocation MYSQL structure";
	}
	else {
		conn = mysql_real_connect(conn, mkConnection->hostName, userName, password, dbName, 0, NULL, 0);
		if (!conn)
		{
			mkConnection->err = TRUE;
			mkConnection->errText = mysql_error(conn);
			if (mkConnection->errText == NULL || mkConnection->errText[0] == '\0') {
				mkConnection->errText = "Couldn't connect to server.";
			}
			mysql_close(conn);
		}
		else {
			mkConnection->connection.mysqlHandle = conn;

			char* sql = "SET SESSION sql_mode = ''";
			DBInt_Statement* stm = mysqlCreateStatement(mkConnection);
			mysqlPrepare(mkConnection, stm, sql);
			mysqlExecuteAnonymousBlock(mkConnection, stm, sql);
			mysqlFreeStatement(mkConnection, stm);
		}
	} 
	return mkConnection;
}


MYSQL_INTERFACE_API
void
mysqlExecuteAnonymousBlock(
	DBInt_Connection* mkConnection,
	DBInt_Statement* stm,
	const char* sql
	)
{
	int result = mysql_stmt_execute(stm->statement.mysql.statement);
	if (result == 0) {
		/*uint64_t rowCount = mysql_stmt_affected_rows(stm->statement.mysql.statement);
		if (rowCount > 0) {
			stm->statement.mysql.currentRowNum = 0;
			if (rowCount == 1) {
				stm->statement.mysql.eof = TRUE;
			}
			else {
				stm->statement.mysql.eof = FALSE;
			}
		}*/
	}
	else {
		mkConnection->err = TRUE;
		mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
}

MYSQL_INTERFACE_API
void 
mysqlSeek(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm, 
	int rowNum
)
{
	mysql_stmt_data_seek(stm->statement.mysql.statement, rowNum);
}

MYSQL_INTERFACE_API
BOOL	
mysqlNext(
	DBInt_Statement* stm
)
{
	int status = mysql_stmt_fetch(stm->statement.mysql.statement);
	if (status == 1) {
		// TODO:
		//mkConnection->err = TRUE;
		//mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
	else if (status == MYSQL_NO_DATA) {
		uint64_t rowCount = mysql_stmt_num_rows(stm->statement.mysql.statement);		
		stm->statement.mysql.eof = TRUE;
	}
	else if (status == MYSQL_DATA_TRUNCATED) {
		// TODO:
	}
	//stm->statement.mysql.resultSet = mysql_fetch_row(stm->statement.mysql.statement);
	//stm->statement.mysql.eof = (stm->statement.mysql.resultSet == NULL);
	return stm->statement.mysql.eof;
}

MYSQL_INTERFACE_API
BOOL	
mysqlIsEof(
	DBInt_Statement* stm
)
{
	uint64_t rowCount = mysql_stmt_num_rows(stm->statement.mysql.statement);
	return stm->statement.mysql.eof;
}

MYSQL_INTERFACE_API
void	
mysqlFirst(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm
)
{
	/*mysql_stmt_data_seek(stm->statement.mysql.statement, stm->statement.mysql.currentRowNum);
	int status = mysql_stmt_fetch(stm->statement.mysql.statement);
	if (status == 1) {
		mkConnection->err = TRUE;
		mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
	else if (status == MYSQL_NO_DATA) {
		stm->statement.mysql.eof = TRUE;
		stm->statement.mysql.currentRowNum = -1;
	}
	else if (status == MYSQL_DATA_TRUNCATED) {
		// TODO:
	}
	else {

	}*/
	stm->statement.mysql.eof = FALSE;
}

MYSQL_INTERFACE_API
void
mysqlLast(
	DBInt_Connection* mkConnection, 
	DBInt_Statement* stm
){
	unsigned long long rowCount = mysql_stmt_num_rows(stm->statement.mysql.statement);
	mysql_stmt_data_seek(stm->statement.mysql.statement, rowCount - 1);
	int status = mysql_stmt_fetch(stm->statement.mysql.statement);
	if (status == 1) {
		mkConnection->err = TRUE;
		mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
	else if (status == MYSQL_NO_DATA) {
		stm->statement.mysql.eof = TRUE;
	}
	else if (status == MYSQL_DATA_TRUNCATED) {
		// TODO:
	}
	else {

	}
	stm->statement.mysql.eof = TRUE;
}

MYSQL_INTERFACE_API
int	
mysqlPrev(
	DBInt_Statement* stm
) 
{
	MYSQL_ROW_OFFSET currentRow = mysql_stmt_row_tell(stm->statement.mysql.statement);
	/*if (currentRow > 0) {
		stm->statement.mysql.currentRowNum = currentRow - 1;
		stm->statement.mysql.currentRowNum = 0;
		mysql_stmt_data_seek(stm->statement.mysql.statement, stm->statement.mysql.currentRowNum);
		int status = mysql_stmt_fetch(stm->statement.mysql.statement);
		if (status == 1) {
			// TODO:
			//mkConnection->err = TRUE;
			//mkConnection->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}
		else if (status == MYSQL_NO_DATA) {
			stm->statement.mysql.eof = TRUE;
			stm->statement.mysql.currentRowNum = -1;
		}
		else if (status == MYSQL_DATA_TRUNCATED) {
			// TODO:
		}
		else {

		}
		stm->statement.mysql.eof = FALSE;
	}
	else {
		stm->statement.mysql.currentRowNum = 0;
	}*/
	return FALSE;
}