using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Utilities;
using Microsoft.Build.Framework;

namespace Examples.Tasks
{
public class HelloWorld : ITask
{
    #region ITask Members
    public IBuildEngine BuildEngine
    { get; set; }
    public ITaskHost HostObject
    { get; set; }

    public bool Execute()
    {
        //set up support for logging
        TaskLoggingHelper loggingHelper = new TaskLoggingHelper(this);
        loggingHelper.LogMessageFromText(
            "Hello MSBuild", MessageImportance.High);

        return true;
    }
    #endregion
}
}
