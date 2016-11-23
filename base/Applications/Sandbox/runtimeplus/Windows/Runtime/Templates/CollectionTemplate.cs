/// System.Extensions License
/// 
/// Copyright (c) 2004 Jonathan de Halleux, http://www.dotnetwiki.org
///
/// This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
///
/// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
///
///3. This notice may not be removed or altered from any source distribution.

using System;
using System.ComponentModel;
using System.Collections;
using System.Xml.Serialization;
using System.CodeDom;

namespace System.Extensions.Templates
{
	using System.Extensions.CodeDom;
	using System.Extensions.CodeDom.Statements;

	/// <summary>
	/// Summary description for StronglyTypedCollectionGenerator.
	/// </summary>
	public class CollectionTemplate : ITemplate
	{
		[Category("Data")]
		public String ElementType = null;
		[Category("Data")]
		public bool ItemGet = true;
		[Category("Data")]
		public bool ItemSet = true;
		[Category("Data")]
		public bool Add = true;
		[Category("Data")]
		public bool AddRange = true;
		[Category("Data")]
		public bool Contains = true;
		[Category("Data")]
		public bool Remove = true;
		[Category("Data")]
		public bool Insert = true;
		[Category("Data")]
		public bool IndexOf = true;
		[Category("Enumerator")]
		public bool Enumerator = true;

		public CollectionTemplate()
		{}

		public ITypeDeclaration Type
		{
			get
			{
				if (this.ElementType==null)
					throw new InvalidOperationException("Element type is undifned");
				return new StringTypeDeclaration(this.ElementType);
			}
		}

		public ClassDeclaration AddClass(NamespaceDeclaration ns)
		{
			ClassDeclaration col = ns.AddClass(this.Name);

			// set base class as CollectionBase
			col.Parent = new TypeTypeDeclaration(typeof(CollectionBase));

			// default constructor
			col.AddConstructor();

			// add indexer
			if (this.ItemGet || this.ItemSet)
			{
				IndexerDeclaration index = col.AddIndexer(
					this.Type
					);
				ParameterDeclaration pindex = index.Signature.AddParam(typeof(int),"index",false);

				// get body
				if (this.ItemGet)
				{
					index.Get.Return(
						(Expr.This.Prop("List").Item(Expr.Arg(pindex)).Cast(this.Type)
						)
						);
				}
				// set body
				if (this.ItemSet)
				{
					index.Set.Add(
						Stm.Assign(
						Expr.This.Prop("List").Item(Expr.Arg(pindex)),
						Expr.Value
						)
						);
				}

			}
			
			string pname = ns.Conformer.ToCamel(this.Type.Name);
			// add method
			if (this.Add)
			{
				MethodDeclaration add = col.AddMethod("Add");
				ParameterDeclaration para = add.Signature.AddParam(this.Type,pname,true);
				add.Body.Add(
					Expr.This.Prop("List").Method("Add").Invoke(para)
					);
			}

			if (this.AddRange)
			{
				MethodDeclaration add = col.AddMethod("AddRange");
				ParameterDeclaration para = add.Signature.AddParam(col,pname,true);

				ForEachStatement fe = Stm.ForEach(
					this.Type,
					"item",
					Expr.Arg(para),
					false
					);
				fe.Body.Add(
					Expr.This.Prop("List").Method("Add").Invoke( fe.Local)
					);

				add.Body.Add(fe);
			}

			// contains method
			if (this.Contains)
			{
				MethodDeclaration contains = col.AddMethod("Contains");
				contains.Signature.ReturnType = new TypeTypeDeclaration(typeof(bool));
				ParameterDeclaration para = contains.Signature.AddParam(this.Type,pname,true);
				contains.Body.Return(
					Expr.This.Prop("List").Method("Contains").Invoke(para)
					);
			}

			// remove method
			if (this.Remove)
			{
				MethodDeclaration remove = col.AddMethod("Remove");
				ParameterDeclaration para = remove.Signature.AddParam(this.Type,pname,true);

				remove.Doc.Summary.AddText("Removes the first occurrence of a specific ParameterDeclaration from this ParameterDeclarationCollection.");

				remove.Body.Add(
					Expr.This.Prop("List").Method("Remove").Invoke(para)
					);
			}

			// insert
			if (this.Insert)
			{
				MethodDeclaration insert = col.AddMethod("Insert");
				ParameterDeclaration index = insert.Signature.AddParam(typeof(int),"index",true);
				ParameterDeclaration para = insert.Signature.AddParam(this.Type,pname,true);
				insert.Body.Add(
					Expr.This.Prop("List").Method("Insert").Invoke(index,para)
					);
			}

			// indexof
			if (this.IndexOf)
			{
				MethodDeclaration indexof = col.AddMethod("IndexOf");
				ParameterDeclaration para = indexof.Signature.AddParam(this.Type,pname,true);
				indexof.Signature.ReturnType = new TypeTypeDeclaration(typeof(int));
				indexof.Body.Return(
					Expr.This.Prop("List").Method("IndexOf").Invoke(para)
					);
			}

			if (this.Enumerator)
			{
				// create subclass
				ClassDeclaration en = col.AddClass("Enumerator");
				// add wrapped field
				FieldDeclaration wrapped = en.AddField(
					typeof(IEnumerator),"wrapped"
					);
				// add IEnumerator
				en.Interfaces.Add(typeof(IEnumerator));

				// add constructor
				ConstructorDeclaration cs = en.AddConstructor();
				ParameterDeclaration collection = cs.Signature.AddParam(col,"collection",true);
				cs.Body.Add(
					Stm.Assign(
						Expr.This.Field(wrapped),
						Expr.Arg(collection).Cast(typeof(CollectionBase)).Method("GetEnumerator").Invoke()
						)
					);

				// add current 
				PropertyDeclaration current = en.AddProperty(this.Type,"Current");
				current.Get.Return(
					(Expr.This.Field(wrapped).Prop("Current")).Cast(this.Type)
					);

				// add explicit interface implementation
				PropertyDeclaration currentEn = en.AddProperty(typeof(Object),"Current");
				currentEn.Get.Return(Expr.This.Prop(current));
				currentEn.PrivateImplementationType = wrapped.Type;

				// add reset
				MethodDeclaration reset = en.AddMethod("Reset");
				reset.ImplementationTypes.Add( wrapped.Type );
				reset.Body.Add( Expr.This.Field(wrapped).Method("Reset").Invoke());

				// add movenext
				MethodDeclaration movenext = en.AddMethod("MoveNext");
				movenext.ImplementationTypes.Add( wrapped.Type );
				movenext.Signature.ReturnType = new TypeTypeDeclaration(typeof(bool));
				movenext.Body.Return( Expr.This.Field(wrapped).Method("MoveNext").Invoke());

				// add get enuemrator
				MethodDeclaration geten = col.AddMethod("GetEnumerator");
				geten.Attributes |= MemberAttributes.New;
				geten.ImplementationTypes.Add( new TypeTypeDeclaration(typeof(IEnumerable)) );
				geten.Signature.ReturnType = en;
				geten.Body.Return(Expr.New(en,Expr.This));
			}

			return col;
		}

		protected string Name
		{
			get
			{
				return this.Type.Name + "Collection";
			}
		}

		#region ITemplate Members

		public void Generate(NamespaceDeclaration ns)
		{
			AddClass(ns);
		}

		#endregion
	}
}
