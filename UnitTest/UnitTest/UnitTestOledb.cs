﻿using System;
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
         [ClassInitialize()]
         public static void MyClassInitialize(TestContext testContext)
         {
             TestCaseIssue.TestCase_init();
         }
        //
        // Use ClassCleanup to run code after all tests in a class have run
         [ClassCleanup()]
         public static void MyClassCleanup() 
         {
             TestCaseIssue.TestCase_init();
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
            TestCasesOld.testcase();
        }

        [TestMethod]
        public void TestMethodIssue()
        {
            //
            // TODO: Add test logic	here
            //
            TestCaseIssue.case_select();
            TestCaseIssue.case_transaction();
            TestCaseIssue.case_connInfo();
            TestCaseIssue.case_dataset();
            TestCaseIssue.case_GetDataTypeName();
            TestCaseIssue.case_GetInt16_OverBound_Min();
            TestCaseIssue.case_GetInt16_OverBound_Max();
        }
    }
}