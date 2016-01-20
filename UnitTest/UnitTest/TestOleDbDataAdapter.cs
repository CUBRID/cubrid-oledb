using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Data.OleDb;
using System.Data;

namespace UnitTest
{
    [TestClass]
    public class TestOleDbDataAdapter
    {
        /**
         * Prepare Test Data
         */
        [ClassInitialize]
        public static void Initialize(TestContext context)
        {

            // The table referencing foreigh key should be droped firstly, or drop the owner tale will fail.
            OleDbCommand command = new OleDbCommand("DROP TABLE IF EXISTS score", TestCases.conn);
            command.ExecuteNonQuery();

            // create student table
            command.CommandText = "DROP TABLE IF EXISTS student";
            command.ExecuteNonQuery();
            command.CommandText = "CREATE TABLE student(id INT AUTO_INCREMENT NOT NULL PRIMARY KEY, name VARCHAR(50) NOT NULL) AUTO_INCREMENT=100";
            command.ExecuteNonQuery();

            // create course table
            command.CommandText = "DROP TABLE IF EXISTS course";
            command.ExecuteNonQuery();
            command.CommandText = "CREATE TABLE course(id INT AUTO_INCREMENT NOT NULL PRIMARY KEY, name VARCHAR(50) NOT NULL) AUTO_INCREMENT=100";
            command.ExecuteNonQuery();

            // create score table
            command.CommandText = "CREATE TABLE score(student_id INT NOT NULL FOREIGN KEY REFERENCES student(id) ON DELETE CASCADE, course_id INT NOT NULL FOREIGN KEY REFERENCES course(id), score INT NOT NULL, PRIMARY KEY(student_id, course_id))";
            command.ExecuteNonQuery();

            // insert data
            command.CommandText = "INSERT INTO student(name) VALUES ('qint'),('weihua'),('guojia')";
            command.ExecuteNonQuery();
            command.CommandText = "INSERT INTO course(name) VALUES ('Database'), ('Software Architecture'), ('Data Structure')";
            command.ExecuteNonQuery();
            command = new OleDbCommand("INSERT INTO score VALUES(?, ?, 90)", TestCases.conn);
            command.Parameters.Add("student_id", OleDbType.Integer);
            command.Parameters.Add("course_id", OleDbType.Integer);
            command.Prepare();
            for (int studentId = 100; studentId < 103; studentId++)
                for (int courseId = 100; courseId < 103; courseId++)
                {
                    command.Parameters[0].Value = studentId;
                    command.Parameters[1].Value = courseId;
                    command.ExecuteNonQuery();
                }
        }

        /**
         * Clean up Test Data
         */
        [ClassCleanup]
        public static void Cleanup()
        {
            // Drop sutdent, course , score tables
           /* OleDbCommand command = new OleDbCommand("DROP TABLE IF EXISTS score", TestCases.conn);
            command.ExecuteNonQuery();
            command.CommandText = "DROP TABLE IF EXISTS student";
            command.ExecuteNonQuery();
            command.CommandText = "DROP TABLE IF EXISTS course";
            command.ExecuteNonQuery();
            * */
        }

        private OleDbDataAdapter createAdapter()
        {
            OleDbDataAdapter adapter = new OleDbDataAdapter();
            adapter.SelectCommand = new OleDbCommand();
            adapter.SelectCommand.Connection = TestCases.conn;
            return adapter;
        }

        [TestMethod]
        public void TestFillDataSetOneSelect()
        {
            //Test execute 1 select statements at once
            OleDbDataAdapter adapter = createAdapter();
            adapter.SelectCommand.CommandText = "select * from student";
            adapter.TableMappings.Add("Table", "student");
            adapter.MissingSchemaAction = MissingSchemaAction.AddWithKey;

            ConnectionState originalState = adapter.SelectCommand.Connection.State;
            DataSet studentDS = new DataSet("Student DS");
            int affectedRowCount = adapter.Fill(studentDS);
            //the connection state should restore to original state
            Assert.AreEqual(originalState, adapter.SelectCommand.Connection.State);
            //all rows in student table should be returned
            Assert.AreEqual(3, affectedRowCount);
            Assert.AreEqual(1, studentDS.Tables.Count);
            Assert.AreEqual(3, studentDS.Tables["student"].Rows.Count);
        }

        [TestMethod]
        public void TestFillDataSetRefreshWith_Schema()
        {
            OleDbDataAdapter adapter = createAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.AddWithKey;
            adapter.SelectCommand.CommandText = "select * from student";
            adapter.TableMappings.Add("Table", "student");

            DataSet studentDS = new DataSet("Student DS");
            adapter.Fill(studentDS);
            int affectedRowCount = adapter.Fill(studentDS);
            Assert.AreEqual(3, affectedRowCount);
            Assert.AreEqual(1, studentDS.Tables.Count);
            Assert.AreEqual(3, studentDS.Tables["student"].Rows.Count);
        }

        [TestMethod]
        public void TestFillDataSetRefreshWithExplicitPK()
        {
            OleDbDataAdapter adapter = createAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.Add;
            adapter.SelectCommand.CommandText = "select * from student";
            adapter.TableMappings.Add("Table", "student");

            DataSet studentDS = new DataSet("Student DS");
            adapter.Fill(studentDS);
            DataTable studentTable = studentDS.Tables["student"];
            studentTable.PrimaryKey = new DataColumn[] { studentTable.Columns["id"] };
            int affectedRowCount = adapter.Fill(studentDS);
            Assert.AreEqual(3, affectedRowCount);
            Assert.AreEqual(1, studentDS.Tables.Count);
            Assert.AreEqual(3, studentDS.Tables["student"].Rows.Count);
        }

        [TestMethod]
        [ExpectedException(typeof(OleDbException))]
        public void TestFillDataSetErrorSelect()
        {
            OleDbDataAdapter adapter = createAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.Add;
            adapter.SelectCommand.CommandText = "select * from s";

            DataSet emptyDS = new DataSet("Empty DS");
            adapter.Fill(emptyDS);
        }

        [TestMethod]
        public void TestFillDataSetDupColumns()
        {
            OleDbDataAdapter adapter = createAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.Add;
            adapter.TableMappings.Add("Table", "stu_crs_scr");
            adapter.SelectCommand.CommandText =
               "SELECT student.id, student.name, course.id, course.name, score FROM student INNER JOIN score ON student.id = score.student_id INNER JOIN course ON score.course_id = course.id WHERE student.id = 100";

            DataSet stuCrsScrDS = new DataSet("Student Course Score DS");
            adapter.Fill(stuCrsScrDS);
            Assert.IsTrue(
                stuCrsScrDS.Tables["stu_crs_scr"].Columns[0].ColumnName.ToLower().Equals("id")
                && stuCrsScrDS.Tables["stu_crs_scr"].Columns[2].ColumnName.ToLower().Equals("id1")
                );
        }

        [TestMethod]
        public void TestFillDataSetMultiSelects()
        {
            OleDbDataAdapter adapter = createAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.Add;
            adapter.TableMappings.Add("Table", "student");
            adapter.TableMappings.Add("Table1", "course");
            adapter.SelectCommand.CommandText = "select * from student;select * from course";

            DataSet stuCrsDS = new DataSet("Student Course DS");
            int affectedRowCount = adapter.Fill(stuCrsDS);
            //Assert.AreEqual(6, affectedRowCount);
            Assert.AreEqual(2, stuCrsDS.Tables.Count);
            Assert.AreEqual(3, stuCrsDS.Tables["student"].Rows.Count);
            Assert.AreEqual(3, stuCrsDS.Tables["course"].Rows.Count);
            Assert.IsNotNull(stuCrsDS.Tables["course"].Rows[1]);
        }

        [TestMethod]
        public void TestFillDataTable()
        {
            OleDbDataAdapter adapter = createAdapter();
            // should has no effect
            adapter.SelectCommand.CommandText = "select * from student;select * from course;";
            DataTable dt = new DataTable("stu");
            int affectedRowCount = adapter.Fill(dt);
            Assert.AreEqual("stu", dt.TableName);
            Assert.AreEqual(3, affectedRowCount);
            Assert.AreEqual(2, dt.Columns.Count);
            Assert.AreEqual(3, dt.Rows.Count);
            Assert.IsNotNull(dt.Columns["id"]);
            Assert.IsNotNull(dt.Columns["name"]);
        }

        [TestMethod]
        public void TestFillDataTableRefreshWithSchema()
        {
            OleDbDataAdapter adapter = createAdapter();
            // should has no effect
            adapter.TableMappings.Add("Table", "stu");
            adapter.MissingSchemaAction = MissingSchemaAction.AddWithKey;
            adapter.SelectCommand.CommandText = "select * from student;";
            DataSet ds = new DataSet();
            DataTable dt = ds.Tables.Add("stu");
            adapter.FillSchema(dt, SchemaType.Mapped);
            int affectedRowCount = adapter.Fill(dt);
            //refresh
            affectedRowCount = adapter.Fill(dt);
            Assert.AreEqual(0, dt.PrimaryKey.Length);
            Assert.AreEqual(3, affectedRowCount);
            Assert.AreEqual(6, dt.Rows.Count);
        }

        [TestMethod]
        public void TestFillDataTableRefreshWithExplicitPK()
        {
            OleDbDataAdapter adapter = createAdapter();
            adapter.SelectCommand.CommandText = "select * from student;";
            DataTable dt = new DataTable("stu");
            int affectedRowCount = adapter.Fill(dt);
            dt.PrimaryKey = new DataColumn[] { dt.Columns["id"] };
            //refresh
            affectedRowCount = adapter.Fill(dt);
            Assert.AreEqual(1, dt.PrimaryKey.Length);
            Assert.AreEqual(3, affectedRowCount);
            Assert.AreEqual(3, dt.Rows.Count);
        }

        [TestMethod]
        public void TestFillDataTableByName() {
            OleDbDataAdapter adapter = createAdapter();
            adapter.SelectCommand.CommandText = "select * from student;select * from course;";

            //no table with the same filled name (case-insensitive)exists
            DataSet ds = new DataSet();
            int affectedRowCount = adapter.Fill(ds, "student");
            Assert.IsNotNull(ds.Tables["student"]);
            Assert.IsNull(ds.Tables["Table2"]);
            Assert.AreEqual(3, affectedRowCount);

            //Only one table with the same filled name (case-insensitive)exists
            ds.Tables.Clear();
            ds.Tables.Add("Student");
            adapter.Fill(ds, "student");
            //Assert.IsNull(ds.Tables["student"]);
            Assert.AreEqual(3, ds.Tables["Student"].Rows.Count);

            //Multiple tables with the same filled name (case-insensitive) exists
            ds.Tables.Clear();
            ds.Tables.Add("Student");
            ds.Tables.Add("StudenT");
            adapter.Fill(ds, "student");
            Assert.IsNotNull(ds.Tables["student"]);
            Assert.AreEqual(3, ds.Tables["student"].Rows.Count);
        }

        [TestMethod]
        public void TestFillDataTableWithADODBRecordSet()
        {
            OleDbDataAdapter adapter = new OleDbDataAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.AddWithKey;
            DataSet stuDS = new DataSet();
            DataTable stuTable = new DataTable("student");
            //stuTable.Columns.Add("id", typeof(int));
            //stuTable.Columns.Add("name", typeof(string));
            stuDS.Tables.Add(stuTable);
            //Use ADO objects from ADO library (msado15.dll) imported
            //  as.NET library ADODB.dll using TlbImp.exe
            ADODB.Connection adoConn = new ADODB.Connection();
            ADODB.Recordset adoRS = new ADODB.Recordset();
            adoConn.Open("Provider=CUBRIDProvider;Location=test-db-server;Data Source=demodb;User Id=dba;Port=30000", "", "", -1);
            adoRS.Open("SELECT * FROM student", adoConn, ADODB.CursorTypeEnum.adOpenDynamic, ADODB.LockTypeEnum.adLockReadOnly, 1);
            int fillRowCount = adapter.Fill(stuTable, adoRS);
            adoRS.Close();
            adoConn.Close();
            Assert.AreEqual(3, fillRowCount);
        }

        [TestMethod]
        public void TestRefreshDataTableWithADODBRecordSet_PK()
        {
            OleDbDataAdapter adapter = new OleDbDataAdapter();
            DataSet stuDS = new DataSet();
            DataTable stuTable = new DataTable("student");
            stuDS.Tables.Add(stuTable);
            //Use ADO objects from ADO library (msado15.dll) imported
            //  as.NET library ADODB.dll using TlbImp.exe
            ADODB.Connection adoConn = new ADODB.Connection();
            ADODB.Recordset adoRS = new ADODB.Recordset();
            adoConn.Open("Provider=CUBRIDProvider;Location=test-db-server;Data Source=demodb;User Id=dba;Port=30000", "", "", -1);
            adoRS.Open("SELECT * FROM student", adoConn, ADODB.CursorTypeEnum.adOpenDynamic, ADODB.LockTypeEnum.adLockReadOnly, 1);
            adapter.Fill(stuTable, adoRS);
            adoRS.Requery(0);
            stuTable.PrimaryKey = new DataColumn[] { stuTable.Columns["id"] };
            int refreshRowCount = adapter.Fill(stuTable, adoRS);
            adoRS.Close();
            adoConn.Close();
            Assert.IsNotNull(stuTable.PrimaryKey);
            Assert.AreEqual(3, refreshRowCount);
        }

        [TestMethod]
        public void TestRefreshDataTableWithADODBRecordSet_Schema()
        {
            OleDbDataAdapter adapter = new OleDbDataAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.AddWithKey;
            DataSet stuDS = new DataSet();
            DataTable stuTable = new DataTable("student");
            stuDS.Tables.Add(stuTable);
            //Use ADO objects from ADO library (msado15.dll) imported
            //  as.NET library ADODB.dll using TlbImp.exe
            ADODB.Connection adoConn = new ADODB.Connection();
            ADODB.Recordset adoRS = new ADODB.Recordset();
            adoConn.Open("Provider=CUBRIDProvider;Location=test-db-server;Data Source=demodb;User Id=dba;Port=30000", "", "", -1);
            adoRS.Open("SELECT * FROM student", adoConn, ADODB.CursorTypeEnum.adOpenDynamic, ADODB.LockTypeEnum.adLockReadOnly, 1);
            adapter.Fill(stuTable, adoRS);
            adoRS.Requery(0);
            int refreshRowCount = adapter.Fill(stuTable, adoRS); // This method does not call Close on the ADO object when the fill operation is complete.
            adoRS.Close();
            adoConn.Close();
            Assert.IsNotNull(stuTable.PrimaryKey);
            Assert.AreEqual(3, refreshRowCount);
        }

        [TestMethod]
        public void TestFillDataTableByNameWithADODBRecordSet()
        {
            OleDbDataAdapter adapter = new OleDbDataAdapter();
            DataSet stuDS = new DataSet();
            DataTable stuTable = new DataTable("student");
            stuTable.Columns.Add("id", typeof(int));
            stuTable.Columns.Add("name", typeof(string));
            stuDS.Tables.Add(stuTable);
            //Use ADO objects from ADO library (msado15.dll) imported
            //  as.NET library ADODB.dll using TlbImp.exe
            ADODB.Connection adoConn = new ADODB.Connection();
            ADODB.Recordset adoRS = new ADODB.Recordset();
            adoConn.Open("Provider=CUBRIDProvider;Location=test-db-server;Data Source=demodb;User Id=dba;Port=30000", "", "", -1);
            adoRS.Open("SELECT * FROM student", adoConn, ADODB.CursorTypeEnum.adOpenDynamic, ADODB.LockTypeEnum.adLockReadOnly, 1);
            int fillRowCount = adapter.Fill(stuDS, adoRS, "student"); //This method implicitly calls Close on the ADO object when the fill operation is complete.
            //adoRS.Close();
            adoConn.Close();
            Assert.AreEqual(3, fillRowCount);
            Assert.AreEqual(3, stuTable.Rows.Count);
        }

        [TestMethod]
        public void TestUpdateDataRow() {
            OleDbDataAdapter adapter = createAdapter();
            adapter.MissingSchemaAction = MissingSchemaAction.AddWithKey;
            adapter.SelectCommand.CommandText = "select * from student";
            adapter.TableMappings.Add("Table", "student");
            DataSet studentDS = new DataSet("Student DS");
            adapter.FillSchema(studentDS, SchemaType.Mapped);
            DataTable table = studentDS.Tables["student"];
            DataColumn col = studentDS.Tables["student"].Columns["id"];
            col.AutoIncrement = true;
            col.AutoIncrementSeed = 100;
            col.AutoIncrementStep = 1;
            adapter.Fill(studentDS);

            adapter.InsertCommand = new OleDbCommand("insert into student(name) values (?)", TestCases.conn);
            adapter.InsertCommand.Parameters.Add("name", OleDbType.VarChar);
            DataRow row = studentDS.Tables["student"].NewRow();
            row["name"] = "qiancan ";
            studentDS.Tables["student"].Rows.Add(row);
            row = studentDS.Tables["student"].NewRow();
            row["name"] = "liuhongjiang ";
            studentDS.Tables["student"].Rows.Add(row);

            adapter.Update(studentDS.Tables["student"].Select(null, null, DataViewRowState.Added));
        }
    }
}
