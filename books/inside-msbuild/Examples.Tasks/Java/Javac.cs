using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Build.Utilities;
using System.ComponentModel;
using System.IO;
using Microsoft.Build.Framework;

namespace Examples.Tasks.Java
{
    //TODO: Add better support for 'some debuggin info' right now it is all or nothing.
    public class Javac : ToolTask
    {
        #region Fields
        private string javaHome;
        private string javaEnvVarName;
        private ITaskItem[] sourceFiles;

        private bool generateDebugginInfo;
        private bool nowarn;
        private bool verbose;
        private bool deprecation;
        private ITaskItem[] classpath;
        private ITaskItem[] sourcepath;
        private ITaskItem[] bootclasspath;
        private ITaskItem[] extdirs;
        private ITaskItem[] endorseddirs;
        private bool performProcAnnotation;
        private string[] processor;
        private ITaskItem[] processorPath;
        private string outputClassDir;
        private string outputSourceDir;
        private bool @implicit;
        private string encoding;
        private string source;
        private string target;
        private bool showVersion;
        private bool showHelp;
        private string[] annotationOptions;
        private bool printNonstandardOptions;
        private string flag;
        #endregion

        #region Constructors
        public Javac()
            : base()
        {
            javaEnvVarName = "JAVA_HOME";
            generateDebugginInfo = false;
            nowarn = false;
            verbose = false;
            deprecation = false;
            performProcAnnotation = false;
            @implicit = false;

        }
        #endregion

        #region Properties
        /// <summary>
        /// Here you can set the path to the Java SDK that should be used.
        /// If this is not provided then the value from the <c>JavaEnvVarName</c>
        /// environment variable is used, if it exists.
        /// </summary>
        public string JavaHome
        {
            get
            {
                string value = this.javaHome;
                if (value == null)
                {
                    value = Environment.GetEnvironmentVariable(JavaEnvVarName);
                }
                return value;
            }
            set { this.javaHome = value; }
        }
        /// <summary>
        /// Gets the name of the Java Environment variable to be used. This will 
        /// default to <c>'JAVA_HOME'</c>.
        /// </summary>
        [DefaultValue("JAVA_HOME")]
        public string JavaEnvVarName
        {
            get { return this.javaEnvVarName; }
            set { this.javaEnvVarName = value; }
        }
        /// <summary>
        /// List of files to send to the <c>Java</c> compiler.
        /// </summary>
        public ITaskItem[] SourceFiles
        {
            get { return this.sourceFiles; }
            set { this.sourceFiles = value; }
        }

        public bool GenerateDebugginInfo
        {
            get { return this.generateDebugginInfo; }
            set { this.generateDebugginInfo = value; }
        }
        public bool Nowarn
        {
            get { return this.nowarn; }
            set { this.nowarn = value; }
        }
        public bool Verbose
        {
            get { return this.verbose; }
            set { this.verbose = false; }
        }
        public bool Deprecation
        {
            get { return this.deprecation; }
            set { this.deprecation = value; }
        }
        public ITaskItem[] Classpath
        {
            get { return this.classpath; }
            set { this.classpath = value; }
        }
        public ITaskItem[] Sourcepath
        {
            get { return this.sourcepath; }
            set { this.sourcepath = value; }
        }
        public ITaskItem[] Bootclasspath
        {
            get { return this.bootclasspath; }
            set { this.bootclasspath = value; }
        }
        public ITaskItem[] Extdirs
        {
            get { return this.extdirs; }
            set { this.extdirs = value; }
        }
        public ITaskItem[] Endorseddirs
        {
            get { return this.endorseddirs; }
            set { this.endorseddirs = value; }
        }
        public bool PerformProcAnnotation
        {
            get { return this.performProcAnnotation; }
            set { this.performProcAnnotation = value; }
        }
        public ITaskItem[] ProcessorPath
        {
            get { return this.processorPath; }
            set { this.processorPath = value; }
        }
        public string[] Processor
        {
            get { return this.processor; }
            set { this.processor = value; }
        }
        public string OutputClassDir
        {
            get { return this.outputClassDir; }
            set { this.outputClassDir = value; }
        }
        public string OutputSourceDir
        {
            get { return this.outputSourceDir; }
            set { this.outputSourceDir = value; }
        }
        public bool Implicit
        {
            get { return this.@implicit; }
            set { this.@implicit = value; }
        }
        public string Encoding
        {
            get { return this.encoding; }
            set { this.encoding = value; }
        }
        public string Source
        {
            get { return this.source; }
            set { this.source = value; }
        }
        public string Target
        {
            get { return this.target; }
            set { this.target = value; }
        }
        public bool ShowVersion
        {
            get { return this.showVersion; }
            set { this.showVersion = value; }
        }
        public bool ShowHelp
        {
            get { return this.showHelp; }
            set { this.showHelp = value; }
        }
        public string[] AnnotationOptions
        {
            get { return this.annotationOptions; }
            set { this.annotationOptions = value; }
        }
        public bool PrintNonStandardOptions
        {
            get { return this.printNonstandardOptions; }
            set { this.printNonstandardOptions = value; }
        }
        public string Flag
        {
            get { return this.flag; }
            set { this.flag = value; }
        }
        #endregion

        protected override string GenerateFullPathToTool()
        {
            string pathToAppend = JavaHome.EndsWith(@"\") ? @"bin\" : @"\bin\";
            string path = Path.Combine(Path.Combine(JavaHome, pathToAppend), ToolName);
            return path;
        }
        protected override string ToolName
        {
            get { return "javac.exe"; }
        }
        protected override bool ValidateParameters()
        {
            IList<string> errorMessages = new List<string>();
            if (string.IsNullOrEmpty(JavaHome))
            {
                string message = "JavaHome value not found";
                errorMessages.Add(message);
            }
            else
            {
                DirectoryInfo di = new DirectoryInfo(JavaHome);
                if (!di.Exists)
                {
                    string message = string.Format("JavaHome [{0}] doesn't exist.", JavaHome);
                    errorMessages.Add(message);
                }
                FileInfo javacFi = new FileInfo(GenerateFullPathToTool());
                if (!javacFi.Exists)
                {
                    string message = string.Format("javac.exe not found at [{0}].", javacFi.FullName);
                    errorMessages.Add(message);
                }
            }

            if (PerformProcAnnotation)
            {
                if (Processor == null || Processor.Length <= 0)
                {
                    string message = string.Format("When using the {0} attribute values for {1} must be provided.",
                        "PerformProcAnnotation", "Processor");
                    errorMessages.Add(message);
                }
            }


            bool isValid = errorMessages.Count <= 0;
            foreach (string errorMessage in errorMessages)
            {
                base.Log.LogError(errorMessage, null);
            }
            return isValid;
        }
        protected override string GenerateCommandLineCommands()
        {
            CommandLineBuilder clb = new CommandLineBuilder();

            clb.AppendSwitch(GenerateDebugginInfo == true ?
                JavacConsts.GenerateAllDebuggingInfoSwitch :
                JavacConsts.GenerateNoDebugginInfoSwitch);

            if (Nowarn)
            {
                clb.AppendSwitch(JavacConsts.NowarnSwitch);
            }
            if (Deprecation)
            {
                clb.AppendSwitch(JavacConsts.DeprecationSwitch);
            }
            if (Classpath != null && Classpath.Length > 0)
            {
                clb.AppendSwitch(
                    GetSwitchStringForItemGroup(JavacConsts.ClasspathSwitch,
                    Classpath, ";", true));
            }
            if (Sourcepath != null && Sourcepath.Length > 0)
            {
                clb.AppendSwitch(
                    GetSwitchStringForItemGroup(JavacConsts.ClasspathSwitch,
                    Sourcepath, ";", true));
            }
            if (Bootclasspath != null && Bootclasspath.Length > 0)
            {
                clb.AppendSwitch(
                    GetSwitchStringForItemGroup(JavacConsts.ClasspathSwitch,
                    Bootclasspath, ";", true));
            }
            if (Extdirs != null && Extdirs.Length > 0)
            {
                clb.AppendSwitch(
                    GetSwitchStringForItemGroup(JavacConsts.ClasspathSwitch,
                    Extdirs, ";", true));
            }

            clb.AppendSwitch(PerformProcAnnotation == true ? 
                    JavacConsts.PerformProcAnnotationOnlySwitch :
                    JavacConsts.PerformProcAnnotationNoneSwitch);

            if (Processor != null && Processor.Length > 0)
            {
                StringBuilder sb = new StringBuilder();
                sb.Append(JavacConsts.ProcessorSwitch);
                sb.Append(" ");
                for(int i=0;i<Processor.Length;i++)
                {
                    if (i > 0)
                    { sb.Append(","); }

                    sb.Append(Processor[i]);
                }
            }
            if (ProcessorPath != null && ProcessorPath.Length > 0)
            {
                clb.AppendSwitch(
                    GetSwitchStringForItemGroup(JavacConsts.ClasspathSwitch,
                    ProcessorPath, ";", true));
            }

            if (OutputClassDir != null)
            {
                clb.AppendSwitch(string.Format("{0} {1}",
                    JavacConsts.OutputClassDirSwtich,OutputClassDir));
            }
            if (OutputSourceDir != null)
            {
                clb.AppendSwitch(string.Format("{0} {1}",
                    JavacConsts.OutputSourceDirSwitch,OutputSourceDir));
            }

            return "-help";
        }
        protected string GetSwitchStringForItemGroup(string switchStr, ITaskItem[] itemGroup, 
            string delimiter, bool quote)
        {
            if (switchStr == null) { throw new ArgumentNullException("switchStr"); }
            if (itemGroup == null) { throw new ArgumentNullException("itemGroup"); }
            if (delimiter == null) { throw new ArgumentNullException("delimter"); }

            StringBuilder sb = new StringBuilder();
            sb.Append(switchStr);
            sb.Append(" ");
            
            for (int i = 0; i < itemGroup.Length; i++)
            {
                if (i > 0)
                { sb.Append(delimiter); }

                ITaskItem item = itemGroup[i];
                string formatStr = quote == true ? "\"{0}\"" : "{0}";
                sb.Append(string.Format(formatStr,
                    item.GetMetadata(JavacConsts.FullPathMetadataName)));
            }
            return sb.ToString();
        }
    }
    class JavacConsts
    {
        public const string GenerateAllDebuggingInfoSwitch = "-g";
        public const string GenerateNoDebugginInfoSwitch = "-g:none";
        public const string NowarnSwitch = "-nowarn";
        public const string VerboseSwitch = "-verbose";
        public const string DeprecationSwitch = "-deprecation";
        public const string ClasspathSwitch = "-classpath";
        public const string SourcepathSwitch = "-sourcepath";
        public const string BootclasspathSwitch = "-bootclasspath";
        public const string ExtdirsSwitch = "-extdirs";
        public const string EndorseddirsSwitch = "-endorseddirs";
        public const string PerformProcAnnotationOnlySwitch = "-proc:only";
        public const string PerformProcAnnotationNoneSwitch = "-proc:none";
        public const string ProcessorSwitch = "-processor";
        public const string FullPathMetadataName = "FullPath";
        public const string OutputClassDirSwtich = "-d";
        public const string OutputSourceDirSwitch = "-s";
        public const string ImplicitReferenceSwitch = "-implicit:class";
        public const string EncodingSwitch = "-encoding";
        public const string SourceSwitch = "-source";
        public const string TargetSwitch = "-target";
        public const string VersionSwitch = "-version";
        public const string HelpSwitch = "-help";
        public const string AnnotationOptionsSwitch = "-AKey";
        public const string PrintNonstandardOptionsSwitch = "-X";
        public const string FlagSwitch = "-J";

    }
}
