using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Build.Utilities;
using Microsoft.Build.Framework;
using System.Threading;
using System.IO;
using System.Text.RegularExpressions;

namespace Examples.Tasks
{
    /// <summary>
    /// This is an MSBuild task that will replace text in a file.
    /// This replacement is executed line by line, so it wouldn't be possible to perform
    /// a replacement that spans more than one line.
    /// </summary>
    public class ReplaceInFile : Task
    {
        #region Fields
        private ITaskItem[] sourceFiles;
        private ITaskItem[] replacedFiles;
        private string pattern;
        private string replacement;
        private bool useRegularExpressions;
        #endregion
        public ReplaceInFile()
        {
            useRegularExpressions = false;
        }
        #region Public properties
        /// <summary>
        /// This is an array of <c>ITaskItem</c>s that contain all the source files
        /// to perform the replacement over.
        /// </summary>
        [Required]
        public ITaskItem[] SourceFiles
        {
            get { return this.sourceFiles; }
            set { this.sourceFiles = value; }
        }
        /// <summary>
        /// This is the pattern that will determine what should be replaced.
        /// </summary>
        [Required]
        public string Pattern
        {
            get { return this.pattern; }
            set { this.pattern = value; }
        }
        /// <summary>
        /// This is what will replace the matched expression.
        /// </summary>
        [Required]
        public string Replacement
        {
            get { return this.replacement; }
            set { this.replacement = value; }
        }
        /// <summary>
        /// Optional input parameter that determines if regular expressions
        /// are used during the replacement.
        /// Default value is <c>false</c>
        /// </summary>
        public bool UseRegularExpressions
        {
            get { return this.useRegularExpressions; }
            set { this.useRegularExpressions = value; }
        }
        /// <summary>
        /// This gets the list of files that contain the replacements
        /// </summary>
        [Output]
        public ITaskItem[] ReplacedFiles
        {
            get { return this.replacedFiles; }
        }
        #endregion

        public override bool Execute()
        {
            
            if (this.SourceFiles == null || this.SourceFiles.Length <= 0)
            {
                string message = "No files to preform replacement over";
                base.Log.LogMessage(MessageImportance.High, message, null);
                return true;
            }

            this.replacedFiles = new ITaskItem[SourceFiles.Length];
            int index = 0;
            foreach (ITaskItem item in SourceFiles)
            {
                string itemPath = item.GetMetadata("FullPath");
                FileInfo file = new FileInfo(itemPath);
                if (!file.Exists)
                {
                    string message = string.Format(
                        "Source file not found [{0}]", itemPath);
                    base.Log.LogWarning(message, null);
                    continue;
                }

                string newFileName = string.Format("{0}{1}{2}", file.Name,
                    DateTime.Now.ToString("yyyyhhmmss"), file.Extension);
                FileInfo destFile = new FileInfo(
                    Path.Combine(file.Directory.FullName, newFileName));
                try
                {
                    using (StreamReader reader = new StreamReader(file.FullName))
                    using (StreamWriter writer = new StreamWriter(destFile.FullName))
                    {
                        string currentLine = null;
                        while ((currentLine = reader.ReadLine()) != null)
                        {
                            string newLine = null;
                            if (useRegularExpressions)
                                newLine = Regex.Replace(currentLine, pattern, replacement);
                            else
                                newLine = currentLine.Replace(Pattern, Replacement);

                            writer.WriteLine(newLine);
                        }
                        writer.Flush();
                    }

                    replacedFiles[index] = new TaskItem(destFile.FullName);
                    index++;
                }
                catch (Exception ex)
                {
                    string message = string.Format(
                        "There was an error trying to perform the replacement. Message: {0}",
                        ex.Message);
                    Log.LogError(message, null);
                    return false;
                }
            }
            return true;
        }
    }
}
