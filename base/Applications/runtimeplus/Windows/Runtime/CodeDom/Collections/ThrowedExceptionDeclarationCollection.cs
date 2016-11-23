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
///
///3. This notice may not be removed or altered from any source distribution.
*/

using System;

namespace System.Extensions.CodeDom.Collections
{
	/// <summary>
	/// A collection of elements of type ThrowedExceptionDeclaration
	/// </summary>
	public class ThrowedExceptionDeclarationCollection: System.Collections.CollectionBase
	{
		/// <summary>
		/// Initializes a new empty instance of the ThrowedExceptionDeclarationCollection class.
		/// </summary>
		public ThrowedExceptionDeclarationCollection()
		{
			// empty
		}

		/// <summary>
		/// Initializes a new instance of the ThrowedExceptionDeclarationCollection class, containing elements
		/// copied from an array.
		/// </summary>
		/// <param name="items">
		/// The array whose elements are to be added to the new ThrowedExceptionDeclarationCollection.
		/// </param>
		public ThrowedExceptionDeclarationCollection(ThrowedExceptionDeclaration[] items)
		{
            AddRange(items);
		}

		/// <summary>
		/// Initializes a new instance of the ThrowedExceptionDeclarationCollection class, containing elements
		/// copied from another instance of ThrowedExceptionDeclarationCollection
		/// </summary>
		/// <param name="items">
		/// The ThrowedExceptionDeclarationCollection whose elements are to be added to the new ThrowedExceptionDeclarationCollection.
		/// </param>
		public ThrowedExceptionDeclarationCollection(ThrowedExceptionDeclarationCollection items)
		{
            AddRange(items);
		}

		/// <summary>
		/// Adds the elements of an array to the end of this ThrowedExceptionDeclarationCollection.
		/// </summary>
		/// <param name="items">
		/// The array whose elements are to be added to the end of this ThrowedExceptionDeclarationCollection.
		/// </param>
		public void AddRange(ThrowedExceptionDeclaration[] items)
		{
			foreach (ThrowedExceptionDeclaration item in items)
			{
                List.Add(item);
			}
		}

		/// <summary>
		/// Adds the elements of another ThrowedExceptionDeclarationCollection to the end of this ThrowedExceptionDeclarationCollection.
		/// </summary>
		/// <param name="items">
		/// The ThrowedExceptionDeclarationCollection whose elements are to be added to the end of this ThrowedExceptionDeclarationCollection.
		/// </param>
		public void AddRange(ThrowedExceptionDeclarationCollection items)
		{
			foreach (ThrowedExceptionDeclaration item in items)
			{
                List.Add(item);
			}
		}

		/// <summary>
		/// Adds an instance of type ThrowedExceptionDeclaration to the end of this ThrowedExceptionDeclarationCollection.
		/// </summary>
		/// <param name="value">
		/// The ThrowedExceptionDeclaration to be added to the end of this ThrowedExceptionDeclarationCollection.
		/// </param>
		public void Add(ThrowedExceptionDeclaration value)
		{
            List.Add(value);
		}

		/// <summary>
		/// Determines whether a specfic ThrowedExceptionDeclaration value is in this ThrowedExceptionDeclarationCollection.
		/// </summary>
		/// <param name="value">
		/// The ThrowedExceptionDeclaration value to locate in this ThrowedExceptionDeclarationCollection.
		/// </param>
		/// <returns>
		/// true if value is found in this ThrowedExceptionDeclarationCollection;
		/// false otherwise.
		/// </returns>
		public bool Contains(ThrowedExceptionDeclaration value)
		{
			return List.Contains(value);
		}

		/// <summary>
		/// Return the zero-based index of the first occurrence of a specific value
		/// in this ThrowedExceptionDeclarationCollection
		/// </summary>
		/// <param name="value">
		/// The ThrowedExceptionDeclaration value to locate in the ThrowedExceptionDeclarationCollection.
		/// </param>
		/// <returns>
		/// The zero-based index of the first occurrence of the _ELEMENT value if found;
		/// -1 otherwise.
		/// </returns>
		public int IndexOf(ThrowedExceptionDeclaration value)
		{
			return List.IndexOf(value);
		}

		/// <summary>
		/// Inserts an element into the ThrowedExceptionDeclarationCollection at the specified index
		/// </summary>
		/// <param name="index">
		/// The index at which the ThrowedExceptionDeclaration is to be inserted.
		/// </param>
		/// <param name="value">
		/// The ThrowedExceptionDeclaration to insert.
		/// </param>
		public void Insert(int index, ThrowedExceptionDeclaration value)
		{
            List.Insert(index, value);
		}

		/// <summary>
		/// Gets or sets the ThrowedExceptionDeclaration at the given index in this ThrowedExceptionDeclarationCollection.
		/// </summary>
		public ThrowedExceptionDeclaration this[int index]
		{
			get
			{
				return (ThrowedExceptionDeclaration)List[index];
			}
			set
			{
                List[index] = value;
			}
		}

		/// <summary>
		/// Removes the first occurrence of a specific ThrowedExceptionDeclaration from this ThrowedExceptionDeclarationCollection.
		/// </summary>
		/// <param name="value">
		/// The ThrowedExceptionDeclaration value to remove from this ThrowedExceptionDeclarationCollection.
		/// </param>
		public void Remove(ThrowedExceptionDeclaration value)
		{
            List.Remove(value);
		}

		/// <summary>
		/// Type-specific enumeration class, used by ThrowedExceptionDeclarationCollection.GetEnumerator.
		/// </summary>
		public class Enumerator: System.Collections.IEnumerator
		{
            private readonly System.Collections.IEnumerator wrapped;

            public Enumerator(ThrowedExceptionDeclarationCollection collection)
			{
                wrapped = ((System.Collections.CollectionBase)collection).GetEnumerator();
			}

			public ThrowedExceptionDeclaration Current
			{
				get
				{
					return (ThrowedExceptionDeclaration) (wrapped.Current);
				}
			}

			object System.Collections.IEnumerator.Current
			{
				get
				{
					return (ThrowedExceptionDeclaration) (wrapped.Current);
				}
			}

			public bool MoveNext()
			{
				return wrapped.MoveNext();
			}

			public void Reset()
			{
                wrapped.Reset();
			}
		}

		/// <summary>
		/// Returns an enumerator that can iterate through the elements of this ThrowedExceptionDeclarationCollection.
		/// </summary>
		/// <returns>
		/// An object that implements System.Collections.IEnumerator.
		/// </returns>        
		public new Enumerator GetEnumerator()
		{
			return new Enumerator(this);
		}
	}

}
