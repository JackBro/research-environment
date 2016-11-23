using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Xml.Serialization;
using System.Reflection;
using System.IO;

namespace Refly.Cons
{


	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class Class1
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			try
			{
				// CodeDom demos
				SimpleDemo.CreateUser();
				SimpleDemo.CreateStronglyTypedCollection();
				SimpleDemo.CreateStronglyTypedDataReader();
				SimpleDemo.CreateStronglyTypedDictionary();

				// xsd wrapper demo
				XsdWrapperDemo.GraphML();
			}
			catch(Exception ex)
			{
				Console.Write(ex.ToString());
			}
		}
	}
}
