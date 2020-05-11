#include "pch.h"

#include <stdlib.h>
#include <string.h>

#include "mysql-interface.h"

#include "..\SodiumShared\SodiumShared.h"

#include <mysql.h>

MYSQL_INTERFACE_API
void
mysqlFreeStatement(
	DBInt_Connection* conn,
	DBInt_Statement* stm
	)
{
	conn->errText = NULL;
	conn->err = FALSE;

	if (stm == NULL || conn == NULL) {
		return;
	}

	// TODO: memory alanlarýný free et
	if (stm->statement.mysql.rs) {
		if (stm->statement.mysql.statement->field_count > 0) {
			for (unsigned int columnIndex = 0; columnIndex < stm->statement.mysql.statement->field_count; columnIndex++) {
				if (stm->statement.mysql.rs[columnIndex].buffer) {
					if (stm->statement.mysql.rs[columnIndex].buffer_type != MYSQL_TYPE_BLOB) {
						/// DO NOT FREE BLOB COLUMNS. 
						/// Have a look the "DBInt_GetLob" function declaration at "db-interface.h" header file. 
						///	It says: "CALLER IS RESPONSIBLE TO RELEASE RETUN VALUE" for blob column data
						mkFree(conn->heapHandle, stm->statement.mysql.rs[columnIndex].buffer);
						stm->statement.mysql.rs[columnIndex].buffer = NULL;
					}
				}
			}
			mkFree(conn->heapHandle, stm->statement.mysql.rs);
			stm->statement.mysql.rs = NULL;
		}
		stm->statement.mysql.rs = NULL;
	}

	if (stm->statement.mysql.statement) {
		mysql_stmt_close(stm->statement.mysql.statement);
		//mysql_free_result(stm->statement.mysql.statement);
		stm->statement.mysql.statement = NULL;
	}
}

MYSQL_INTERFACE_API
void
mysqlExecuteSelectStatement(
	DBInt_Connection* conn,
	DBInt_Statement* stm,
	const char* sql
)
{
	conn->err = FALSE;
	conn->errText = NULL;
	unsigned int fieldCount = 0;
	int columnIndex = -1;
	int result = mysql_stmt_execute(stm->statement.mysql.statement);
	if (result == 0) {

		MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);
		MYSQL_FIELD* fields = mysql_fetch_fields(mysqlres);
		fieldCount = mysql_num_fields(mysqlres);
		if (stm->statement.mysql.rs == NULL) {
			size_t memSize = sizeof(MYSQL_BIND) * fieldCount;
			stm->statement.mysql.rs = mkMalloc(conn->heapHandle, memSize, __FILE__, __LINE__);
		}

		for (unsigned int i = 0; i < fieldCount; i++)
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
					stm->statement.mysql.rs[i].buffer = mkMalloc(conn->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].buffer_length = 60;
					stm->statement.mysql.rs[i].is_null = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].length = mkMalloc(conn->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].error = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].is_unsigned = FALSE;
					break;
				}
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_VARCHAR: {
					unsigned long len = fields[i].length + 4;
					stm->statement.mysql.rs[i].buffer_type = MYSQL_TYPE_STRING;
					stm->statement.mysql.rs[i].buffer = mkMalloc(conn->heapHandle, len, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].buffer_length = len;
					stm->statement.mysql.rs[i].is_null = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].length = mkMalloc(conn->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].error = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
					break;
				}
				case MYSQL_TYPE_BLOB: {
					columnIndex = i;
					stm->statement.mysql.rs[i].buffer_type = MYSQL_TYPE_BLOB;
					stm->statement.mysql.rs[i].buffer_length = 0;
					stm->statement.mysql.rs[i].buffer = NULL;//  mkMalloc(conn->heapHandle, len, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].is_null = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
					stm->statement.mysql.rs[i].length = mkMalloc(conn->heapHandle, 60, __FILE__, __LINE__);
					stm->statement.mysql.rs[i].error = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
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

		if (fieldCount > 0) {

			bool res = mysql_stmt_bind_result(stm->statement.mysql.statement, stm->statement.mysql.rs);
			if (res == FALSE) {

				int status = mysql_stmt_store_result(stm->statement.mysql.statement);
				if (status == 0) {
					status = mysql_stmt_fetch(stm->statement.mysql.statement);
					if (status == 1) {
						conn->err = TRUE;
						conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
					}
					else if (status == MYSQL_NO_DATA) {
						stm->statement.mysql.eof = TRUE;
					}
					else if (status == MYSQL_DATA_TRUNCATED) {
						if (columnIndex > -1) {
							unsigned long memSize = *stm->statement.mysql.rs[columnIndex].length;
							stm->statement.mysql.rs[columnIndex].buffer_length = memSize;
							stm->statement.mysql.rs[columnIndex].buffer = mkMalloc(conn->heapHandle, memSize, __FILE__, __LINE__);

							status = mysql_stmt_fetch_column(stm->statement.mysql.statement, &stm->statement.mysql.rs[columnIndex], columnIndex, 0);
							if (status != 0) {
								conn->err = TRUE;
								conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
							}
						}
					}
					else {
						
						/*uint64_t rowCount = mysql_stmt_affected_rows(stm->statement.mysql.statement);*/
					}
				}
			}
			else {
				conn->err = TRUE;
				conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
			}

		}
	}
	else {
		conn->err = TRUE;
		conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
}

MYSQL_INTERFACE_API
void * 
mysqlGetLob(
	DBInt_Connection * conn, 
	DBInt_Statement * stm, 
	const char * columnName, 
	DWORD * sizeOfValue
)
{
	conn->err = FALSE;
	conn->errText = NULL;

	void* lobContent = NULL;
	*sizeOfValue = 0;

	if (stm->statement.mysql.statement) {
		MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);
		MYSQL_FIELD* field;
		int i = 0;
		while ((field = mysql_fetch_field(mysqlres)))
		{
			if (_stricmp(field->name, columnName) == 0) {

				if (field->type == MYSQL_TYPE_BLOB) {
					lobContent = stm->statement.mysql.rs[i].buffer;
					*sizeOfValue = stm->statement.mysql.rs[i].buffer_length;
					break;
				}
			}
			i++;
		}
	}
	else {
		conn->err = TRUE;
		conn->errText = "You must call DBint_Prepere and DBInt_ExecuteSelectStatement sequencially. Then check if a valid result set available or not";
	}

	return lobContent;
}

MYSQL_INTERFACE_API
void 
mysqlBindLob(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* imageFileName, 
	char* bindVariableName
)
{
	conn->err = FALSE;
	conn->errText = NULL;

	if (imageFileName == NULL) {
		conn->err = TRUE;
		conn->errText = "No image file name provided";
		return;
	}

	// Opening binary file
	FILE* file = _fsopen(imageFileName, "rb", _SH_DENYNO);

	if (file == NULL) {
		conn->errText = "Image file name is not accessible";
		return;
	}

	// Getting file size 
	fseek(file, 0L, SEEK_END);
	DWORD fileSize = ftell(file);

	if (fileSize > 0) {
		// Reading file content into memory 
		void* buffer = mkMalloc(conn->heapHandle, fileSize, __FILE__, __LINE__);
		fseek(file, 0L, SEEK_SET);
		size_t blocks_read = fread(buffer, fileSize, 1, file);

		if (blocks_read == 1) {

			///
			///
			///
			_mysqlBind(conn, stm, MYSQL_TYPE_BLOB, bindVariableName, buffer, fileSize); 
			///
			///
			///
		}
		else {
			// Error handling 
			//conn->errText = ETEXT(ERR_CORE_IMAGE_CANNOT_BE_READ);
		}
		// Free resources 
		mkFree(conn->heapHandle, buffer);
	}

	// Free resources 
	fclose(file);
}

MYSQL_INTERFACE_API
void 
mysqlExecuteDeleteStatement(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	mysqlExecuteInsertStatement(conn, stm, sql);
}

MYSQL_INTERFACE_API
void 
mysqlExecuteUpdateStatement(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	mysqlExecuteInsertStatement(conn, stm, sql);
}

MYSQL_INTERFACE_API
char* 
mysqlExecuteInsertStatement(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	conn->err = FALSE;
	conn->errText = NULL;
	char* retval = mkMalloc(conn->heapHandle, 15, __FILE__, __LINE__);

	if (stm->statement.mysql.paramCount > 0) {
		bool res = mysql_stmt_bind_param(stm->statement.mysql.statement, stm->statement.mysql.bindVariables);
		if (res == FALSE) {
			
		}
		else {
			conn->err = TRUE;
			conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}
	}

	if (conn->err == FALSE) {
		int result = mysql_stmt_execute(stm->statement.mysql.statement);
		if (result == 0) {
			uint64_t rowCount = mysql_stmt_affected_rows(stm->statement.mysql.statement);
			mkItoa(rowCount, retval);
		}
		else {
			conn->err = TRUE;
			conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}
	}

	return retval;
}


void
_mysqlBind(
	DBInt_Connection* conn,
	DBInt_Statement* stm,
	enum enum_field_types bufferType, 
	char* bindVariableName,
	void* bindVariableValue,
	size_t valueLength
)
{
	conn->err = FALSE;
	conn->errText = NULL;

	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_type = bufferType;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer = mkMalloc(conn->heapHandle, valueLength, __FILE__, __LINE__);

	errno_t err_no = memcpy_s((char*)stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer, valueLength, bindVariableValue, valueLength);
	if (err_no)
	{
		printf("Error executing memcpy_s.\n");
	}

	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_length = (unsigned long)valueLength;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null) = FALSE;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length = mkMalloc(conn->heapHandle, sizeof(unsigned long), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length) = (unsigned long)valueLength;

	stm->statement.mysql.bindedVariableCount++;
}

MYSQL_INTERFACE_API
void 
mysqlBindString(
	DBInt_Connection* conn,
	DBInt_Statement* stm, 
	char* bindVariableName, 
	char* bindVariableValue, 
	size_t valueLength
)
{
	conn->err = FALSE;
	conn->errText = NULL;

	///
	///
	///
	_mysqlBind(conn, stm, MYSQL_TYPE_STRING, bindVariableName, bindVariableValue, valueLength);
	///
	///
	///
	/*
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_type = MYSQL_TYPE_STRING;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer = mkMalloc(conn->heapHandle, valueLength, __FILE__, __LINE__);
	memcpy_s((char*)stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer, valueLength, bindVariableValue, strlen(bindVariableValue));
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_length = (unsigned long) valueLength;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null) = FALSE;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length = mkMalloc(conn->heapHandle, sizeof(unsigned long), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length) = (unsigned long) valueLength;
	stm->statement.mysql.bindedVariableCount++;	
	*/
}

MYSQL_INTERFACE_API
void 
mysqlBindNumber(
	DBInt_Connection* conn,
	DBInt_Statement* stm, 
	char* bindVariableName, 
	char* bindVariableValue, 
	size_t valueLength
)
{
	conn->err = FALSE;
	conn->errText = NULL;

	///
	///
	///
	_mysqlBind(conn, stm, MYSQL_TYPE_LONG, bindVariableName, bindVariableValue, valueLength);
	///
	///
	///
	
	/*stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_type = MYSQL_TYPE_LONG;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer = mkMalloc(conn->heapHandle, valueLength, __FILE__, __LINE__);
	strncpy_s(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer, valueLength, bindVariableValue, strlen(bindVariableValue));
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].buffer_length = (unsigned long) valueLength;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null = mkMalloc(conn->heapHandle, sizeof(bool), __FILE__, __LINE__);
	*(stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].is_null) = FALSE;
	stm->statement.mysql.bindVariables[stm->statement.mysql.bindedVariableCount].length = NULL; // length is ignored for numeric and temporal data types because the buffer_type value determines the length of the data value. 
	stm->statement.mysql.bindedVariableCount++;*/
}

MYSQL_INTERFACE_API
BOOL
mysqlCommit(
	DBInt_Connection* conn
	)
{
	conn->errText = NULL;
	conn->err = FALSE;
	char* sql = "commit";

	DBInt_Statement* stm = mysqlCreateStatement(conn);
	mysqlPrepare(conn, stm, sql);
	mysqlExecuteAnonymousBlock(conn, stm, sql);

	// starting a new transaction
	//if (conn->errText == NULL) {
	//	beginNewTransaction(conn);
	//}

	// on success returns true
	return (conn->errText != NULL);
}

MYSQL_INTERFACE_API
BOOL
mysqlRollback(
	DBInt_Connection* conn
)
{
	conn->errText = NULL;
	conn->err = FALSE;
	char* sql = "rollback";

	DBInt_Statement* stm = mysqlCreateStatement(conn);
	mysqlPrepare(conn, stm, sql);
	mysqlExecuteAnonymousBlock(conn, stm, sql);

	// starting a new transaction
	//if (conn->errText == NULL) {
	//	beginNewTransaction(conn);
	//}

	// on success returns true
	return (conn->errText != NULL);
}

MYSQL_INTERFACE_API
void
mysqlPrepare(
	DBInt_Connection* conn,
	DBInt_Statement* stm,
	const char* sql
	)
{
	conn->errText = NULL;
	conn->err = FALSE;

	stm->statement.mysql.statement = mysql_stmt_init(conn->connection.mysqlHandle);
	if (stm->statement.mysql.statement) {
		bool flag = 1;
		int res = mysql_stmt_attr_set(stm->statement.mysql.statement, STMT_ATTR_UPDATE_MAX_LENGTH, &flag);
		if (res == TRUE) {
			conn->err = TRUE;
			conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
		}
		else {
			unsigned long sqlLength = (unsigned long)strnlen_s(sql, 2000);
			int result = mysql_stmt_prepare(stm->statement.mysql.statement, sql, sqlLength);
			if (result == 0) {
				stm->statement.mysql.paramCount = mysql_stmt_param_count(stm->statement.mysql.statement);
				if (stm->statement.mysql.paramCount > 0) {
					stm->statement.mysql.bindVariables = mkMalloc(conn->heapHandle, sizeof(MYSQL_BIND) * stm->statement.mysql.paramCount, __FILE__, __LINE__);
				}
			}
			else {
				conn->err = TRUE;
				conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
			}
		}	
	}
	else {
		conn->err = TRUE;
		conn->errText = "Out of memory";
	}
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
	DBInt_Connection* conn
)
{
	DBInt_Statement* retObj = (DBInt_Statement*)mkMalloc(conn->heapHandle, sizeof(DBInt_Statement), __FILE__, __LINE__);
	retObj->statement.mysql.rs = NULL;
	retObj->statement.mysql.statement = NULL;
	retObj->statement.mysql.eof = FALSE;
	conn->errText = NULL;
	conn->err = FALSE;
	return retObj;
}

MYSQL_INTERFACE_API
unsigned int 
mysqlGetColumnCount(
	DBInt_Connection* conn, 
	DBInt_Statement* stm
) 
{
	conn->err = FALSE;
	conn->errText = NULL;
	return stm->statement.mysql.statement->field_count;
}

MYSQL_INTERFACE_API
const char* 
mysqlGetColumnNameByIndex(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	unsigned int index
)
{
	conn->errText = NULL;
	conn->err = FALSE;
	return stm->statement.mysql.statement->fields[index].name;
}

MYSQL_INTERFACE_API 
SODIUM_DATABASE_COLUMN_TYPE
mysqlGetColumnType(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* columnName
)
{
	conn->errText = NULL;
	conn->err = FALSE;
	
	MYSQL_FIELD* field;
	SODIUM_DATABASE_COLUMN_TYPE retVal = HTSQL_COLUMN_TYPE_NOTSET;

	MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);

	if (mysqlres) {

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

		mysql_free_result(mysqlres);
	}

	return retVal;
}

MYSQL_INTERFACE_API
unsigned int 
mysqlGetColumnSize(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* columnName
)
{
	conn->errText = NULL;
	conn->err = FALSE;

	unsigned int colSize = 0;

	return colSize;
}

MYSQL_INTERFACE_API
void 
mysqlExecuteDescribe(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* sql
)
{
	conn->errText = NULL;
	conn->err = FALSE;
	/*
	PGresult* res = PQdescribePrepared(conn->connection.postgresqlHandle, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		conn->errText = PQerrorMessage(conn->connection.postgresqlHandle);
		PQclear(res);
	}
	else {
		stm->statement.postgresql.currentRowNum = 0;
		stm->statement.postgresql.resultSet = res;
	}
	POSTCHECK(conn);*/
}

MYSQL_INTERFACE_API
int 
mysqlIsConnectionOpen(
	DBInt_Connection* conn
)
{
	conn->errText = NULL;
	conn->err = FALSE;
	if (conn == NULL) {
		return FALSE;
	}
	if (conn->connection.mysqlHandle == NULL) {
		return FALSE;
	}
	return TRUE;
}

MYSQL_INTERFACE_API
void 
mysqlDestroyConnection(
	DBInt_Connection* conn
)
{
	conn->errText = NULL;
	conn->err = FALSE;

	if (conn) {
		if (conn->connection.mysqlHandle) {
			mysql_close(conn->connection.mysqlHandle);
			conn->connection.mysqlHandle = NULL;
		}
	}
}

MYSQL_INTERFACE_API
const char* 
mysqlGetColumnValueByColumnName(
	DBInt_Connection* conn, 
	DBInt_Statement* stm, 
	const char* columnName
)
{
	conn->errText = NULL;
	conn->err = FALSE;
	char* retval = "";
	
	if (stm->statement.mysql.statement) {
		MYSQL_RES* mysqlres = mysql_stmt_result_metadata(stm->statement.mysql.statement);
		if (mysqlres) {
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
							retval = mkMalloc(conn->heapHandle, 60, __FILE__, __LINE__);
							mkItoa(*p, retval);
							break;
						}
						case MYSQL_TYPE_VARCHAR:
						case MYSQL_TYPE_VAR_STRING:
						case MYSQL_TYPE_STRING: {
							retval = stm->statement.mysql.rs[i].buffer;
							break;
						} 
						case MYSQL_TYPE_BLOB: {
							retval = stm->statement.mysql.rs[i].buffer;
							break;
						}
						default: {
							break;
						}
					} 
					break;
				}
				i++;
			}
			mysql_free_result(mysqlres);
		}
	}
	else {
		conn->err = TRUE;
		conn->errText = "You must call DBint_Prepere and DBInt_ExecuteSelectStatement sequencially. Then check if a valid result set available or not";
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
	DBInt_Connection* conn = (DBInt_Connection*)mkMalloc(heapHandle, sizeof(DBInt_Connection), __FILE__, __LINE__);
	conn->dbType = SODIUM_MYSQL_SUPPORT;
	conn->errText = NULL;
	conn->heapHandle = heapHandle;
	conn->err = FALSE;

	strcpy_s(conn->hostName, HOST_NAME_LENGTH, hostName);

	MYSQL* myConn = mysql_init(NULL);
	if (myConn == NULL)
	{
		conn->err = TRUE;
		conn->errText = "Not enough memory to allocation MYSQL structure";
	}
	else {
		myConn = mysql_real_connect(myConn, conn->hostName, userName, password, dbName, 0, NULL, 0);
		if (!myConn)
		{
			conn->err = TRUE;
			conn->errText = mysql_error(myConn);
			if (conn->errText == NULL || conn->errText[0] == '\0') {
				conn->errText = "Couldn't connect to server.";
			}
			mysql_close(myConn);
		}
		else {
			conn->connection.mysqlHandle = myConn;

			char* sql = "SET SESSION sql_mode = ''";
			DBInt_Statement* stm = mysqlCreateStatement(conn);
			mysqlPrepare(conn, stm, sql);
			mysqlExecuteAnonymousBlock(conn, stm, sql);
			mysqlFreeStatement(conn, stm);
		}
	} 
	return conn;
}


MYSQL_INTERFACE_API
void
mysqlExecuteAnonymousBlock(
	DBInt_Connection* conn,
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
		conn->err = TRUE;
		conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
	}
}

MYSQL_INTERFACE_API
void 
mysqlSeek(
	DBInt_Connection* conn, 
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
		//conn->err = TRUE;
		//conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
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
	DBInt_Connection* conn, 
	DBInt_Statement* stm
)
{
	/*mysql_stmt_data_seek(stm->statement.mysql.statement, stm->statement.mysql.currentRowNum);
	int status = mysql_stmt_fetch(stm->statement.mysql.statement);
	if (status == 1) {
		conn->err = TRUE;
		conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
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
	DBInt_Connection* conn, 
	DBInt_Statement* stm
){
	unsigned long long rowCount = mysql_stmt_num_rows(stm->statement.mysql.statement);
	mysql_stmt_data_seek(stm->statement.mysql.statement, rowCount - 1);
	int status = mysql_stmt_fetch(stm->statement.mysql.statement);
	if (status == 1) {
		conn->err = TRUE;
		conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
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
			//conn->err = TRUE;
			//conn->errText = mysql_stmt_error(stm->statement.mysql.statement);
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