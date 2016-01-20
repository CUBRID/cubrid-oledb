using System;
using System.Text;
using System.Data;
using System.Data.OleDb;
using System.Data.Common;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTest
{
    public class TestCaseIssue
    {
        private static readonly string connString =
         "Provider=CUBRIDProvider;Location=192.168.189.134;Data Source=demodb;User Id=dba;Port=30000";
        private static OleDbConnection conn = new OleDbConnection();

        public static void TestCase_init()
        {
            conn.ConnectionString = connString;
            conn.Open();
        }
        public static void  TestCase_dinit()
        {
            conn.Close();
        }
        static private void ExecuteMultiQueries(OleDbConnection conn, string[] multiQueries)
        {
            OleDbCommand command = new OleDbCommand();
            command.Connection = conn;
            foreach (string query in multiQueries)
            {
                try
                {
                    command.CommandText = query;
                    command.ExecuteNonQuery();
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                }
            }
            command.Dispose();
        }
        private static void DisplayData(System.Data.DataTable table)
        {
            foreach (System.Data.DataRow row in table.Rows)
            {
                foreach (System.Data.DataColumn col in table.Columns)
                {
                    Console.WriteLine("{0} = {1}", col.ColumnName, row[col]);
                }
                Console.WriteLine("============================");
            }
        }
        static private OleDbDataReader CreateReader(OleDbConnection conn, string query)
        {
            OleDbCommand command = new OleDbCommand();
            command.Connection = conn;
            command.CommandText = query;
            OleDbDataReader OleDbReader = command.ExecuteReader();
            OleDbReader.Read();
            return OleDbReader;
        }
        private static void ExecuteSQL(string sql, OleDbConnection conn)
        {
            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                cmd.ExecuteNonQuery();
            }
        }
        public static void case_select()
        {
            using (OleDbCommand cmd = new OleDbCommand("select * from code;", conn))
            {
                OleDbDataReader reader = (OleDbDataReader)cmd.ExecuteReader();
                int i = 0;
                while (reader.Read())
                {
                    i++;
                    // Console.WriteLine(reader.GetInt32(0));
                    Console.WriteLine(reader.GetValue(0).ToString());
                    Assert.IsNotNull(reader);
                }
                if (0 == i)
                {
                    Console.WriteLine("no data.");
                }
            }
        }
        public static void case_transaction()
        {          
            OleDbCommand command = new OleDbCommand();
            OleDbTransaction transaction = null;

            // Set the Connection to the new OleDbConnection.
            command.Connection = conn;

            // Open the connection and execute the transaction.
            try
            {
                Console.WriteLine("connection.Database:" + conn.Database);

                ExecuteSQL("drop table if exists t;", conn);
                ExecuteSQL("create table t(dt bit);", conn);

                // Start a local transaction
                transaction = conn.BeginTransaction();

                // Assign transaction object for a pending local transaction.
                command.Connection = conn;
                command.Transaction = transaction;

                // Execute the commands.
                command.CommandText = "Insert into t (dt) VALUES (B'1')";
                command.ExecuteNonQuery();
                System.Console.WriteLine("Insert 1");
                // output();
                transaction.Commit();
                Console.WriteLine("Both records are written to database.");
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                try
                {
                    // Attempt to roll back the transaction.
                    Console.WriteLine("before end.");
                    transaction.Rollback();
                    Console.WriteLine("Rollback end.");
                }
                catch
                {
                    // Do nothing here; transaction is not active.
                }
            }
            finally
            {
            }
        }
        public static void case_connInfo()
        {
            try
            {
                //  DataTable dtTemp1 = conn.GetSchema(null);//OK
                DataTable dtTemp2 = conn.GetSchema("Tables");//Exception

                string[] restrictions = new string[4];
                restrictions[2] = "CODE";
                DataTable dtTemp3 = conn.GetSchema("Tables", restrictions);//Exception
            }
            catch (Exception err)
            {
                Console.WriteLine(err.Message);
                Console.WriteLine(err.StackTrace);
            }
        }
        public static void case_dataset()
        {
            OleDbDataAdapter da=new OleDbDataAdapter("select * from code;",conn);

            DataSet ds = new DataSet();
            da.Fill(ds);

            DisplayData(ds.Tables[0]);
        }
        public static void case_datatable()
        {
            OleDbDataAdapter da = new OleDbDataAdapter("select * from code;", conn);

            DataTable dt = new DataTable();
            da.Fill(dt);

            DisplayData(dt);
        }
        static public void case_GetDataTypeName()
        {
            // Open Connection
            string strConn = connString;
            OleDbConnection connCubrid = conn;

            // Create a test table with 4 columns in various data type
            string testTable = "table_data_type";
            string strDropTable = string.Format("DROP TABLE {0}", testTable);
            string strCreateTable = string.Format(@"CREATE TABLE {0}(id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,col_1 INT, col_2 VARCHAR, col_3 CHAR,col_4 bit);", testTable);
            string strSqlInsert = string.Format("INSERT INTO {0}(col_1, col_2, col_3) VALUE(123, 'varchar contents', 'a')", testTable);

            // Execute multiple queries
            ExecuteMultiQueries(connCubrid, new string[] { strDropTable, strCreateTable });

            string strSqlSelect = string.Format("SELECT * FROM {0} ORDER BY id DESC;", testTable);
            OleDbDataReader OleDbReader = CreateReader(connCubrid, strSqlSelect);

            // Check for GetDataTypeName returned values
            // Assert.AreEqual("INT", OleDbReader.GetDataTypeName(0));
            // Assert.AreEqual("INT", OleDbReader.GetDataTypeName(1));
            // Assert.AreEqual("VARCHAR", OleDbReader.GetDataTypeName(2));
            // Assert.AreEqual("CHAR", OleDbReader.GetDataTypeName(3));

            Console.WriteLine(OleDbReader.GetDataTypeName(0));
            Console.WriteLine(OleDbReader.GetDataTypeName(1));
            Console.WriteLine(OleDbReader.GetDataTypeName(2));
            Console.WriteLine(OleDbReader.GetDataTypeName(3));
        }
        static public void case_GetInt16_OverBound_Max()
        {
            // Open Connection
            string strConn = connString;
            OleDbConnection connCubrid = conn;

            int test_int = Int16.MaxValue + 1;  //Int32  32768
            string testTable = "t_type_int16_overbound";
            string strCreateTable = string.Format("CREATE TABLE {0}(id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, f_int INT)", testTable);
            string strDropTable = string.Format("DROP TABLE {0}", testTable);
            string strSqlInsert = string.Format("INSERT INTO {0}(f_int) VALUE({1})", testTable, test_int);

            ExecuteMultiQueries(connCubrid, new string[] { strDropTable, strCreateTable, strSqlInsert });

            string strSqlSelect = string.Format("SELECT f_int FROM {0} ORDER BY id DESC;", testTable);
            OleDbDataReader OleDbReader = CreateReader(connCubrid, strSqlSelect);
            // Int16 result = OleDbReader.GetInt16(0);
            Console.WriteLine(test_int);  // output:  32768
            try
            {
                Console.WriteLine(OleDbReader.GetInt16(0));   // output:  -32768, expected result: OleDbException
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }

        }
        static public void case_GetInt16_OverBound_Min()
        {
            // Open Connection
            string strConn = connString;
            OleDbConnection connCubrid = conn;

            // int test_int = Int32.MinValue;  //Int32  -32769
            int test_int = Int32.MinValue;
            string testTable = "t_type_int16_overbound";
            string strCreateTable = string.Format("CREATE TABLE {0}(id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, f_int INT)", testTable);
            string strDropTable = string.Format("DROP TABLE {0}", testTable);
            string strSqlInsert = string.Format("INSERT INTO {0}(f_int) VALUE({1})", testTable, test_int);

            ExecuteMultiQueries(connCubrid, new string[] { strDropTable, strCreateTable, strSqlInsert });

            string strSqlSelect = string.Format("SELECT * FROM {0} ORDER BY id DESC;", testTable);
            OleDbDataReader OleDbReader = CreateReader(connCubrid, strSqlSelect);

            Console.WriteLine(test_int);  // output:  -32769    
            try
            {
                Console.WriteLine(OleDbReader.GetInt16(0));   // output:  -32768, expected result: OleDbException
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }
    }
}
