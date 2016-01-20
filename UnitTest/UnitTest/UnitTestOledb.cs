using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTest
{
    /// <summary>
    /// Summary description for UnitTest1
    /// </summary>
    [TestClass]
    public class UnitTestOledb
    {
        public UnitTestOledb()
        {
            //
            // TODO: Add constructor logic here
            //
        }

        private TestContext testContextInstance;

        /// <summary>
        ///Gets or sets the test context which provides
        ///information about and functionality for the current test run.
        ///</summary>
        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region Additional test attributes
        //
        // You can use the following additional attributes as you write your tests:
        //
        // Use ClassInitialize to run code before running the first test in the class
         //[ClassInitialize()]
         [AssemblyInitialize]
         public static void AsseblyInitialize(TestContext testContext)
         {
             TestCases.TestCase_init();
         }
        //
        // Use ClassCleanup to run code after all tests in a class have run
         //[ClassCleanup()]
        [AssemblyCleanup]
         public static void AsseblyCleanup() 
         {
             TestCases.TestCase_dinit();
         }
        //
        // Use TestInitialize to run code before running each test 
        // [TestInitialize()]
        // public void MyTestInitialize() { }
        //
        // Use TestCleanup to run code after each test has run
        // [TestCleanup()]
        // public void MyTestCleanup() { }
        //
        #endregion

        [TestMethod]
        public void TestMethodOld()
        {
            //
            // TODO: Add test logic	here
            //
            return;
            TestCasesOld.testcase();
        }

        [TestMethod]
        public void Issue()
        {
            //
            // TODO: Add test logic	here
            //
            return;
            TestCases.TestIssue();
        }
        [TestMethod]
        public void Transaction()
        {
            //
            // TODO: Add test logic	here
            //
            return;
            TestCases.TestTransaction();
        }

        [TestMethod]
        public void OleDbCommand()
        {
            //
            // TODO: Add test logic	here
            //
            //return;
            TestCases.Test_OleDbCommand();
            TestCases.Test_OleDbCommandBuilder();
            TestCases.Test_OleDbConnection();
            TestCases.Test_OleDbConnectionStringBuilder();
        }
    } 
}
