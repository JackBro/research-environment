/* System.Extensions License
/// 
/// Copyright (c) 2004 Jonathan de Halleux, http://www.dotnetwiki.org
///
/// Copyright 2016 Jonathan Moore
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
*/

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Threading;
using System.Reflection;
using System.CodeDom;
using System.Xml.Serialization;

namespace System.Extensions.CodeDom.Xsd
{
	using System.Extensions.CodeDom.Collections;
	using System.Extensions.CodeDom.Expressions;
	using System.Extensions.CodeDom.Statements;

	/// <summary>
	/// Transforms the XSD wrapper classes outputed by the Xsd.exe utility
	/// to nicer classes using Reflection.
	/// </summary>
	/// <remarks>
	/// </remarks>
	public class XsdWrapperGenerator
	{
		private NamespaceDeclaration ns;
		private NameConformer conformer;
        readonly TypeTypeDeclarationDictionary types = new TypeTypeDeclarationDictionary();
        private XsdWrapperConfig config = new XsdWrapperConfig();

		/// <summary>
		/// Creates a new wrapper generator.
		/// </summary>
		/// <param name="name"></param>
		public XsdWrapperGenerator(string name)
		{
			if (name==null)
				throw new ArgumentNullException(nameof(name));

			this.conformer = new NameConformer();
			this.ns = new NamespaceDeclaration(name,this.conformer);
			this.ns.AddImport("System.Xml");
			this.ns.AddImport("System.Xml.Serialization");
		}

		public NamespaceDeclaration Ns
		{
			get
			{
				return this.ns;
			}
		}

		public TypeTypeDeclarationDictionary Types
		{
			get
			{
				return this.types;
			}
		}

		public XsdWrapperConfig Config
		{
			get
			{
				return this.config;
			}
			set
			{
				this.config = value;
			}
		}

		/// <summary>
		/// Adds a class to the wrapped class list
		/// </summary>
		/// <param name="t"></param>
		public void Add(Type t)
		{
			if (t==null)
				throw new ArgumentNullException(nameof(t));
			if (t.IsEnum)
				CreateEnum(t);
			else if (t.IsClass)
				CreateClass(t);
		}

		public void Generate()
		{
			foreach(Type t in this.types.Keys)
			{
				if (t.IsEnum)
					GenerateEnumType(t);
				else if (t.IsClass)
					GenerateClassType(t);
			}
		}

		private void CreateClass(Type t)
		{
			var c = this.ns.AddClass(this.conformer.ToCapitalized(t.Name));

			// check if XmlType present
			if(TypeHelper.HasCustomAttribute(t,typeof(XmlTypeAttribute)))
			{
				var typeAttr = 
					(XmlTypeAttribute)TypeHelper.GetFirstCustomAttribute(t,typeof(XmlTypeAttribute));
				var type =new CodeAttributeDeclaration(
					"XmlType"
					);
				type.Arguments.Add(
					new CodeAttributeArgument("IncludeInSchema",
						new CodePrimitiveExpression(typeAttr.IncludeInSchema))
					);
				if (this.Config.KeepNamespaces)
				{
					type.Arguments.Add(
						new CodeAttributeArgument("Namespace",
						new CodePrimitiveExpression(typeAttr.Namespace))
						);
				}
				type.Arguments.Add(
					new CodeAttributeArgument("TypeName",
					new CodePrimitiveExpression(typeAttr.TypeName)
					)
					);
				
				c.CustomAttributes.Add(type);
			}

			// check if XmlRoot present
			if(TypeHelper.HasCustomAttribute(t,typeof(XmlRootAttribute)) )
			{
				var rootAttr = 
					(XmlRootAttribute)TypeHelper.GetFirstCustomAttribute(t,typeof(XmlRootAttribute));
				var root =new CodeAttributeDeclaration(
					"XmlRoot"
					);
				root.Arguments.Add(
					new CodeAttributeArgument("ElementName",
					new CodePrimitiveExpression(rootAttr.ElementName)
					)
					);
				root.Arguments.Add(
					new CodeAttributeArgument("IsNullable",
					new CodePrimitiveExpression(rootAttr.IsNullable)
					)
					);
				if (this.Config.KeepNamespaces)
				{
					root.Arguments.Add(
						new CodeAttributeArgument("Namespace",
						new CodePrimitiveExpression(rootAttr.Namespace)
						)
						);
				}
				root.Arguments.Add(
					new CodeAttributeArgument("DataType",
					new CodePrimitiveExpression(rootAttr.DataType)
					)
					);
				
				c.CustomAttributes.Add(root);
			}
			else
			{
				var root =new CodeAttributeDeclaration(
					"XmlRoot"
					);
				root.Arguments.Add(
					new CodeAttributeArgument("ElementName",new CodePrimitiveExpression(t.Name))
					);
				c.CustomAttributes.Add(root);
			}
			this.types.Add(t,c);
		}

		private void CreateEnum(Type t)
		{
			var flags = TypeHelper.HasCustomAttribute(t,typeof(FlagsAttribute));
			var e = ns.AddEnum(conformer.ToCapitalized(t.Name),flags);
            types.Add(t,e);
		}

		private void GenerateClassType(Type t)
		{
			ClassDeclaration c = this.ns.Classes[this.conformer.ToCapitalized(t.Name)];

			// generate fields and properties	
			foreach(FieldInfo f in t.GetFields())
			{
				if (f.FieldType.IsArray)
				{
					AddArrayField(c,f);
				}
				else
				{
					AddField(c,f);
				}
			}

			// add constructors
			GenerateDefaultConstructor(c);
		}

		private void GenerateEnumType(Type t)
		{
			EnumDeclaration e = this.ns.Enums[this.conformer.ToCapitalized(t.Name)];
			if (e==null)
				throw new Exception();
			// generate fields and properties	
			foreach(FieldInfo f in t.GetFields())
			{
				if (f.Name == "Value__")
					continue;

				var field = e.AddField(f.Name);
				// add XmlEnum attribute
				var xmlEnum = new CodeAttributeDeclaration("XmlEnum");
				var arg = new CodeAttributeArgument(
					"Name",
					new CodePrimitiveExpression(f.Name)
					);
				xmlEnum.Arguments.Add(arg);
				field.CustomAttributes.Add(xmlEnum);
			}
		}

		private void AddArrayField(ClassDeclaration c, FieldInfo f)
		{
			// create a collection
			var col = c.AddClass(this.conformer.ToCapitalized(f.Name)+"Collection");
			col.Parent = new TypeTypeDeclaration(typeof(System.Collections.CollectionBase));

			// default constructor
			col.AddConstructor();
			// default indexer
			var index = col.AddIndexer(
				typeof(Object)
				);
			var pindex = index.Signature.AddParam(typeof(int),"index",false);
/*			index.Get = new SnippetStatement();
			index.Get.WriteLine("return this.List[{0}];",pindex.Name);
			index.Set = new SnippetStatement();
			index.Set.WriteLine("this.List[{0}]=value;",pindex.Name);
*/
			// add object method
			var addObject = col.AddMethod("Add");
			var paraObject = addObject.Signature.AddParam(new TypeTypeDeclaration(typeof(Object)),"o",true);
			addObject.Body.Add(
				Expr.This.Prop("List").Method("Add").Invoke(paraObject)
				);

			// if typed array add methods for type
			if (f.FieldType.GetElementType()!=typeof(Object))
			{
				AddCollectionMethods(
					col,
					MapType(f.FieldType.GetElementType()),
					this.conformer.ToCapitalized(f.FieldType.GetElementType().Name),
					"o"
					);
			}

			foreach(XmlElementAttribute ea in f.GetCustomAttributes(typeof(XmlElementAttribute),true))
			{
				var name = this.conformer.ToCapitalized(ea.ElementName);
				var pname= this.conformer.ToCamel(name);

				ITypeDeclaration mappedType = null;
				if (ea.Type!=null)
					mappedType = MapType(ea.Type);

				if (mappedType==null || mappedType == f.FieldType.GetElementType())
					continue;

				AddCollectionMethods(col,mappedType,name,pname);
			}

			// add field
			var fd = c.AddField(col, f.Name);
			fd.InitExpression = Expr.New( col );
			var p = c.AddProperty(fd,true,true,false);

			// setting attributes
			AttachXmlElementAttributes(p,f);
		}

		private void AddCollectionMethods(ClassDeclaration col, ITypeDeclaration mappedType, string name, string pname)
		{
			// add method
			var add = col.AddMethod("Add"+name);
			var para = add.Signature.AddParam(mappedType,pname,true);
			add.Body.Add(
				Expr.This.Prop("List").Method("Add").Invoke(para)
				);

			// add method
			var contains = col.AddMethod("Contains"+name);
			contains.Signature.ReturnType = new TypeTypeDeclaration(typeof(bool));
			para = contains.Signature.AddParam(mappedType,pname,true);
			contains.Body.Return(
				Expr.This.Prop("List").Method("Contains").Invoke(para)
				);

			// add method
			var remove = col.AddMethod("Remove"+name);
			para = remove.Signature.AddParam(mappedType,pname,true);
			remove.Body.Add(
				Expr.This.Prop("List").Method("Remove").Invoke(para)
				);
		}

		private void AddField(ClassDeclaration c, FieldInfo f)
		{
			if(c==null)
				throw new ArgumentNullException(nameof(c));
			if (f==null)
				throw new ArgumentNullException(nameof(f));

			var fd = c.AddField(MapType(f.FieldType),f.Name);
			var p = c.AddProperty(fd,true,true,false);
			// adding attributes
			if (TypeHelper.HasCustomAttribute(f,typeof(XmlAttributeAttribute)))
			{
				var att = (XmlAttributeAttribute)TypeHelper.GetFirstCustomAttribute(f,typeof(XmlAttributeAttribute));
				var attr = new CodeAttributeDeclaration("XmlAttribute");
				string attrName = att.AttributeName;
				if (att.AttributeName.Length==0)
					attrName = f.Name;
				var arg = new CodeAttributeArgument(
					"AttributeName",
					new CodePrimitiveExpression(attrName)
					);
				attr.Arguments.Add(arg);
				p.CustomAttributes.Add(attr);
			}
			else
			{
				if (TypeHelper.HasCustomAttribute(f,typeof(XmlElementAttribute)))
				{
					AttachXmlElementAttributes(p,f);
				}
				else
				{
					var attr = new CodeAttributeDeclaration("XmlElement");
					attr.Arguments.Add(
						new CodeAttributeArgument(
						"ElementName",
						new CodePrimitiveExpression(f.Name)
						)
						);
					p.CustomAttributes.Add(attr);
				}
			}
		}

		private void AttachXmlElementAttributes(PropertyDeclaration p, FieldInfo f)
		{
			// if array is type add element
			if (f.FieldType.IsArray && f.FieldType.GetElementType()!=typeof(Object))
			{
				var attr = new CodeAttributeDeclaration("XmlElement");
				attr.Arguments.Add(
					new CodeAttributeArgument(
					"ElementName",
					new CodePrimitiveExpression(f.Name)
					)
					);
				attr.Arguments.Add(
						new CodeAttributeArgument(
						"Type",
						new CodeTypeOfExpression(MapType(f.FieldType.GetElementType()).Name)
						)
						);						
				p.CustomAttributes.Add(attr);
			}

			foreach(XmlElementAttribute el in f.GetCustomAttributes(typeof(XmlElementAttribute),true))
			{
				var attr = new CodeAttributeDeclaration("XmlElement");
				attr.Arguments.Add(
					new CodeAttributeArgument(
					"ElementName",
					new CodePrimitiveExpression(el.ElementName)
					)
					);
				if (el.Type!=null)
				{
					attr.Arguments.Add(
						new CodeAttributeArgument(
						"Type",
						new CodeTypeOfExpression(MapType(el.Type).Name)
						)
						);						
					p.CustomAttributes.Add(attr);
				}
			}
		}

		private void GenerateDefaultConstructor(ClassDeclaration c)
		{
			var cd = new ConstructorDeclaration(c);
		}

		private ITypeDeclaration MapType(string t)
		{
			if (t==null)
				throw new ArgumentNullException(nameof(t));

			var type = Type.GetType(t,true);
			return MapType(type);
		}

		private ITypeDeclaration MapType(Type t)
		{
			if (t==null)
				throw new ArgumentNullException(nameof(t));
			if (this.types.Contains(t))
				return this.types[t];
			else					
				return new TypeTypeDeclaration(t);
		}
	}
}
