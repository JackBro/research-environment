using System;
using System.Collections.Generic;
//using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace unittest.Examples.Tasks
{
    /// <summary>
    /// Class that provides some const values to the test classes.
    /// This class is based on <c>MSBuild.Community.Tasks.Tests.TaskUtility</c>.
    /// </summary>
    public static class TestHelper
    {
        #region Fields
        private static string testDir;
        private static string assemblyDir;
        private static string assemblyName;
        #endregion

        #region Properties
        public static string AssemblyName
        {
            get
            {
                if (assemblyName == null)
                {
                    assemblyName = Assembly.GetExecutingAssembly().FullName.Split(',')[0];
                }
                return assemblyName;
            }
        }
        public static string TestDirectory
        {
            get
            {
                if (testDir==null)
                {
                    testDir = Path.GetFullPath(Path.Combine(AssemblyDirectory, string.Format("{0}/{1}", "../test", AssemblyName)));
                }
                return testDir;
            }
        }
        public static string AssemblyDirectory
        {
            get
            {
                if (assemblyDir == null)
                {
                    assemblyDir = new Uri(Path.GetDirectoryName(Assembly.GetExecutingAssembly().CodeBase)).AbsolutePath;
                }
                return assemblyDir;
            }
        }
        public static ITaskItem[] ConvertToTaskItemArray(params string[] items)
        {
            TaskItem[] resultList = null;
            if (items != null)
            {
                resultList = new TaskItem[items.Length];
                for (int i = 0; i < resultList.Length; i++)
                {
                    resultList[i] = new TaskItem(items[i]);
                }
            }
            return resultList;
        }
        public static ITaskItem[] ConvertToTaskItemArray(IList<string> items)
        {
            TaskItem[] resultList = null;
            if (items != null)
            {
                resultList = new TaskItem[items.Count];
                int i = 0;
                foreach(string str in items)
                {
                    resultList[i] = new TaskItem(str);
                }
            }
            return resultList;
        }
        #endregion
    }
}
