using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Build.Framework;
using log4net;
using System.Threading;
using log4net.Config;
using System.IO;
using Microsoft.Build.BuildEngine;

namespace Examples.Loggers
{
    /// <summary>
    /// Logger that can be used to re-direct logging
    /// to <c>log4net</c>.
    /// </summary>
    public class SedoLogger : ConsoleLogger
    {
        #region Fields
        private ILog logger = LogManager.GetLogger(typeof(SedoLogger));

        //private string log4netConfigFile;
        /// <summary>
        /// Map that will contain all the parameters.
        /// </summary>
        private IDictionary<string, string> _paramaterBag;
        /// <summary>
        /// Determines is a summary should be displayed or not.
        /// </summary>
        private bool _showSummary;
        #endregion

        #region Properties
        /// <summary>
        /// TODO: Doc this
        /// </summary>
        public string Log4netConfigFile;
        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="SedoLogger"/> class.
        /// </summary>
        public SedoLogger()
            : base()
        {
            //override the default WriteHandler
            base.WriteHandler = new WriteHandler(MessageHandler);
        }
        /// <summary>
        /// Registers the logger for the specified events.
        /// </summary>
        /// <param name="eventSource">The <see cref="T:Microsoft.Build.Framework.IEventSource"/> to register with the logger.</param>
        public override void Initialize(IEventSource eventSource)
        {
            base.Initialize(eventSource);

            try
            {
                InitializeParameters();
            }
            catch (Exception e)
            {
                throw new LoggerException("Unable to initialize the logger", e);
            }

            //currently not being used for anything
            ShowSummary = false;
            InitializeLog4net();
        }
        /// <summary>
        /// This will read in the parameters and process them.
        /// The parameter string should look like: paramName1=val1;paramName2=val2;paramName3=val3
        /// This method will also cause the known parameter properties of this class to be set if they
        /// are present.
        /// </summary>
        protected void InitializeParameters()
        {
            try
            {
                this._paramaterBag = new Dictionary<string, string>();
                if (this.Parameters == null || this.Parameters.Length <= 0)
                {
                    return;
                }
                foreach (string paramString in this.Parameters.Split(";".ToCharArray()))
                {
                    string[] keyValue = paramString.Split("=".ToCharArray());
                    if (keyValue == null || keyValue.Length < 2)
                    {
                        continue;
                    }
                    this.ProcessParam(keyValue[0].ToLower(), keyValue[1]);
                }
            }
            catch (Exception e)
            {
                throw new LoggerException("Unable to initialize parameterss; message=" + e.Message, e);
            }
        }
        /// <summary>
        /// Method that will process the parameter value. If either <code>name</code> or
        /// <code>value</code> is empty then this parameter will not be processed.
        /// </summary>
        /// <param name="name">name of the paramater</param>
        /// <param name="value">value of the parameter</param>
        protected void ProcessParam(string name, string value)
        {
            try
            {
                if (name == null || name.Length <= 0 ||
                    value == null || value.Length <= 0)
                {
                    return;
                }

                //add to param bag so subclasses have easy method to fetch other parameter values
                AddToParameters(name, value);
                switch (name.Trim().ToUpper())
                {
                    //case ("LOGFILE"):
                    //case ("L"):
                    //    this._fileName = value;
                    //    break;

                    case ("VERBOSITY"):
                    case ("V"):
                        ProcessVerbosity(value);
                        break;

                    //case ("APPEND"):
                    //case ("A"):
                    //    ProcessAppend(value);
                    //    break;

                    case ("SHOWSUMMARY"):
                    case ("S"):
                        ProcessShowSummary(value);
                        break;
                }
            }
            catch (LoggerException /*le*/)
            {
                throw;
            }
            catch (Exception e)
            {
                string message = "Unable to process parameters;[name=" + name + ",value=" + value + " message=" + e.Message;
                throw new LoggerException(message, e);
            }
        }
        /// <summary>
        /// This will set the verbosity level from the parameter
        /// </summary>
        /// <param name="level"></param>
        protected void ProcessVerbosity(string level)
        {
            if (level == null || level.Trim().Length <= 0)
            {
                return;
            }
            switch (level.Trim().ToUpper())
            {
                case ("QUIET"):
                case ("Q"):
                    base.Verbosity = LoggerVerbosity.Quiet;
                    break;

                case ("MINIMAL"):
                case ("M"):
                    base.Verbosity = LoggerVerbosity.Minimal;
                    break;

                case ("NORMAL"):
                case ("N"):
                    base.Verbosity = LoggerVerbosity.Normal;
                    break;

                case ("DETAILED"):
                case ("D"):
                    base.Verbosity = LoggerVerbosity.Detailed;
                    break;

                case ("DIAGNOSTIC"):
                case ("DIAG"):
                    base.Verbosity = LoggerVerbosity.Diagnostic;
                    break;

                default:
                    throw new LoggerException("Unable to process the verbosity: " + level);
            }
        }
        /// <summary>
        /// This will process the show summary value based on the parameter.
        /// NOTE: In this implementation a <code>LoggerException</code> is thrown if the string
        /// cause an exception to be raised when <code>bool.Parse()</code> is called on it. If you require a
        /// different behavior simply override this method.
        /// </summary>
        /// <param name="summaryStr"></param>
        protected void ProcessShowSummary(string summaryStr)
        {
            try
            {
                this._showSummary = bool.Parse(summaryStr);
            }
            catch (Exception e)
            {
                throw new LoggerException("Unable to process summary parameter [" + summaryStr + "]", e);
            }
        }
        /// <summary>
        /// Adds the given name and value to the <code>_parameterBag</code>.
        /// If the bag already contains the name as a key, this value will replace the previous value.
        /// </summary>
        /// <param name="name">name of the parameter</param>
        /// <param name="value">value for the paramter</param>
        private void AddToParameters(string name, string value)
        {
            try
            {
                if (_paramaterBag.ContainsKey(name))
                {
                    _paramaterBag[name] = value;
                }
                else
                {
                    _paramaterBag.Add(name, value);
                }
            }
            catch (Exception e)
            {
                throw new LoggerException("Unable to add to parameters bag", e);
            }
        }
        /// <summary>
        /// Initializes the log4net.
        /// </summary>
        protected void InitializeLog4net()
        {
            string log4netConfigLocation = GetParameterValue(Log4netLoggerConsts.Log4netConfigFileSwitch);
            if (string.IsNullOrEmpty(log4netConfigLocation))
            {
                log4netConfigLocation = Log4netLoggerConsts.DefaultConfigFileName;
            }

            Log4netConfigFile = log4netConfigLocation;
            if (!string.IsNullOrEmpty(Log4netConfigFile))
            {
                FileInfo configFile = new FileInfo(Log4netConfigFile);
                if (configFile.Exists)
                {
                    XmlConfigurator.Configure(configFile);
                }
            }
        }
        /// <summary>
        /// This can be used to get the values of parameter that this class is not aware of.
        /// If the value is not present then string.Empty is returned.
        /// </summary>
        /// <param name="name">name of the parameter to fetch</param>
        /// <returns></returns>
        protected string GetParameterValue(string name)
        {
            string lowerName = name.ToLower();
            if (this._paramaterBag.ContainsKey(lowerName))
            {
                return this._paramaterBag[lowerName];
            }
            else
            {
                return string.Empty;
            }
        }
        #region Methods
        private void MessageHandler(string message)
        {
            this.logger.Info(message);
        }
        #endregion
    }
    static class Log4netLoggerConsts
    {
        public const string Log4netConfigFileSwitch = "log4netConfigFile";
        public const string DefaultConfigFileName = "log4net.msbuild.xml";
    }
}
