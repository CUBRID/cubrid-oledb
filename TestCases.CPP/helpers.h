//Helper functions

#define EQ_STR(str1, str2) !wcscmp(str1, str2)

bool OpenConnection();
bool OpenConnection(DBPROPSET *ps);
void CloseConnection();

bool TestSetup();
bool TestSetup(DBPROPSET *ps);
void TestCleanup();

long TableRowsCount(TCHAR *tableName);
bool ExecuteSQL(TCHAR *sql);
bool DropTable(TCHAR *tableName);

void SetUpdateableRowsetProperties(CDBPropSet* pPropSet);




