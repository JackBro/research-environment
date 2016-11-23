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
using System.CodeDom;

namespace System.Extensions.CodeDom
{
	using System.Extensions.CodeDom.Collections;
	using System.Extensions.CodeDom.Statements;

	/// <summary>
	/// Summary description for MethodDeclaration.
	/// </summary>
	public class MethodDeclaration : ImplementationMemberDeclaration
	{
		private StatementCollection body = new StatementCollection();
		private MethodSignature signature = new MethodSignature();

		internal MethodDeclaration(string name, Declaration declaringType)
			:base(name,declaringType)
		{
			this.Attributes = MemberAttributes.Public;
		}

		public MethodSignature Signature
		{
			get
			{
				return this.signature;
			}
		}

		public StatementCollection Body
		{
			get
			{
				return this.body;
			}
			set
			{
				this.body = value;
			}
		}


		public override System.CodeDom.CodeTypeMember ToCodeDom()
		{
			CodeMemberMethod m = new CodeMemberMethod();
			// name
			m.Name = this.Name;
			this.signature.ToCodeDom(m);			
			base.ToCodeDom(m);
			this.SetImplementationTypes(m);

			// body
			if (this.body!=null)
			{
				this.body.ToCodeDom(m.Statements);
			}

			return m;
		}
	}
}
