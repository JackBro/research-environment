/* System.Extensions License
/// 
/// Copyright (c) 2004 Jonathan de Halleux, http://www.dotnetwiki.org
/// Copyright (c) 2016 Jonathan Moore
///
/// This software is provided 'as-is', without any express or implied warranty. 
/// In no event will the authors be held liable for any damages arising from 
/// the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose, 
/// including commercial applications, and to alter it and redistribute it 
/// freely, subject to the following restrictions:
///
/// 1. The origin of this software must not be misrepresented; 
/// you must not claim that you wrote the original software. 
/// If you use this software in a product, an acknowledgment in the product 
/// documentation would be appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, 
/// and must not be misrepresented as being the original software.
///
///3. This notice may not be removed or altered from any source distribution.
*/
using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.IO;
using System.Reflection;
using Microsoft.CSharp;
using Microsoft.VisualBasic;
using System.Collections;

namespace System.Extensions.CodeDom
{
	/// <summary>
	/// Summary description for CodeGenerator.
	/// </summary>
	public class CodeGenerator
	{
        string tab = "    ";
        CodeDomProvider provider = new CSharpCodeProvider();
        readonly CodeGeneratorOptions options = new CodeGeneratorOptions();

        public string Tab
		{
			get
			{
				return this.tab;
			}
			set
			{
				this.tab = value;
			}
		}

		public CodeDomProvider Provider
		{
			get
			{
				return this.provider;
			}
			set
			{
				if (value==null)
					throw new ArgumentNullException(nameof(value));
				this.provider = value;
			}
		}

		public CodeGeneratorOptions Options
		{
			get
			{
				return this.options;
			}
		}

		public static CodeDomProvider CsProvider
		{
			get
			{
				return new CSharpCodeProvider();
			}
		}

		public static CodeDomProvider VbProvider
		{
			get
			{
				return new VBCodeProvider();
			}
		}

		private string PathFromNamespace(string outputPath, string ns)
		{
			var path =String.Format("{0}\\{1}",
				outputPath,
				ns.Replace('.','\\')
				).Replace('/','\\');
			Directory.CreateDirectory(path);
			return path;
		}

		public void GenerateCode(string outputPath, NamespaceDeclaration ns, string filename)
		{
			if (ns==null)
				throw new ArgumentNullException(nameof(ns));

			// Obtain an ICodeGenerator from a CodeDomProvider class.
			var gen = this.provider.CreateGenerator(filename);
			

			foreach(DictionaryEntry de in ns.ToCodeDom())
			{
				var key = (FileName)de.Key;
				var nms = (CodeNamespace)de.Value;

				// creating directory
				var path = PathFromNamespace(outputPath,key.Namespace);

				using(StreamWriter writer = new StreamWriter(path + "\\" + key.Name + "." + this.Provider.FileExtension))
				{
					// Create a TextWriter to a StreamWriter to an output file.
					var tw = new IndentedTextWriter(writer, tab);
					// Generate source code using the code generator.
					gen.GenerateCodeFromNamespace(nms, tw, options);
				}
			}
		}
	}
}
