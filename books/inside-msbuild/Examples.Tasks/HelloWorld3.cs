using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Utilities;
using Microsoft.Build.Framework;

namespace Examples.Tasks
{
public class HelloWorld3 : Task
{
    #region Properties
    /// <summary>
    /// First name, this is required.
    /// </summary>
    [Required]
    public string FirstName
    { get; set; }
    /// <summary>
    /// Last name, this is optional
    /// </summary>
    public string LastName
    { get; set; }
    #endregion

    public override bool Execute()
    {
        Log.LogMessage(string.Format("Hello {0} {1}", FirstName, LastName));

        return true;
    }
}
}
