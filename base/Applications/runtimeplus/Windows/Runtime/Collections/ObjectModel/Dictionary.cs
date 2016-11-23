/// Copyright (c) 2015 Jonathan Moore
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
namespace System.Extensions.Collections.ObjectModel
{
  using System.Collections;
  using System.Collections.Generic;
  using System.Threading;
  using System.Threading.Tasks;

  /// <summary>
  /// Provides the base class for a generic dictionary.
  /// </summary>
  /// <typeparam name="TKey">The type of the keys in the dictionary.</typeparam>
  /// <typeparam name="TValue">The type of the values in the dictionary.</typeparam>
  public abstract class Dictionary<TKey, TValue> : IDictionary<TKey, TValue>
  {
    /// <summary>
    /// Stores the internal dictionary.
    /// </summary>
    private IDictionary<TKey, TValue> items;

    /// <summary>
    /// Initializes a new instance of the Dictionary class with the default key comparer.
    /// </summary>
    protected Dictionary()
    {
      this.items = new System.Collections.Generic.Dictionary<TKey, TValue>();
    }

    /// <summary>
    /// Initializes a new instance of the Dictionary class with the specified key comparer.
    /// </summary>
    /// <param name="comparer">The comparer to use when testing for equality on keys.</param>
    protected Dictionary(IEqualityComparer<TKey> comparer)
    {
      this.items = new System.Collections.Generic.Dictionary<TKey, TValue>(comparer);
    }

    /// <summary>
    /// Gets a collection containing the values in the Dictionary.
    /// </summary>
    public ICollection<TValue> Values
    {
      get { return this.items.Values; }
    }

    /// <summary>
    /// Gets the number of key/value pairs contained in the Dictionary.
    /// </summary>
    public int Count
    {
      get { return this.items.Count; }
    }

    /// <summary>
    /// Gets a value indicating whether the Dictionary is read-only.
    /// </summary>
    public bool IsReadOnly
    {
      get { return this.items.IsReadOnly; }
    }

    /// <summary>
    /// Gets a collection containing the keys in the Dictionary.
    /// </summary>
    public ICollection<TKey> Keys
    {
      get { return this.items.Keys; }
    }

    /// <summary>
    /// Gets or sets the value associated with the specified key.
    /// </summary>
    /// <param name="key">The key of the element to get or set.</param>
    /// <returns>The value associated with the specified key.  If the specified key is not found, a get operation throws an exception, and a set operation creates a new element with the specified key.</returns>
    public TValue this[TKey key]
    {
      get { return this.items[key]; }
      set { this.SetItem(key, value); }
    }

    /// <summary>
    /// Adds an item to the Dictionary.
    /// </summary>
    /// <param name="key">The key of the item to add.</param>
    /// <param name="value">The item to add.</param>
    public void Add(TKey key, TValue value)
    {
      this.AddItem(key, value);
    }

    /// <summary>
    /// Removes an item from the Dictionary.
    /// </summary>
    /// <param name="key">The key of the item to remove.</param>
    /// <returns><b>True</b> if it was removed, <b>false</b> otherwise.</returns>
    public bool Remove(TKey key)
    {
      if (this.ContainsKey(key))
      {
        this.RemoveItem(key);
        return true;
      }
      else
      {
        return false;
      }
    }

    /// <summary>
    /// Removes all objects from the Dictionary.
    /// </summary>
    public void Clear()
    {
      this.ClearItems();
    }

    /// <summary>
    /// Determines whether the Dictionary contains the specified key.
    /// </summary>
    /// <param name="key">The key to locate.</param>
    /// <returns><b>True</b> if the Dictionary contains an element with the specified key, <b>false</b> otherwise.</returns>
    public bool ContainsKey(TKey key)
    {
      return this.items.ContainsKey(key);
    }

    /// <summary>
    /// Gets the value associated with the specified key.
    /// </summary>
    /// <param name="key">The key of the item to get.</param>
    /// <param name="value">The item if found, otherwise the default value for the type.</param>
    /// <returns><b>True</b> if the item is found, <b>false</b> otherwise.</returns>
    public bool TryGetValue(TKey key, out TValue value)
    {
      return this.items.TryGetValue(key, out value);
    }

    /// <summary>
    /// Returns an enumerator that iterates through the Dictionary.
    /// </summary>
    /// <returns>The enumerator.</returns>
    public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
    {
      return this.items.GetEnumerator();
    }

    /// <summary>
    /// Determines whether the Dictionary contains a specified item by using the default equality comparer.
    /// </summary>
    /// <param name="item">The item to look for.</param>
    /// <returns><b>True</b> if found, <b>false</b> otherwise.</returns>
    public bool Contains(KeyValuePair<TKey, TValue> item)
    {
      return this.items.Contains(item);
    }

    /// <summary>
    /// Copies the elements of the Dictionary to an array, starting at the specified array index.
    /// </summary>
    /// <param name="array">The one-dimensional that is the destination of the elements copied from the Dictionary.  The array must have zero-based indexing.</param>
    /// <param name="arrayIndex">The zero-based index in <em>array</em> at which copying begins.</param>
    public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
    {
      this.items.CopyTo(array, arrayIndex);
    }

    /// <summary>
    /// Adds a <see cref="KeyValuePair{TKey,TValue}"/> to the dictionary.
    /// </summary>
    /// <param name="item">The item to add.</param>
    void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
    {
      this.Add(item.Key, item.Value);
    }

    /// <summary>
    /// Removes a <see cref="KeyValuePair{TKey,TValue}"/> to the dictionary.
    /// </summary>
    /// <param name="item">The item to remove.</param>
    /// <returns><b>True</b> if successful, <b>false</b> otherwise.</returns>
    bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
    {
      if (this.Contains(item))
      {
        return this.Remove(item.Key);
      }
      else
      {
        return false;
      }
    }

    /// <summary>
    /// Returns an enumerator that iterates through the Dictionary.
    /// </summary>
    /// <returns>The enumerator.</returns>
    IEnumerator IEnumerable.GetEnumerator()
    {
      return this.items.GetEnumerator();
    }

    /// <summary>
    /// Removes all objects from the Dictionary.
    /// </summary>
    protected virtual void ClearItems()
    {
      this.items.Clear();
    }

    /// <summary>
    /// Removes all objects from the Dictionary by calling Remove on each of them.
    /// </summary>
    protected void ClearItemsByRemoving()
    {
      TKey[] keys = new TKey[this.Count];

      Parallel.For(0, this.Count, index =>
      {
          this.Remove(keys[index]);
      });
    }

    /// <summary>
    /// Adds an item to the Dictionary.
    /// </summary>
    /// <param name="key">The key of the item to add.</param>
    /// <param name="value">The item to add.</param>
    protected virtual void AddItem(TKey key, TValue value)
    {
      this.items.Add(key, value);
    }

    /// <summary>
    /// Removes an item from the Dictionary.
    /// </summary>
    /// <param name="key">The key of the item to remove.</param>
    protected virtual void RemoveItem(TKey key)
    {
      this.items.Remove(key);
    }

    /// <summary>
    /// Replaces the item with the specified key.
    /// </summary>
    /// <param name="key">The key of the item to replace.</param>
    /// <param name="value">The item to replace it with.</param>
    protected virtual void SetItem(TKey key, TValue value)
    {
      this.items[key] = value;
    }
  }
}
