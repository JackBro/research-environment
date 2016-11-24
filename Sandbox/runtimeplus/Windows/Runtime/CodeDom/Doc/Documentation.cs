/// System.Extensions License
/// 
/// Copyright (c) 2004 Jonathan de Halleux, http://www.dotnetwiki.org
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

using System;
using System.Xml;
using System.CodeDom;

namespace System.Extensions.CodeDom.Doc
{
	using System.Extensions.CodeDom.Collections;

	public class Documentation
	{
		private XmlMarkup doc = new XmlMarkup("doc");
		private XmlMarkup summary;
		private XmlMarkup remarks;

		public Documentation()
		{
			this.summary = this.doc.Add("summary");
			this.remarks = this.doc.Add("remarks");
		}

		public XmlMarkup Summary
		{
			get
			{
				return this.summary;
			}
		}

		public XmlMarkup Remarks
		{
			get
			{
				return this.remarks;
			}
		}

		public void AddException(Type t, string description)
		{
			XmlMarkup  m =this.doc.Add("exception",TypeFinder.CrossRef(description));
			m.AddAttribute("cref",t.FullName);
		}

		public void AddException(ThrowedExceptionDeclaration ex)
		{
			AddException(ex.ExceptionType,ex.Description);
		}

		public XmlMarkup AddParam(ParameterDeclaration para)
		{
			XmlMarkup m = this.doc.Add("param");
			m.AddAttribute("name",para.Name);
			return m;
		}

		public void AddInclude(string file, string path)
		{
			XmlMarkup m = this.doc.Add("include");
			m.AddAttribute("file",file);
			m.AddAttribute("path",path);
		}

		public void ToCodeDom(CodeCommentStatementCollection comments)
		{
			// comments
			foreach(XmlElement el in this.doc.Root.ChildNodes)
				comments.Add(new CodeCommentStatement(el.OuterXml,true));
		}
	}
}
