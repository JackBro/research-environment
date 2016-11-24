/* System.Extensions License
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
///3. This notice may not be removed or altered from any source distribution.
*/

using System;
using System.CodeDom;
using System.IO;

namespace System.Extensions.CodeDom
{
	public class ParameterDeclaration
	{
        private readonly string name;
        private readonly ITypeDeclaration type;
        private StringWriter description = new StringWriter();
		private bool nonNull;

		internal ParameterDeclaration(ITypeDeclaration type, string name, bool nonNull)
		{
			if (type==null)
				throw new ArgumentNullException(nameof(type));
			if (name==null)
				throw new ArgumentNullException(nameof(name));
			this.type = type;
			this.name=name;
			this.nonNull = nonNull;
		}

		public String Name
		{
			get
			{
				return this.name;
			}
		}

		public StringWriter Description
		{
			get
			{
				return this.description;
			}
		}

		public bool IsNonNull
		{
			get
			{
				return this.nonNull;
			}
		}

		public ITypeDeclaration Type
		{
			get
			{
				return this.type;
			}
		}

		public CodeParameterDeclarationExpression ToCodeDom()
		{
			var p = new CodeParameterDeclarationExpression(
				this.type.TypeReference, 
				this.name
				);
			return p;
		}
	}
}
