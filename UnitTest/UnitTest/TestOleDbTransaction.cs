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
        public static void TestTransaction()
        {
            TestCases.Test_Transaction();
            TestCases.Test_Transaction_Rollback();
            TestCases.Test_Transaction_Parameters();
            TestCases.Test_Transaction_Level();
        }
        /// <summary>
        /// Test OleDbTransaction class
        /// </summary>    
        public static void Test_Transaction()
        {
            using (OleDbConnection conn = new OleDbConnection())
            {
                OleDbTransaction transaction = null;
                conn.ConnectionString = TestCases.connString;
                conn.Open();

                TestCases.ExecuteSQL("drop table if exists t", conn);

                transaction = conn.BeginTransaction();

                string sql = "create table t(idx integer)";
                using (OleDbCommand command = new OleDbCommand(sql, conn))
                {
                    command.Transaction = transaction;
                    command.ExecuteNonQuery();
                }

                int tablesCount = TestCases.GetTablesCount("t", conn, transaction);
                Assert.IsTrue(tablesCount == 1);

                transaction.Rollback();

                //Verify the table does not exist
                tablesCount = GetTablesCount("t", conn, transaction);
                Assert.IsTrue(tablesCount == 0);

                transaction = conn.BeginTransaction();

                sql = "create table t(idx integer)";
                using (OleDbCommand command = new OleDbCommand(sql, conn))
                {
                    command.Transaction = transaction;
                    command.ExecuteNonQuery();
                }

                tablesCount = GetTablesCount("t", conn, transaction);
                Assert.IsTrue(tablesCount == 1);

                transaction.Commit();

                tablesCount = GetTablesCount("t", conn, transaction);
                Assert.IsTrue(tablesCount == 1);

                transaction = conn.BeginTransaction();

                TestCases.ExecuteSQL("drop table t", conn, transaction);

                transaction.Commit();

                tablesCount = GetTablesCount("t", conn);
                Assert.IsTrue(tablesCount == 0);
            }
        }
        public static void Test_Transaction_Rollback()
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
        /// <summary>
        /// Test OleDbTransaction class, using parameters
        /// </summary>
        public static void Test_Transaction_Parameters()
        {
          OleDbTransaction tran = null;

          using (OleDbConnection conn = new OleDbConnection())
          {
            conn.ConnectionString = TestCases.connString;
            conn.Open();

            CreateTestTable(conn);

            tran = conn.BeginTransaction();

            string sql = "insert into t values(?, ?, ?, ?, ?, ?)";
            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
              OleDbParameter p1 = new OleDbParameter("?p1", OleDbType.Integer);
              p1.Value = 1;
              cmd.Parameters.Add(p1);

              OleDbParameter p2 = new OleDbParameter("?p2", OleDbType.Char);
              p2.Value = 'A';
              cmd.Parameters.Add(p2);

              OleDbParameter p3 = new OleDbParameter("?p3", OleDbType.VarChar);
              p3.Value = "OleDb";
              cmd.Parameters.Add(p3);

              OleDbParameter p4 = new OleDbParameter("?p4", OleDbType.Double);
              p4.Value = 1.1f;
              cmd.Parameters.Add(p4);

              OleDbParameter p5 = new OleDbParameter("?p5", OleDbType.Double);
              p5.Value = 2.2d;
              cmd.Parameters.Add(p5);

              OleDbParameter p6 = new OleDbParameter("?p6", OleDbType.DBDate);
              p6.Value = DateTime.Now;
              cmd.Parameters.Add(p6);
              cmd.Transaction = tran; 
              cmd.ExecuteNonQuery();

              tran.Commit();
            }

            Assert.IsTrue(GetTableRowsCount("t", conn) == 1);

            CleanupTestTable(conn);
          }
        }

        /// <summary>
        /// Test_Transaction_Level
        /// </summary>
        public static void Test_Transaction_Level()
        {
            OleDbTransaction tran = null;

            using (OleDbConnection conn = new OleDbConnection())
            {
                conn.ConnectionString = TestCases.connString;
                conn.Open();

                CreateTestTable(conn);

                tran = conn.BeginTransaction(IsolationLevel.Serializable);
                Assert.IsTrue(tran.IsolationLevel == IsolationLevel.Serializable);
                tran.Commit();

                tran = conn.BeginTransaction(IsolationLevel.ReadCommitted);
                Assert.IsTrue(tran.IsolationLevel == IsolationLevel.ReadCommitted);
                tran.Commit();

                tran = conn.BeginTransaction(IsolationLevel.ReadUncommitted);
                Assert.IsTrue(tran.IsolationLevel == IsolationLevel.ReadUncommitted);
                tran.Commit();

                tran = conn.BeginTransaction(IsolationLevel.RepeatableRead);
                Assert.IsTrue(tran.IsolationLevel == IsolationLevel.RepeatableRead);
                tran.Commit();

                try
                {
                    tran = conn.BeginTransaction(IsolationLevel.Chaos);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.Message);
                }

                CleanupTestTable(conn);
            }
        }
    }
}