using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Build.Utilities;
using System.IO;
using Microsoft.Build.Framework;
using Microsoft.Win32;
using Microsoft.Build.Tasks;

namespace Examples.Tasks
{


/// <summary>
/// MSBuild task that uses the makezipexe.exe executable
/// that to create a self extracting exe from a zip file.
/// </summary>
public class MakeZipExe : ToolTask
{
    #region Fields
    private const string ExeName = "makezipexe.exe";
    #endregion

    #region Constructors
    public MakeZipExe()
    {
        Overwrite = false;
    }
    #endregion

    #region Properties
    /// <summary>
    /// The zip file that will be converted to an <c>.exe</c>
    /// </summary>
    [Required]
    public ITaskItem Zipfile { get; set; }
    /// <summary>
    /// The output file where the <c>.exe</c> will be written to.
    /// </summary>
    public ITaskItem OutputFile { get; set; }
    /// <summary>
    /// Value that determines if the task is allowed to overwrite an existing file
    /// at the <c>OutputFile</c> location.
    /// Default value is <c>False</c>.
    /// </summary>
    public bool Overwrite { get; set; }
    #endregion

    protected override bool ValidateParameters()
    {
        base.Log.LogMessageFromText("Validating arguments", MessageImportance.Low);
        if (Zipfile == null)
        {
            //MSBuild will handle this, but just to 
            //make sure we'll put the here anywayz
            string message = "Zipfile not specified, this is a required parameter.";
            base.Log.LogError(message, null);
            return false;
        }
        if (!File.Exists(Zipfile.GetMetadata("FullPath")))
        {
            string message = string.Format("Missing ZipFile [{0}]", Zipfile);
            base.Log.LogError(message, null);
            return false;
        }
        if (OutputFile == null)
        {
            //MSBuild will handle this, but just to make sure we'll put the here anywayz
            string message = "Zipfile not specified, this is a required parameter.";
            base.Log.LogError(message, null);
            return false;
        }
        if (File.Exists(OutputFile.GetMetadata("FullPath")) && !Overwrite)
        {
            string message = string.Format("Output file {0}, Overwrite false.", 
                OutputFile);
            base.Log.LogError(message, null);
            return false;
        }

        return base.ValidateParameters();
    }
    protected override string GenerateFullPathToTool()
    {
        string path = ToolPath;
        //If ToolPath was not provided by the MSBuild script try to find it.
        if (string.IsNullOrEmpty(path))
        {
            //HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\8.0\Setup\VS
            string regKey = @"SOFTWARE\Microsoft\VisualStudio\9.0\Setup\VS";
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(regKey))
            {
                if (key != null)
                {
                    string keyValue = key.GetValue("EnvironmentDirectory", null).ToString();
                    path = keyValue;
                }
            }
        }
        if (string.IsNullOrEmpty(path))
        {
            //HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\8.0\Setup\VS
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey
                (@"SOFTWARE\Microsoft\VisualStudio\8.0\Setup\VS"))
            {
                if (key != null)
                {
                    string keyValue = key.GetValue("EnvironmentDirectory", null).ToString();
                    path = keyValue;
                }
            }

        }

        if (string.IsNullOrEmpty(path))
        {
            Log.LogError("VisualStudio install directory not found",
                null);
            return string.Empty;
        }
        string fullpath = Path.Combine(path, ToolName);
        return fullpath;
    }
    protected override string GenerateCommandLineCommands()
    {
        StringBuilder sb = new StringBuilder();
        if (Zipfile != null)
        {
            sb.Append(
                string.Format("-zipfile:{0} ",
                Zipfile.GetMetadata("FullPath")));
        }
        if(OutputFile != null)
        {
            sb.Append(
                string.Format("-output:{0} ",
                OutputFile.GetMetadata("FullPath")));
        }
        if (Overwrite)
        {
            sb.Append("-overwrite:true ");
        }
        return sb.ToString();
    }
    /// <summary>
    /// Overrides ToolName to return the name of the .exe.
    /// </summary>
    protected override string ToolName
    {
        get { return ExeName; }
    }
}




}
