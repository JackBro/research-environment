using System;
using System.Collections.Generic;
//using System.Linq;
using System.Text;
using NUnit.Framework;
using Examples.Tasks;
using System.IO;
using Microsoft.Build.Utilities;

namespace unittest.Examples.Tasks
{
    [TestFixture]
    public class MoveTest
    {
        #region Properties
        public string BaseTestDir
        {
            get { return Path.Combine(TestHelper.TestDirectory,@"moveTest\"); }
        }
        public string DestTestDir
        {
            get { return Path.Combine(BaseTestDir, @"dest\"); }
        }
        #endregion


        [TestFixtureSetUp]
        public void Setup()
        {
            DirectoryInfo baseDir = new DirectoryInfo(BaseTestDir);
            DirectoryInfo destDir = new DirectoryInfo(DestTestDir);

            if (baseDir.Exists)
            {
                baseDir.Delete(true);
            }
            if (destDir.Exists)
            {
                destDir.Delete(true);
            }


            baseDir.Create();
            destDir.Create();
        }
        [TestFixtureTearDown]
        public void TearDown()
        {
            DirectoryInfo baseDir = new DirectoryInfo(BaseTestDir);
            DirectoryInfo destDir = new DirectoryInfo(DestTestDir);

            if (baseDir.Exists)
            {
                baseDir.Delete(true);
            }
            if (destDir.Exists)
            {
                destDir.Delete(true);
            }
        }

        [Test]
        public void TestMove01()
        {
            //create the file that is to be moved
            string testFilePath = Path.Combine(TestHelper.TestDirectory, @"fileToMove.txt");
            FileInfo testFile = new FileInfo(testFilePath);
            EnsureDirectory(testFile.Directory.FullName);
            //write some text to the file
            File.WriteAllText(testFilePath,string.Format("Test file from: {0}",GetType().FullName));

            string destDir = Path.Combine(TestHelper.TestDirectory, @"dest\");
            EnsureDirectory(destDir);

            Move move = new Move();
            move.BuildEngine = new MockBuild();
            move.SourceFiles = TestHelper.ConvertToTaskItemArray(testFilePath);
            move.DestinationFolder = new TaskItem(destDir);
            move.Execute();
        }
        private void EnsureDirectory(string dirPath)
        {
            if (string.IsNullOrEmpty(dirPath)) { throw new ArgumentNullException("dirPath"); }

            if (!Directory.Exists(dirPath))
            {
                Directory.CreateDirectory(dirPath);
            }
        }
    }
}
