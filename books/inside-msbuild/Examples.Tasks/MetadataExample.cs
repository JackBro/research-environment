using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Utilities;
using Microsoft.Build.Framework;

namespace Examples.Tasks
{

public class MetadataExample : Task
{
    #region Properties
    [Required]
    public ITaskItem[] ServerList
    { get; set; }

    [Output]
    public ITaskItem[] Result
    { get; set; }
    #endregion

    public override bool Execute()
    {
        if (ServerList.Length > 0)
        {
            Result = new TaskItem[ServerList.Length];

            for(int i=0;i<Result.Length;i++)
            {
                ITaskItem item = ServerList[i];
                ITaskItem newItem = new TaskItem(item.ItemSpec);
                string fullpath = item.GetMetadata("Fullpath");

                newItem.SetMetadata("ServerName", item.GetMetadata("Name"));
                newItem.SetMetadata("DropLoc", item.GetMetadata("DropLocation"));

                newItem.SetMetadata("IpAddress", string.Format("127.0.0.{0}", i+10));
                Result[i] = newItem;
            }
        }
        return true;
    }
}



}
