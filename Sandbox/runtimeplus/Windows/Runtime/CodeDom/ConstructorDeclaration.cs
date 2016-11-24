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
	/// <summary>
	/// Summary description for ConstructorDeclaration.
	/// </summary>
	public class ConstructorDeclaration : Declaration
	{
		private Declaration declaringType;
		private MethodSignature signature = new MethodSignature();
		private StatementCollection body = new StatementCollection();

		internal ConstructorDeclaration(Declaration declaringType)
			:base("",declaringType.Conformer)
		{
			this.declaringType = declaringType;
			this.Attributes = MemberAttributes.Public;
		}

		public MethodSignature Signature
		{
			get
			{
				return this.signature;
			}
		}

		public CodeConstructor ToCodeDom()
		{
			CodeConstructor ctr = new CodeConstructor();

			ctr.Name = this.Name;
			base.ToCodeDom(ctr);

			this.Signature.ToCodeDom(ctr);

			// base call

			// body
			if (this.body!=null)
			{
				this.body.ToCodeDom(ctr.Statements);
			}

			return ctr;
		}

		public StatementCollection Body
		{
			get
			{
				return this.body;
			}
		}
	}
}
