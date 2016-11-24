using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Build.Utilities;
using System.IO;
using System.Web.Script.Serialization;
using Microsoft.Build.Framework;
using System.Threading;
using System.Globalization;
using System.Resources;
using System.Reflection;
using System.ComponentModel;

#pragma warning disable 0618 // supress JavaScriptSerializer obsolete warning

namespace Examples.Tasks.JSLint
{
    /// <summary>
    /// MSBuild task to invoke the JSLint on Javascript files to verify them.
    /// Which was created by Douglas Crockford.
    /// <see cref="http://www.jslint.com"/>
    /// </summary>
    public class JSLint : Task
    {
        #region static items
        private const string StartDelimiter = "__JSLINT__START__";
        private const string EndDelimiter = "__JSLINT__END__";
        private const string ResultStartDelimiter = "__START__ERROR__REPORT__";
        private const string ResultEndDelimiter = "__END__ERROR__REPORT__";

        private static string extractedResourceFilename = null;
        private static object lockextractedResourceFilename = "lockextractedResourceFilename";

        private static string jslintResourcename = "Examples.Tasks.JSLint.jslintMsbuild.js";
        private List<JavascriptErrorInfo> mAllErrors;
        #endregion

        #region Field
        //TODO: Rename to mScripts
        private ITaskItem[] mScriptFiles;
        private bool mSuccess;
        private JavascriptErrorInfo[] mErrors;
        private int mExitCode;
        private int mTimeout;

        private JSLintOptions mOptions;
        private bool mUseGoodpartsOptions;
        private bool mUseRecommendedOptions;
        public bool mStopOnFirstFailure;
        #endregion

        #region Constructors
        public JSLint()
            : base()
        {
            mExitCode = int.MinValue;
            mTimeout = 60;
            mOptions = new JSLintOptions();
            mStopOnFirstFailure = false;
            mAllErrors = new List<JavascriptErrorInfo>();
            
        }

        #endregion

        #region Properties
        /// <summary>
        /// Item that the JSLint tool will be invoked on.
        /// </summary>
        [Required]
        public ITaskItem[] ScriptFiles
        {
            get { return mScriptFiles; }
            set { mScriptFiles = value; }
        }
        /// <summary>
        /// Timeout of the JSLint in seconds. Must be greater than 0.
        /// </summary>
        public int Timeout
        {
            get { return mTimeout; }
            set { mTimeout = value; }
        }
        /// <summary>
        /// MSBuild input to specify the usage of all GoodParts options
        /// as defined at www.jslint.com
        /// </summary>
        public bool UseGoodpartsOptions
        {
            get { return mUseGoodpartsOptions; }
            set { mUseGoodpartsOptions = value; }
        }
        /// <summary>
        /// MSBuild input to specify the usage of all Recommended options
        /// as defined at www.jslint.com
        /// </summary>
        public bool UseRecommendedOptions
        {
            get { return mUseRecommendedOptions; }
            set { mUseRecommendedOptions = value; }
        }
        /// <summary>
        /// If this is set to <c>true</c> then when the first <c>Javascript</c>
        /// file contains errors, other <c>Javascripts</c> will not be processed.
        /// </summary>
        [DefaultValue(false)]
        public bool StopOnFirstFailure
        {
            get { return mStopOnFirstFailure; }
            set { mStopOnFirstFailure = value; }
        }
        #region JSLintOptions
        /// <summary>
        /// adsafe option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool adsafe { get { return mOptions.adsafe; } set { mOptions.adsafe = value; } }
        /// <summary>
        /// bitwise option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool bitwise { get { return mOptions.bitwise; } set { mOptions.bitwise = value; } }
        /// <summary>
        /// browser option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool browser { get { return mOptions.browser; } set { mOptions.browser = value; } }
        /// <summary>
        /// cap option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool cap { get { return mOptions.cap; } set { mOptions.cap = value; } }
        /// <summary>
        /// debug option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool debug { get { return mOptions.debug; } set { mOptions.debug = value; } }
        /// <summary>
        /// eqeqeq option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool eqeqeq { get { return mOptions.eqeqeq; } set { mOptions.eqeqeq = value; } }
        /// <summary>
        /// evil option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool evil { get { return mOptions.evil; } set { mOptions.evil = value; } }
        /// <summary>
        /// forin option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool forin { get { return mOptions.forin; } set { mOptions.forin = value; } }
        /// <summary>
        /// fragment option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool fragment { get { return mOptions.fragment; } set { mOptions.fragment = value; } }
        /// <summary>
        /// glovar option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool glovar { get { return mOptions.glovar; } set { mOptions.glovar = value; } }
        /// <summary>
        /// laxbreak option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool laxbreak { get { return mOptions.laxbreak; } set { mOptions.laxbreak = value; } }
        /// <summary>
        /// nomen option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool nomen { get { return mOptions.nomen; } set { mOptions.nomen = value; } }
        /// <summary>
        /// on option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool on { get { return mOptions.on; } set { mOptions.on = value; } }
        /// <summary>
        /// passfail option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool passfail { get { return mOptions.passfail; } set { mOptions.passfail = value; } }
        /// <summary>
        /// plusplus option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool plusplus { get { return mOptions.plusplus; } set { mOptions.plusplus = value; } }
        /// <summary>
        /// regexp option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool regexp { get { return mOptions.regexp; } set { mOptions.regexp = value; } }
        /// <summary>
        /// rhino option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool rhino { get { return mOptions.rhino; } set { mOptions.rhino = value; } }
        /// <summary>
        /// undef option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool undef { get { return mOptions.undef; } set { mOptions.undef = value; } }
        /// <summary>
        /// sidebar option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool sidebar { get { return mOptions.sidebar; } set { mOptions.sidebar = value; } }
        /// <summary>
        /// white option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool white { get { return mOptions.white; } set { mOptions.white = value; } }
        /// <summary>
        /// widget option
        /// <see cref="http://www.jslint.com"/>
        /// </summary>
        public bool widget { get { return mOptions.widget; } set { mOptions.widget = value; } }
        #endregion

        #region OUTPUTS
        [Output]
        public bool Success
        {
            get { return mSuccess; }
            set { mSuccess = value; }
        }
        [Output]
        public JavascriptErrorInfo[] Errors
        {
            get { return mErrors; }
        }
        [Output]
        public int ExitCode
        {
            get { return mExitCode; }
            set { mExitCode = value; }
        }
        #endregion

        #endregion
        /// <summary>
        /// Executes the task.
        /// </summary>
        /// <returns></returns>
        public override bool Execute()
        {
            Log.LogMessage(MessageImportance.Low, "Start JSLint Verification", null);
            if (UseGoodpartsOptions && UseRecommendedOptions)
            {
                string message = string.Format("Only one of the options ['{0}','{1}'] can be specified, not both.", "UseGoodpartsOptions", "UseRecommendedOptions");
                ArgumentException ae = new ArgumentException(message);
                Log.LogErrorFromException(ae);
                throw ae;
            }

            JavaScriptSerializer jsSerializer = new JavaScriptSerializer();

            JSLintOptions effectiveOptons = GetEffectiveOptions();
            string optionsJs = jsSerializer.Serialize(effectiveOptons);
            string verifyScriptPath = GetJslintFile();
            string command = string.Format(@"cscript //Nologo ""{0}""", verifyScriptPath);

            bool success = true;

            foreach (ITaskItem scriptFile in ScriptFiles)
            {
                if (!success && this.mStopOnFirstFailure)
                {//dont process more files if one has already failed
                    break;
                }

                //if (Script == null) { throw new ArgumentNullException("Script"); }
                //string scriptPath = Script.GetMetadata("FullPath");
                string scriptPath = scriptFile.GetMetadata("FullPath");

                if (string.IsNullOrEmpty(scriptPath)) { throw new ArgumentNullException("scriptPath"); }
                if (!File.Exists(scriptPath)) { throw new FileNotFoundException(string.Format("Script file not found at [{0}]", scriptPath), scriptPath); }

                try
                {
                    IWshRuntimeLibrary.WshShell shell = new IWshRuntimeLibrary.WshShell();

                    Thread thread = new Thread(delegate()
                    {

                        System.Diagnostics.Debug.WriteLine("started");
                        IWshRuntimeLibrary.WshExec wshExec = shell.Exec(command);

                        //first line must the options
                        wshExec.StdIn.WriteLine(optionsJs);

                        wshExec.StdIn.WriteLine(StartDelimiter);
                        //needed to make line numbers match up
                        wshExec.StdIn.WriteLine(string.Empty);
                        string file = File.ReadAllText(scriptPath);
                        wshExec.StdIn.Write(file);

                        wshExec.StdIn.WriteLine(string.Empty);
                        wshExec.StdIn.WriteLine(EndDelimiter);

                        string errorMessage = wshExec.StdErr.ReadAll();
                        wshExec.Terminate();
                        mExitCode = wshExec.ExitCode;

                        string errorJsonStr = null;
                        if (errorMessage != null && errorMessage.Length > 0)
                        {
                            System.Diagnostics.Debug.WriteLine(errorMessage);
                            int startIndex = errorMessage.IndexOf(ResultStartDelimiter);
                            int endIndex = errorMessage.IndexOf(ResultEndDelimiter);

                            if (startIndex >= 0 && endIndex > startIndex)
                            {
                                startIndex += ResultStartDelimiter.Length;
                                errorJsonStr = errorMessage.Substring(startIndex, endIndex - startIndex).Trim();
                            }
                        }
                        JavascriptErrorInfo[] errors = null;
                        if (!string.IsNullOrEmpty(errorJsonStr))
                        {
                            errors = GetErrorsFromString(errorJsonStr);

                            mAllErrors.AddRange(errors);
                        }
                        if (errors != null)
                        {
                            foreach (JavascriptErrorInfo error in errors)
                            {
                                if (error != null)
                                {
                                    string subCategory = "JavaScript";
                                    string errorCode = "JSLint";
                                    string helpKeyword = "JavascriptError";
                                    int lineNumber = error.Line;
                                    int colNumber = error.Character;

                                    base.Log.LogError(subCategory, errorCode, helpKeyword, scriptPath, error.Line, error.Character, 0, 0, error.Reason, new object[0]);
                                }
                            }
                            success = false;
                        }
                        else
                        {
                            Log.LogMessage(MessageImportance.Normal, string.Format("No errors found in file [{0}]", scriptFile), null);
                        }
                    });

                    thread.Start();
                    thread.Join(Timeout * 1000);
                }
                catch (Exception ex)
                {
                    success = false;
                    string message = ex.Message;
                    Log.LogErrorFromException(ex);
                }
            }

            if (mAllErrors != null && mAllErrors.Count > 0)
            {
                mErrors = mAllErrors.ToArray();
            }

            success = success && mAllErrors.Count == 0;

            Log.LogMessage(MessageImportance.Low, "End JSLint Verification", null);
            return success;
        }
        private JSLintOptions GetEffectiveOptions()
        {
            JSLintOptions result = mOptions;
            if (UseGoodpartsOptions)
            {
                //result = OptionConsts.mGoodOptons;
                result = MergeOptions(OptionConsts.mGoodOptons, mOptions);
            }
            else
            {
                result = MergeOptions(OptionConsts.mRecommendedOptions, mOptions);
            }

            return result;
        }
        private JSLintOptions MergeOptions(JSLintOptions primary, JSLintOptions secondary)
        {
            //If the JSLintOptions object changes we may have to add attributes to specify
            //which properties to merge here, this is kinda primitive as it stands
            Type jsoType = typeof(JSLintOptions);
            JSLintOptions mergedOptions = new JSLintOptions();
            PropertyInfo[] properties = jsoType.GetProperties(BindingFlags.Public | BindingFlags.SetProperty | BindingFlags.Instance);
            foreach (PropertyInfo property in properties)
            {
                if (property.PropertyType.Equals(typeof(bool)))
                {
                    bool primaryValue = (bool)property.GetValue(primary, null);
                    bool secondaryValue = (bool)property.GetValue(secondary, null);

                    if (primaryValue || secondaryValue)
                    {
                        property.SetValue(mergedOptions, true, null);
                    }
                }
            }

            return mergedOptions;
        }
        private JavascriptErrorInfo[] GetErrorsFromString(string str)
        {
            JavascriptErrorInfo[] result = null;
            if (!string.IsNullOrEmpty(str))
            {
                JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
                result = (JavascriptErrorInfo[])jsSerializer.Deserialize<JavascriptErrorInfo[]>(str);
            }

            return result;
        }
        //TODO: Figure out how to delete this object after all instances of the 
        //      task completes
        private string GetJslintFile()
        {
            string result = extractedResourceFilename;
            if (result == null || !File.Exists(result))
            {
                result = null;
                //string resourceName = "Sedodream.MSBuild.Tasks.Javascript.jslintMsbuild.js";
                string resourceName = jslintResourcename;
                try
                {

                    Assembly assembly = this.GetType().Assembly;
                    string[] resourceNames = assembly.GetManifestResourceNames();

                    string filename = string.Format("{0}.js", Guid.NewGuid().ToString().Replace("-", ""));
                    string tempFileFullPath = Path.Combine(System.IO.Path.GetTempPath(), filename);

                    using (Stream stream = assembly.GetManifestResourceStream(resourceName))
                    using (StreamReader reader = new StreamReader(stream))
                    using (StreamWriter writer = new StreamWriter(tempFileFullPath))
                    {
                        while (reader.Peek() != -1)
                        {
                            writer.Write((char)reader.Read());
                        }
                    }

                    if (extractedResourceFilename == null)
                    {
                        lock (lockextractedResourceFilename)
                        {
                            if (extractedResourceFilename == null)
                            {
                                extractedResourceFilename = tempFileFullPath;
                            }
                        }
                    }

                    result = extractedResourceFilename;
                }
                catch (Exception ex)
                {
                    Log.LogError("Unable to extract JSLint resouce with name [{0}]", resourceName);
                    Log.LogErrorFromException(ex);
                    throw;
                }
            }

            return result;
        }
    }

    [Serializable]
    public class JavascriptErrorInfo
    {
        #region Field
        private string mFilepath;
        private int mLine;
        private int mCharacter;
        private string mEvidence;
        private string mReason;
        #endregion

        #region Properties
        private string Filpath
        {
            get { return mFilepath; }
            set { mFilepath = value; }
        }
        public int Line
        {
            get { return mLine; }
            set { mLine = value; }
        }
        public int Character
        {
            get { return mCharacter; }
            set { mCharacter = value; }
        }
        public string Evidence
        {
            get { return mEvidence; }
            set { mEvidence = value; }
        }
        public string Reason
        {
            get { return mReason; }
            set { mReason = value; }
        }
        #endregion
    }
    static class OptionConsts
    {
        public static readonly JSLintOptions mGoodOptons;
        public static readonly JSLintOptions mRecommendedOptions;
        static OptionConsts()
        {
            mRecommendedOptions = new JSLintOptions();
            mRecommendedOptions.eqeqeq = true;
            mRecommendedOptions.glovar = true;
            mRecommendedOptions.nomen = true;
            mRecommendedOptions.undef = true;
            mRecommendedOptions.white = true;

            mGoodOptons = new JSLintOptions();
            mGoodOptons.eqeqeq = true;
            mGoodOptons.glovar = true;
            mGoodOptons.nomen = true;
            mGoodOptons.undef = true;
            mGoodOptons.white = true;
            mGoodOptons.bitwise = true;
        }
    }
    /// <summary>
    /// Simple class that contains the options, this is
    /// serialized into a JSON object and passed to the JS file.
    /// This class is only created because it is passed to
    /// the JS file.
    /// </summary>
    [Serializable]
    class JSLintOptions
    {
        private IDictionary<string, bool> mOptionsBag;
        public JSLintOptions()
        {
            mOptionsBag = new Dictionary<string, bool>();
            mOptionsBag["adsafe"] = false;
            mOptionsBag["bitwise"] = false;
            mOptionsBag["browser"] = false;
            mOptionsBag["cap"] = false;
            mOptionsBag["debug"] = false;
            mOptionsBag["eqeqeq"] = false;
            mOptionsBag["evil"] = false;
            mOptionsBag["forin"] = false;
            mOptionsBag["fragment"] = false;
            mOptionsBag["glovar"] = false;
            mOptionsBag["laxbreak"] = false;
            mOptionsBag["nomen"] = false;
            mOptionsBag["on"] = false;
            mOptionsBag["passfail"] = false;
            mOptionsBag["plusplus"] = false;
            mOptionsBag["regexp"] = false;
            mOptionsBag["rhino"] = false;
            mOptionsBag["undef"] = false;
            mOptionsBag["sidebar"] = false;
            mOptionsBag["white"] = false;
            mOptionsBag["widget"] = false;
        }
        public bool adsafe { get { return mOptionsBag["adsafe"]; } set { mOptionsBag["adsafe"] = value; } }
        public bool bitwise { get { return mOptionsBag["bitwise"]; } set { mOptionsBag["bitwise"] = value; } }
        public bool browser { get { return mOptionsBag["browser"]; } set { mOptionsBag["browser"] = value; } }
        public bool cap { get { return mOptionsBag["cap"]; } set { mOptionsBag["cap"] = value; } }
        public bool debug { get { return mOptionsBag["debug"]; } set { mOptionsBag["debug"] = value; } }
        public bool eqeqeq { get { return mOptionsBag["eqeqeq"]; } set { mOptionsBag["eqeqeq"] = value; } }
        public bool evil { get { return mOptionsBag["evil"]; } set { mOptionsBag["evil"] = value; } }
        public bool forin { get { return mOptionsBag["forin"]; } set { mOptionsBag["forin"] = value; } }
        public bool fragment { get { return mOptionsBag["fragment"]; } set { mOptionsBag["fragment"] = value; } }
        public bool glovar { get { return mOptionsBag["glovar"]; } set { mOptionsBag["glovar"] = value; } }
        public bool laxbreak { get { return mOptionsBag["laxbreak"]; } set { mOptionsBag["laxbreak"] = value; } }
        public bool nomen { get { return mOptionsBag["nomen"]; } set { mOptionsBag["nomen"] = value; } }
        public bool on { get { return mOptionsBag["on"]; } set { mOptionsBag["on"] = value; } }
        public bool passfail { get { return mOptionsBag["passfail"]; } set { mOptionsBag["passfail"] = value; } }
        public bool plusplus { get { return mOptionsBag["plusplus"]; } set { mOptionsBag["plusplus"] = value; } }
        public bool regexp { get { return mOptionsBag["regexp"]; } set { mOptionsBag["regexp"] = value; } }
        public bool rhino { get { return mOptionsBag["rhino"]; } set { mOptionsBag["rhino"] = value; } }
        public bool undef { get { return mOptionsBag["undef"]; } set { mOptionsBag["undef"] = value; } }
        public bool sidebar { get { return mOptionsBag["sidebar"]; } set { mOptionsBag["sidebar"] = value; } }
        public bool white { get { return mOptionsBag["white"]; } set { mOptionsBag["white"] = value; } }
        public bool widget { get { return mOptionsBag["widget"]; } set { mOptionsBag["widget"] = value; } }
    }
}

#pragma warning restore 0618