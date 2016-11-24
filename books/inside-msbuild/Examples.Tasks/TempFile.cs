using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace Examples.Tasks
{
/// <summary>
/// This is an MSBuild task that will create and return the name of a temporary file.
/// </summary>
public class TempFile : Task
{
    #region Properties
    /// <summary>
    /// MSBuild output that contains the full path to a temporary file.
    /// </summary>
    [Output]
    public ITaskItem TempFilePath
    { get; private set; }
    #endregion

    public override bool Execute()
    {
        string path = System.IO.Path.GetTempFileName();

        TempFilePath = new TaskItem(path);
        return true;
    }
}



}
