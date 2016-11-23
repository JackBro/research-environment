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
	using System.Extensions.CodeDom.Expressions;
	using System.Extensions.CodeDom.Statements;

	/// <summary>
	/// Summary description for Expr.
	/// </summary>
	public sealed class Expr
	{
		private Expr()
		{}

		public static ThisReferenceExpression This
		{
			get
			{
				return new ThisReferenceExpression();
			}
		}

		public static BaseReferenceExpression Base
		{
			get
			{
				return new BaseReferenceExpression();
			}
		}

		public static PropertySetValueReferenceExpression Value
		{
			get
			{
				return new PropertySetValueReferenceExpression();
			}
		}

		public static NativeExpression Null
		{
			get
			{
				return new NativeExpression(
						new CodePrimitiveExpression(null)
					);
			}
		}

		public static PrimitiveExpression True
		{
			get
			{
				return Prim(true);
			}
		}

		public static PrimitiveExpression False
		{
			get
			{
				return Prim(false);
			}
		}

		public static ObjectCreationExpression New(
			Type t,
			params Expression[] parameters)
		{
			return New(new TypeTypeDeclaration(t),parameters);
		}

		public static ObjectCreationExpression New(
			ITypeDeclaration type,
			params Expression[] parameters)
		{
			return new ObjectCreationExpression(type,parameters);
		}

		public static ArgumentReferenceExpression Arg(
			ParameterDeclaration p)
		{
			return new ArgumentReferenceExpression(p);
		}

		public static VariableReferenceExpression Var(
			VariableDeclarationStatement v)
		{
			return new VariableReferenceExpression(v);
		}

		public static PrimitiveExpression Prim(bool value)
		{
			return new PrimitiveExpression(value);
		}
		public static PrimitiveExpression Prim(string value)
		{
			return new PrimitiveExpression(value);
		}
		public static PrimitiveExpression Prim(int value)
		{
			return new PrimitiveExpression(value);
		}
		public static PrimitiveExpression Prim(long value)
		{
			return new PrimitiveExpression(value);
		}
		public static PrimitiveExpression Prim(decimal value)
		{
			return new PrimitiveExpression(value);
		}
		public static PrimitiveExpression Prim(double value)
		{
			return new PrimitiveExpression(value);
		}
		public static PrimitiveExpression Prim(short value)
		{
			return new PrimitiveExpression(value);
		}

		public static PrimitiveExpression Prim(byte value)
		{
			return new PrimitiveExpression(value);
		}

		public static PrimitiveExpression Prim(DateTime value)
		{
			return new PrimitiveExpression(value);
		}

		public static CastExpression Cast(Type t, Expression e)
		{
			return Cast(new TypeTypeDeclaration(t),e);
		}

		public static CastExpression Cast(ITypeDeclaration t, Expression e)
		{
			return new CastExpression(t,e);
		}

		public static NativeExpression Snippet(string snippet)
		{
			return new NativeExpression(
				new CodeSnippetExpression(snippet)
				);
		}

		public static NativeExpression TypeOf(Type t)
		{
			return TypeOf(new TypeTypeDeclaration(t));
		}

		public static NativeExpression TypeOf(ITypeDeclaration t)
		{
			return new NativeExpression(
				new CodeTypeOfExpression(t.TypeReference)
				);
		}
	}
}
