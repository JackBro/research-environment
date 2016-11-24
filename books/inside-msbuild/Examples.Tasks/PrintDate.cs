using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Utilities;
using Microsoft.Build.Framework;

namespace Examples.Tasks
{
    public class PrintDate : Task
    {
        #region Fields
        private DateTime date;
        #endregion

        #region Properties
        [Required]
        public DateTime Date
        {
            get { return this.date; }
            set { this.date = value; }
        }
        #endregion

        public override bool Execute()
        {
            Log.LogMessageFromText(string.Format("Date: {0}",this.Date), MessageImportance.Normal);
            return true;
        }
    }
}
