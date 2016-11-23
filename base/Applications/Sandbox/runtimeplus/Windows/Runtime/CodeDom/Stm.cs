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
	using System.Extensions.CodeDom.Collections;
	using System.Extensions.CodeDom.Statements;

	/// <summary>
	/// A bunch of static methods around the CodeDom class constructors.
	/// </summary>
	public sealed class Stm
	{
		private Stm(){}

		static public AssignStatement Assign(Expression left, Expression right)
		{
			return new AssignStatement(left,right);
		}

		static public ExpressionStatement ToStm(Expression expr)
		{
			return new ExpressionStatement(expr);
		}

		static public MethodReturnStatement Return(Expression expr)
		{
			return new MethodReturnStatement(expr);
		}

		static public MethodReturnStatement Return()
		{
			return new MethodReturnStatement();
		}

		static public NativeStatement Comment(string comment)
		{
			return new NativeStatement( 
				new CodeCommentStatement(comment,false)
				);
		}

		static public ConditionStatement If(Expression condition, params Statement[] trueStatements)
		{
			return new ConditionStatement(condition,trueStatements);
		}

		static public ConditionStatement IfIdentity(Expression left, Expression right, params Statement[] trueStatements)
		{
			return If(
				left.Identity(right),
				trueStatements
				);
		}

		static public ConditionStatement ThrowIfNull(Expression condition, Expression toThrow)
		{
			return IfNull(condition,
				Stm.Throw(toThrow)
				);
		}

		static public ConditionStatement ThrowIfNull(Expression condition, string message)
		{
			return ThrowIfNull(
				condition,
				Expr.New(typeof(NullReferenceException),Expr.Prim(message))
				);
		}

		static public ConditionStatement ThrowIfNull(ParameterDeclaration param)
		{
			return ThrowIfNull(
				Expr.Arg(param),
				Expr.New(typeof(ArgumentNullException),Expr.Prim(param.Name))
				);
		}

		static public ConditionStatement IfNotIdentity(Expression left, Expression right, params Statement[] trueStatements)
		{
			return If(
				left.NotIdentity(right),
				trueStatements
				);
		}

		static public ConditionStatement IfNull(Expression left,params Statement[] trueStatements)
		{
			return IfIdentity(
				left,Expr.Null,
				trueStatements
				);
		}

		static public ConditionStatement IfNotNull(Expression left,params Statement[] trueStatements)
		{
			return IfNotIdentity(
				left,Expr.Null,
				trueStatements
				);
		}

		static public NativeStatement Snippet(string snippet)
		{
			return new NativeStatement(
				new CodeSnippetStatement(snippet)
				);
		}

		static public ThrowExceptionStatement Throw(Expression toThrow)
		{
			return new ThrowExceptionStatement(toThrow);
		}

		static public ThrowExceptionStatement Throw(Type t, params Expression[] args)
		{
			return Throw(new TypeTypeDeclaration(t),args);
		}

		static public ThrowExceptionStatement Throw(ITypeDeclaration t, params Expression[] args)
		{
			return new ThrowExceptionStatement(
					Expr.New(t,args)
				);
		}

		static public TryCatchFinallyStatement Try()
		{
			return new TryCatchFinallyStatement();
		}

		static public CatchClause Catch(ParameterDeclaration localParam)
		{
			return new CatchClause(localParam);
		}

		static public CatchClause Catch(String t,  string name)
		{
			return Catch(new StringTypeDeclaration(t),name);
		}

		static public CatchClause Catch(Type t,  string name)
		{
			return Catch(new TypeTypeDeclaration(t),name);
		}

		static public CatchClause Catch(ITypeDeclaration t,  string name)
		{
			return Catch(new ParameterDeclaration(t,name,false));
		}

		static public VariableDeclarationStatement Var(String type, string name)
		{
			return Var(new StringTypeDeclaration(type),name);
		}

		static public VariableDeclarationStatement Var(Type type, string name)
		{
			return Var(new TypeTypeDeclaration(type),name);
		}

		static public VariableDeclarationStatement Var(ITypeDeclaration type, string name)
		{
			return new VariableDeclarationStatement(type,name);
		}

		static public VariableDeclarationStatement Var(String type, string name,Expression initExpression)
		{
			return Var(new StringTypeDeclaration(type),name,initExpression);
		}

		static public VariableDeclarationStatement Var(Type type, string name,Expression initExpression)
		{
			return Var(new TypeTypeDeclaration(type),name,initExpression);
		}

		static public VariableDeclarationStatement Var(ITypeDeclaration type, string name,Expression initExpression)
		{
			VariableDeclarationStatement var =  Var(type,name);
			var.InitExpression = initExpression;
			return var;
		}

		static public IterationStatement For(
			Statement initStatement, Expression testExpression, Statement incrementStatement
			)
		{
			return new IterationStatement(initStatement,testExpression,incrementStatement);
		}

		static public IterationStatement While(
			Expression testExpression
			)
		{
			return new IterationStatement(
				new SnippetStatement(""),
				testExpression,
				new SnippetStatement("")
				);
		}

		public static ForEachStatement ForEach( 
			ITypeDeclaration localType,
			string localName,
			Expression collection,
			bool enumeratorDisposable
			)
		{
			return new ForEachStatement(localType,localName,collection
				,enumeratorDisposable
				);
		}
	}
}
