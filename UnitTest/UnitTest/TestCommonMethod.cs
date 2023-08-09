using System;
using System.Text;
using System.Data;
using System.Data.OleDb;
using System.Data.Common;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTest
{
    public partial class TestCases
    {
        /// <summary>
        /// Test CUBRIDCom
        /// 
        private static void CleanupTestTable(OleDbConnection conn)
        {
            TestCases.ExecuteSQL("drop table if exists t", conn);
        }
        private static int GetTableRowsCount(string tableName, OleDbConnection conn)
        {
            int count = -1;
            string sql = "select count(*) from `" + tableName + "`";

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                count = Convert.ToInt32(cmd.ExecuteScalar());
            }

            return count;
        }

        private static int GetTablesCount(string tableName, OleDbConnection conn, OleDbTransaction transaction)
        {
            int count = 0;
            string sql = "select count(*) from db_class where class_name = '" + tableName + "'";

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                cmd.Transaction = transaction;
                count = Convert.ToInt32(cmd.ExecuteScalar());
            }

            return count;
        }
        private static int GetTablesCount(string tableName, OleDbConnection conn)
        {
            int count = 0;
            string sql = "select count(*) from db_class where class_name = '" + tableName + "'";

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                count = Convert.ToInt32(cmd.ExecuteScalar());
            }

            return count;
        }
        private static void CreateTestTable(OleDbConnection conn)
        {
            TestCases.ExecuteSQL("drop table if exists t", conn);
            TestCases.ExecuteSQL("create table t(a int, b char(10), c string, d float, e double, f date)", conn);
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
        private static void ExecuteSQL(string sql, OleDbConnection conn, OleDbTransaction transaction)
        {
            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                cmd.Transaction = transaction;
                cmd.ExecuteNonQuery();
            }
        }

    }
}