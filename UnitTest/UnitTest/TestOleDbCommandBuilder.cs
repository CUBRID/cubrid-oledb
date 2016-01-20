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
        /// Test OleDbTransaction class
        /// </summary>
        public static void Test_OleDbCommandBuilder()
        {
            Test_Command_CommandBuilder();
            Test_Command_QuoteIdentifier();
        }
        private static void Test_Command_CommandBuilder()
        {
            (new OleDbCommand("drop table if exists t", conn)).ExecuteNonQuery();
            (new OleDbCommand("create table t(id int not null default 1,name string default 'cubrid',PRIMARY KEY (id))", conn)).ExecuteNonQuery();
            (new OleDbCommand("insert into t(id,name) value(1,'cubrid');", conn)).ExecuteNonQuery();

            DataSet dataSet = new DataSet();
            DataTable dataTable = new DataTable();
            string query = "select id,name from t";
            OleDbDataAdapter dataAdapter = new OleDbDataAdapter(query, conn);

            OleDbCommandBuilder commandBuilder =new OleDbCommandBuilder(dataAdapter);

            dataAdapter.Fill(dataTable);

            //Without the OleDbCommandBuilder this line would fail.
            foreach (System.Data.DataRow row in dataTable.Rows)
            {
                foreach (System.Data.DataColumn col in dataTable.Columns)
                {
                    Assert.AreEqual(row[col], 1);
                    break;
                }
            }

            OleDbCommand t1 = commandBuilder.GetInsertCommand();
            Assert.AreEqual(t1.CommandText, "INSERT INTO T DEFAULT VALUES");
        }
        private static void Test_Command_QuoteIdentifier()
        {
            string query = "select id,name from t";
            OleDbDataAdapter dataAdapter = new OleDbDataAdapter(query, conn);

            OleDbCommandBuilder commandBuilder = new OleDbCommandBuilder(dataAdapter);

            string sql_quote_conn = commandBuilder.QuoteIdentifier("select id,name from t", conn);
            Assert.AreEqual(sql_quote_conn, "\"select id,name from t\"");

            string sql_unquote_conn = commandBuilder.UnquoteIdentifier("\"select id,name from t\"", conn);
            Assert.AreEqual(sql_unquote_conn, "select id,name from t");
        }
    }
}