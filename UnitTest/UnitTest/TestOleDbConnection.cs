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
        public static void Test_OleDbConnection()
        {
            Test_GetSchema();
            Test_GetOleDbSchemaTable();
        }

        private static void Test_GetSchema()
        {
            DataTable table = conn.GetSchema();//OK
            DisplayData(table);

            foreach (System.Data.DataRow row in table.Rows)
            {
                Assert.AreEqual(table.Rows.Count, 9);

                foreach (System.Data.DataColumn col in table.Columns)
                {
                    Assert.AreEqual(col.ColumnName, "CollectionName");
                    Assert.AreEqual(row[col], "MetaDataCollections");
                    break;
                }
                break;
            }
        }
        private static void Test_GetOleDbSchemaTable()
        {
            using (OleDbConnection connection = new
                      OleDbConnection(connString))
            {
                connection.Open();
                DataTable table = connection.GetOleDbSchemaTable(
                    OleDbSchemaGuid.Tables,
                    new object[] { null, null, null, "TABLE" });

                foreach (System.Data.DataRow row in table.Rows)
                {
                    Assert.AreEqual(table.Rows.Count, 16);

                    foreach (System.Data.DataColumn col in table.Columns)
                    {
                        Assert.AreEqual(col.ColumnName, "TABLE_CATALOG");
                        break;
                    }
                    break;
                }
            }
        }
    }
}