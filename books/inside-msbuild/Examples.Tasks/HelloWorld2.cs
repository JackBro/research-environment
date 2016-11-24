using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Utilities;
using Microsoft.Build.Framework;

namespace Examples.Tasks
{
public class HelloWorld2 : Task
{
    public override bool Execute()
    {
        Log.LogMessageFromText
            ("Hello MSBuild from Task!", MessageImportance.High);
        return true;
    }
}
}
