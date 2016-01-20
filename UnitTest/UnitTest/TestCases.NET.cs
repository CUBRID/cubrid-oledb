/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution. 
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer. 
 *
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution. 
 *
 * - Neither the name of the <ORGANIZATION> nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software without 
 *   specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. 
 *
 */

using System;
using System.Collections.Generic;
using System.Data;
using System.Data.OleDb;
using System.Diagnostics;
using System.Reflection;
using System.Text.RegularExpressions;

namespace UnitTest
{
    /// <summary>
    /// Implementation of test cases for the CUBRID OLE DB Provider
    /// </summary>
    public class TestCasesOld
    {
        private static readonly string connString = "Provider=CUBRIDProvider;Location=192.168.189.133;Data Source=demodb;User Id=dba;Port=30000";

        static int executed = 0;
        static int passed = 0;
        static int testCasesCount = 0;

        static List<string> failedTestCases = new List<string>();

        //Specifiy the REGEX name patterns for the test cases to be executed
        //static string[] runFilters = new string[] { @"\w+" }; // "\w+" means: Match any test case name  

        //Specify what test cases to execute (use a regular expression):

        //static bool matchExactName = true; //set to False if you want the runFilters to match ALL test cases with names that begin with ...
        //static string[] runFilters = new string[] { @"Test_SetOperations" };

        static bool matchExactName = false;
        static string[] runFilters = new string[] { @"\w+" };

        /* Documentation and examples for using ADO.NET:
        http://msdn.microsoft.com/en-us/library/e80y5yhx%28v=VS.80%29.aspx
        */

        public static void testcase()
        {
            Console.WriteLine("Test cases execution started...");
            Console.WriteLine();

            TestCasesOld.Run(TestCasesOld.Test_Demo_Basic);
            TestCasesOld.Run(TestCasesOld.Test_Parameters);
            TestCasesOld.Run(TestCasesOld.Test_Various_DataTypes);



            Console.WriteLine();
            Console.WriteLine(String.Format("*** Test cases results ***"));
            Console.WriteLine(String.Format("{0} test case(s) analyzed.", testCasesCount));
            Console.WriteLine(String.Format("{0} test case(s) executed.", executed));
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine(String.Format("{0} test case(s) passed.", passed));
            if (executed - passed > 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine(String.Format("{0} test case(s) failed.", executed - passed));

                Console.WriteLine("The following test cases have failed:");
                foreach (string failedTestCase in failedTestCases)
                {
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine(String.Format("\t{0}", failedTestCase));
                }
            }
            Console.ResetColor();
            Console.WriteLine();
        }

        /// <summary>
        /// Test basic SQL statements execution
        /// </summary>
        private static void Test_Demo_Basic()
        {
            using (OleDbConnection conn = new OleDbConnection())
            {
                conn.ConnectionString = TestCasesOld.connString;
                conn.Open();

                (new OleDbCommand("drop table if exists t", conn)).ExecuteNonQuery();
                (new OleDbCommand("create table t(id int, str string)", conn)).ExecuteNonQuery();
                (new OleDbCommand("insert into t values(1, 'abc')", conn)).ExecuteNonQuery();

                using (OleDbCommand cmd = new OleDbCommand("select * from t", conn))
                {
                    using (OleDbDataReader reader = cmd.ExecuteReader())
                    {
                        reader.Read();
             
                        Debug.Assert(reader.GetName(0).ToLower() == "id");
                        Debug.Assert(reader.GetName(1).ToLower() == "str");

                        Debug.Assert(reader.GetDataTypeName(0) == "DBTYPE_I4");
                        Debug.Assert(reader.GetDataTypeName(1) == "DBTYPE_LONGVARCHAR");

                        Debug.Assert(reader.GetFieldType(0).UnderlyingSystemType.Name == "Int32");
                        Debug.Assert(reader.GetFieldType(1).UnderlyingSystemType.Name == "String");

                        Debug.Assert(reader.GetValue(0).ToString() == "1");
                        Debug.Assert(reader.GetString(1) == "abc");

                        Debug.Assert(reader.GetInt32(0) == 1);
                        Debug.Assert(reader.GetString(1) == "abc");
                    }
                }

                (new OleDbCommand("drop table if exists t", conn)).ExecuteNonQuery();
            }
        }

        /// <summary>
        /// Test SQL statement with parameters
        /// </summary>
        private static void Test_Parameters()
        {
            string val32 = "/asp/sns/member/dotcom-20000.asp";
            string val40 = val32 + new String(' ', 8);

            using (OleDbConnection conn = new OleDbConnection())
            {
                conn.ConnectionString = TestCasesOld.connString;
                conn.Open();

                (new OleDbCommand("DROP TABLE IF EXISTS dat_client_ip_log", conn)).ExecuteNonQuery();
                (new OleDbCommand("CREATE TABLE dat_client_ip_log (request_url character varying(256) DEFAULT '' NOT NULL)", conn)).ExecuteNonQuery();
                //insert 40-chars string
                (new OleDbCommand("INSERT INTO dat_client_ip_log(request_url) VALUES('" + val40 + "')", conn)).ExecuteNonQuery();

                using (OleDbCommand cmd = new OleDbCommand("SELECT request_url, LENGTH(request_url) FROM dat_client_ip_log", conn))
                {
                    using (OleDbDataReader reader = cmd.ExecuteReader())
                    {
                        reader.Read();

                        Debug.Assert(reader.GetString(0) == val32);
                        Debug.Assert(reader.GetInt32(1) == val40.Length);

                        Debug.Assert(reader.GetDataTypeName(0) == "DBTYPE_VARCHAR");
                        Debug.Assert(reader.GetDataTypeName(1) == "DBTYPE_I4");

                        Debug.Assert(reader.GetFieldType(0).UnderlyingSystemType.Name == "String");
                        Debug.Assert(reader.GetFieldType(1).UnderlyingSystemType.Name == "Int32");
                    }
                }


                //Try the same as above, but with parameters
                (new OleDbCommand("DROP TABLE IF EXISTS dat_client_ip_log", conn)).ExecuteNonQuery();
                (new OleDbCommand("CREATE TABLE dat_client_ip_log (request_url character varying(256) DEFAULT '' NOT NULL)", conn)).ExecuteNonQuery();
                //insert 40-chars string
                using (OleDbCommand cmd = new OleDbCommand("INSERT INTO dat_client_ip_log(request_url) VALUES(?)", conn))
                {
                    OleDbParameter parameter = new OleDbParameter("@p1", OleDbType.VarChar, 256);
                    parameter.Value = val40;
                    cmd.Parameters.Add(parameter);

                    cmd.ExecuteNonQuery();
                }

                using (OleDbCommand cmd = new OleDbCommand("SELECT request_url, LENGTH(request_url) FROM dat_client_ip_log", conn))
                {
                    using (OleDbDataReader reader = cmd.ExecuteReader())
                    {
                        reader.Read();

                        Debug.Assert(reader.GetString(0) == val32);
                        Debug.Assert(reader.GetInt32(1) == val40.Length);

                        Debug.Assert(reader.GetDataTypeName(0) == "DBTYPE_VARCHAR");
                        Debug.Assert(reader.GetDataTypeName(1) == "DBTYPE_I4");

                        Debug.Assert(reader.GetFieldType(0).UnderlyingSystemType.Name == "String");
                        Debug.Assert(reader.GetFieldType(1).UnderlyingSystemType.Name == "Int32");
                    }
                }

                (new OleDbCommand("DROP TABLE IF EXISTS dat_client_ip_log", conn)).ExecuteNonQuery();
            }
        }

        /// <summary>
        /// Test CUBRID data types Get...()
        /// </summary>
        private static void Test_Various_DataTypes()
        {
            using (OleDbConnection conn = new OleDbConnection())
            {
                conn.ConnectionString = TestCasesOld.connString;
                conn.Open();

                TestCasesOld.ExecuteSQL("drop table if exists t", conn);

                string sql = "create table t(";
                sql += "c_integer_ai integer AUTO_INCREMENT, ";
                sql += "c_smallint smallint, ";
                sql += "c_integer integer, ";
                sql += "c_bigint bigint, ";
                sql += "c_numeric numeric(15,1), ";
                sql += "c_float float, ";
                sql += "c_decimal decimal(15,3), ";
                sql += "c_double double, ";
                sql += "c_char char, ";
                sql += "c_varchar varchar(4096), ";
                sql += "c_time time, ";
                sql += "c_date date, ";
                sql += "c_timestamp timestamp, ";
                sql += "c_datetime datetime, ";
                sql += "c_bit bit(1), ";
                sql += "c_varbit bit varying(4096), ";
                sql += "c_monetary monetary, ";
                sql += "c_string string";
                sql += ")";
                TestCasesOld.ExecuteSQL(sql, conn);

                sql = "insert into t values(";
                sql += "1, ";
                sql += "11, ";
                sql += "111, ";
                sql += "1111, ";
                sql += "1.1, ";
                sql += "1.11, ";
                sql += "1.111, ";
                sql += "1.1111, ";
                sql += "'a', ";
                sql += "'abcdfghijk', ";
                sql += "TIME '13:15:45 pm', ";
                sql += "DATE '00-10-31', ";
                sql += "TIMESTAMP '13:15:45 10/31/2008', ";
                sql += "DATETIME '13:15:45 10/31/2008', ";
                sql += "B'0', ";
                sql += "B'0', ";
                sql += "123456789, ";
                sql += "'qwerty'";
                sql += ")";
                TestCasesOld.ExecuteSQL(sql, conn);

                sql = "select * from t";
                using (OleDbCommand cmd = new OleDbCommand(sql, conn))
                {
                    try
                    {
                        OleDbDataReader reader = cmd.ExecuteReader();
                        while (reader.Read()) //only one row will be available
                        {
                            Debug.Assert(reader.GetInt32(0) == 1);
                            Debug.Assert(reader.GetInt16(1) == 11);
                            Debug.Assert(reader.GetInt32(2) == 111);
                            Debug.Assert(reader.GetInt64(3) == 1111);
                            Debug.Assert(reader.GetDecimal(4) == (decimal)1.1);
                            Debug.Assert(reader.GetFloat(5) == (float)1.11); //"Single"
                            Debug.Assert(reader.GetDecimal(6) == (decimal)1.111);
                            Debug.Assert(reader.GetDouble(7) == (double)1.1111);

                            //We use GetString() because GetChar() is not supported or System.Data.OleDb.
                            //http://msdn.microsoft.com/en-us/library/system.data.oledb.oledbdatareader.getchar
                            Debug.Assert(reader.GetString(8) == "a"); //"String" ("Char" in CUBRID)

                            Debug.Assert(reader.GetString(9) == "abcdfghijk"); //"String" ("String in CUBRID)

                            //GetGateTime cannot cast just the time value in a DateTime object, so we use TimeSpan
                            Debug.Assert(reader.GetTimeSpan(10) == new TimeSpan(13, 15, 45));//"TimeSpan"

                            Debug.Assert(reader.GetDateTime(11) == new DateTime(2000, 10, 31)); //"DateTime"
                            Debug.Assert(reader.GetDateTime(12) == new DateTime(2008, 10, 31, 13, 15, 45)); //"DateTime"
                            Console.WriteLine(reader.GetValue(13));
                            Debug.Assert(reader.GetDateTime(13) == new DateTime(2008, 10, 31, 13, 15, 45)); //"DateTime"

                            //The GetByte() method does not perform any conversions and the driver does not give tha data as Byte
                            //http://msdn.microsoft.com/en-us/library/system.data.oledb.oledbdatareader.getbyte
                            //Use GetValue() or GetBytes() methods to retrieve BIT coulumn value
                            //     Debug.Assert((reader.GetValue(14) as byte[])[0] == (byte)0); //"Byte[]" ("bit(1)" in CUBRID)
                            //Or
                            Byte[] value = new Byte[1];
                            reader.GetBytes(14, 0, value, 0, 1);
                            
                            // Debug.Assert(value[0] == (byte)0);//"Byte[]" ("bit(1)" in CUBRID)
                            //Debug.Assert((reader.GetValue(14) as byte[])[0] == (byte)0); //"Byte[]" ("bit varying(4096)" in CUBRID)
                            //Or
                            //  reader.GetBytes(15, 0, value, 0, 1);
                            // Debug.Assert(value[0] == (byte)0);//"Byte[]" ("bit varying(4096)" in CUBRID)

                            Debug.Assert(reader.GetDouble(16) == 123456789.0); //"Double" ("Monetary" in CUBRID)
                            Debug.Assert(reader.GetString(17) == "qwerty"); //"String"
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.Message);
                    }
                }

                TestCasesOld.ExecuteSQL("drop table if exists t", conn);
            }
        }


        #region Helpers

        private static OleDbConnection GetDemodbConnection()
        {
            OleDbConnection conn = new OleDbConnection(TestCasesOld.connString);
            conn.Open();

            if (conn == null)
            {
                throw new Exception("Can't connect to the [demodb] database!");
            }

            return conn;
        }

        private static void ExecuteSQL(string sql, OleDbConnection conn)
        {
            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                cmd.ExecuteNonQuery();
            }
        }

        private static void CreateTestTable(OleDbConnection conn)
        {
            TestCasesOld.ExecuteSQL("drop table if exists t", conn);
            TestCasesOld.ExecuteSQL("create table t(a int, b char(10), c string, d float, e double, f date)", conn);
        }

        private static void CleanupTestTable(OleDbConnection conn)
        {
            TestCasesOld.ExecuteSQL("drop table if exists t", conn);
        }

        private static void CreateTestTableLOB(OleDbConnection conn)
        {
            TestCasesOld.ExecuteSQL("drop table if exists t", conn);
            TestCasesOld.ExecuteSQL("create table t(b BLOB, c CLOB)", conn);
        }

        private static void CleanupTestTableLOB(OleDbConnection conn)
        {
            TestCasesOld.ExecuteSQL("drop table if exists t", conn);
        }

        private static int GetTableRowsCount(string tableName, OleDbConnection conn)
        {
            int count = -1;
            string sql = "select count(*) from `" + tableName + "`";

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                count = (int)cmd.ExecuteScalar();
            }

            return count;
        }

        private static int GetTablesCount(string tableName, OleDbConnection conn)
        {
            int count = 0;
            string sql = "select count(*) from db_class where class_name = '" + tableName + "'";

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                count = (int)cmd.ExecuteScalar();
            }

            return count;
        }

        private static object GetSingleValue(string sql, OleDbConnection conn)
        {
            object ret = null;

            using (OleDbCommand cmd = new OleDbCommand(sql, conn))
            {
                ret = cmd.ExecuteScalar();
            }

            return ret;
        }

        private static void RunByName(string testCaseName)
        {
            foreach (string regexFilter in TestCasesOld.runFilters)
            {
                Match match = Regex.Match(testCaseName, regexFilter, RegexOptions.IgnoreCase);
                if (match.Success && (TestCasesOld.matchExactName == false || (TestCasesOld.matchExactName && testCaseName == regexFilter)))
                {
                    Console.WriteLine("Executing: [" + testCaseName + "]" + "...");
                    executed++;

                    try
                    {
                        Type t = typeof(TestCasesOld);
                        MethodInfo method = t.GetMethod(testCaseName, BindingFlags.Static | BindingFlags.NonPublic);
                        if (method == null)
                        {
                            Console.WriteLine("Error - Method not found: " + testCaseName + "!");
                            return;
                        }

                        method.Invoke(null, null);

                        passed++;
                    }
                    catch (TargetInvocationException ex)
                    {
                        //Be aware, this is not working by default in Debugger
                        //Read more here (including how to setup the VS debugger):
                        //http://stackoverflow.com/questions/2658908/why-is-targetinvocationexception-treated-as-uncaught-by-the-ide
                        Console.WriteLine("Error: " + ex.InnerException.Message);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("Error: " + ex.Message);
                    }

                    Console.WriteLine("Completed.");

                    break; //exit foreach, as test case name might match multiple filters
                }
                else
                {
                    //Console.WriteLine("Skipped: [" + testCaseName + "]");
                }
            }
        }

        //This method wil ALWAYS be executed, for each test case, BEFORE the test case execution starts
        //It is recommended to put in here all the setup required by the test cases
        private static void TestSetup()
        {
            try
            {
                using (OleDbConnection conn = new OleDbConnection())
                {
                    conn.ConnectionString = TestCasesOld.connString;
                    conn.Open();

                    ExecuteSQL("drop table if exists t", conn);
                }
            }
            catch { }
        }

        //This method wil ALWAYS be executed, for each test case, AFTER the test case execution end
        //It is recommended to put in here all the cleanup that should be done, to leave the system as it was in the beginning
        private static void TestCleanup()
        {
            try
            {
                using (OleDbConnection conn = new OleDbConnection())
                {
                    conn.ConnectionString = TestCasesOld.connString;
                    conn.Open();

                    ExecuteSQL("drop table if exists t", conn);
                }
            }
            catch { }
        }

        //This method wil ALWAYS be executed, one time only, BEFORE the test cases execution starts
        //It is recommended to put in here all the setup required BEFORE starting executing the test cases
        private static void SuiteSetup()
        {
        }

        //This method wil ALWAYS be executed, one time only, AFTER all the test cases execution ended
        //It is recommended to put in here all the setup required AFTER starting executing the test cases in the suite
        private static void SuiteCleanup()
        {
        }

        private static void RunWithException(Action f, Exception expectedException)
        {
            bool exceptionOccured = false;
            string testCaseName;

            try
            {
                TestCasesOld.SuiteSetup();

                testCaseName = f.Method.ToString().Replace("Void ", "").Replace("()", "");

                if (testCaseName.StartsWith("Test_"))
                    testCasesCount++;

                foreach (string regexFilter in TestCasesOld.runFilters)
                {
                    Match match = Regex.Match(testCaseName, regexFilter, RegexOptions.IgnoreCase);
                    if (match.Success && (TestCasesOld.matchExactName == false || (TestCasesOld.matchExactName && testCaseName == regexFilter)))
                    {
                        Console.WriteLine("Executing: [" + testCaseName + "]" + "...");
                        executed++;

                        try
                        {
                            TestCasesOld.TestSetup();
                            f();
                            TestCasesOld.TestCleanup();
                        }
                        catch (Exception ex)
                        {
                            if (ex.Message == expectedException.Message)
                            {
                                exceptionOccured = true;
                                passed++;
                            }
                            else
                            {
                                Console.WriteLine("Error: " + ex.Message);
                                Console.WriteLine("Details: " + ex.StackTrace);
                            }
                        }

                        break; //exit foreach, as test case name might match multiple filters
                    }
                    else
                    {
                        //Console.WriteLine("Skipped: [" + testCaseName + "]");
                    }
                }
            }
            finally
            {
                TestCasesOld.SuiteCleanup();
            }

            if (!exceptionOccured)
            {
                failedTestCases.Add(testCaseName);
            }
        }

        private static void Run(Action f)
        {
            bool testPassed = false;
            string testCaseName;

            try
            {
                TestCasesOld.SuiteSetup();

                testCaseName = f.Method.ToString().Replace("Void ", "").Replace("()", "");

                if (testCaseName.StartsWith("Test_"))
                    testCasesCount++;

                foreach (string regexFilter in TestCasesOld.runFilters)
                {
                    Match match = Regex.Match(testCaseName, regexFilter, RegexOptions.IgnoreCase);
                    if (match.Success && (TestCasesOld.matchExactName == false || (TestCasesOld.matchExactName && testCaseName == regexFilter)))
                    {
                        Console.WriteLine("Executing: [" + testCaseName + "]" + "...");
                        executed++;

                        try
                        {
                            TestCasesOld.TestSetup();
                            f();
                            TestCasesOld.TestCleanup();

                            passed++;
                            testPassed = true;
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine("Error: " + ex.Message);
                            Console.WriteLine("Details: " + ex.StackTrace);
                        }

                        break; //exit foreach, as test case name might match multiple filters
                    }
                    else
                    {
                        //Console.WriteLine("Skipped: [" + testCaseName + "]");
                    }
                }
            }
            finally
            {
                TestCasesOld.SuiteCleanup();
            }

            if (!testPassed)
            {
                failedTestCases.Add(testCaseName);
            }
        }

        #endregion

    }
}
