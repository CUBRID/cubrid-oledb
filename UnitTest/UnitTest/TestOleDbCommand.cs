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
        public static void Test_OleDbCommand()
        {
            Test_Command_Cancel();
            Test_Command_CreateParameter();
            Test_Command_Reader();
            Test_Command_Prepare();
            Test_Command_ResetCommandTimeout();
        }
        private static void Test_Command_ResetCommandTimeout()
        {
            using (OleDbCommand cmd = new OleDbCommand("select * from code;", conn))
            {
                Assert.AreEqual(cmd.CommandTimeout,30);
                cmd.CommandTimeout = 20;
                Assert.AreEqual(cmd.CommandTimeout, 20);
                cmd.ResetCommandTimeout();
                Assert.AreEqual(cmd.CommandTimeout, 30);
            }
        }
        private static void Test_Command_Prepare()
        {
            (new OleDbCommand("drop table if exists t", conn)).ExecuteNonQuery();
            (new OleDbCommand("create table t(id int)", conn)).ExecuteNonQuery();

            using (OleDbCommand cmd = new OleDbCommand("insert into t(id) value(?);", conn))
            {
                OleDbParameter para = cmd.CreateParameter();
                para.Value = 10;
                para.OleDbType = OleDbType.Integer;
                para.ParameterName = "id";
                cmd.Parameters.Add(para);

                cmd.Prepare();
                cmd.ExecuteNonQuery();

                int count = GetTableRowsCount("t",conn);
                Assert.AreEqual(count, 1);
            }
        }

        private static void Test_Command_Cancel()
        {
            using (OleDbCommand cmd = new OleDbCommand("select * from code;", conn))
            {
                cmd.ExecuteReader();
                cmd.Cancel();

                OleDbCommand cmd_clone = cmd.Clone();
                Assert.AreEqual(cmd.ToString(), cmd_clone.ToString());
            }
        }

        private static void Test_Command_CreateParameter()
        {
            using (OleDbCommand cmd = new OleDbCommand("select * from code where s_name=?;", conn))
            {
                OleDbParameter para = cmd.CreateParameter();
                para.Value = 'X';
                para.ParameterName = "s_name";
                cmd.Parameters.Add(para);

                OleDbDataReader reader = cmd.ExecuteReader();
                reader.Read();
                Assert.AreEqual(reader.GetString(1), "Mixed");
            }
        }

        private static void Test_Command_Reader()
        {
            (new OleDbCommand("drop table if exists t", conn)).ExecuteNonQuery();
            (new OleDbCommand("create table t(id int, str string)", conn)).ExecuteNonQuery();

            string sql = "select * from code;";
            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                OleDbDataReader reader = cmd.ExecuteReader();
                reader.Read();
                Assert.AreEqual(reader.GetString(1), "Mixed");
            }

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                string data = (string)cmd.ExecuteScalar();
                Assert.AreEqual(data, "X");
            }
        }
    }
}