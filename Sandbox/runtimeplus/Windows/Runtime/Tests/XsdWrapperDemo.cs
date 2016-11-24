using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Xml.Serialization;
using System.Reflection;
using System.IO;

namespace Refly.Cons
{
	using Refly.CodeDom;
	using Refly.CodeDom.Xsd;	
	
	/// <summary>
	/// Summary description for XsdWrapperDemo.
	/// </summary>
	public class XsdWrapperDemo
	{
		public static void GraphML()
		{
			// XSD wrapper demo
			XsdWrapperGenerator xsg = new XsdWrapperGenerator("QuickGraph.Serialization.GraphML");
			foreach(Type t in Assembly.GetExecutingAssembly().GetExportedTypes())
			{
				if (t.Namespace!= "QuickGraph.Serialization.GraphML")
					continue;
				xsg.Add(t);
			}

			xsg.Generate();

			// getting generator
			CodeGenerator gen = new CodeGenerator();
			gen.GenerateCode("..\\..",xsg.Ns);

		}
	}
}
