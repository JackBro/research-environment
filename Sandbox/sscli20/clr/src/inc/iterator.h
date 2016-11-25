// ==++==
// 
//   
//    Copyright (c) 2006 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
// ---------------------------------------------------------------------------
// Iterator.h
// ---------------------------------------------------------------------------

// ================================================================================
// Iterator pattern:
//
// This pattern is similar to the STL iterator pattern.  It basically consists of
// wrapping an "iteration variable" in an object, and providing pointer-like operators
// on the iterator. Example usage:
//
// for (Iterator start = foo->Begin(), end = foo->End(); start != end; start++)
// {
//      // use foo, start
// }
//
// There are 3 levels of iterator functionality
//      1. Enumerator (STL forward) - only go forward one at a time
//          Enumerators have the following operations:
//              operator * : access current element as ref
//              operator -> : access current element as ptr
//              operator++ : go to the next element
//              operator==, operator!= : compare to another enumerator
//
//
//      2. Scanner (STL bidirectional) - backward or forward one at a time
//          Scanners have all the functionality of enumerators, plus:
//              operator-- : go backward one element
//
//      3. Indexer (STL random access)  - skip around arbitrarily
//          Indexers have all the functionality of scanners, plus:
//              operator[] : access the element at index from iterator as ref
//              operator+= : advance iterator by index 
//              operator+ : return new iterator at index from iterator
//              operator-= : rewind iterator by index 
//              operator- : return new iterator at index back from iterator
//              operator <, operator <=, operator >, operator>= :
//                  range comparison on two indexers    
//
// The object being iterated should define the following methods:
//      Begin()             return an iterator starting at the first element
//      End()               return an iterator just past the last element
//
// Iterator types are normally defined as member types named "Iterator", no matter
// what their functionality level is.
// ================================================================================

#ifndef ITERATOR_H_
#define ITERATOR_H_

#include "contract.h"



template <typename CONTAINER>
class CheckedIteratorBase
{
  protected:
#if defined(_DEBUG)
    const CONTAINER *m_container;
#endif

    CHECK CheckRevision() const
    {
        LEAF_CONTRACT;
        CHECK_OK;
    }

    CheckedIteratorBase()
    {
        LEAF_CONTRACT;
#if defined(_DEBUG)
        m_container = NULL;
#endif
    }

    CheckedIteratorBase(const CONTAINER *container)
    {
        LEAF_CONTRACT;
#if defined(_DEBUG)
        m_container = container;
#endif
    }

    void Resync(const CONTAINER *container)
    {
        LEAF_CONTRACT;

    }
    
#if defined(_DEBUG)
    const CONTAINER *GetContainerDebug() const
    {
        LEAF_CONTRACT;
        return m_container;
    }
#endif

  public:
    CHECK Check() const
    {
        WRAPPER_CONTRACT;
        CHECK(CheckRevision());
        CHECK_OK;
    }

    CHECK CheckContainer(const CONTAINER *container) const
    {
        WRAPPER_CONTRACT;
#if defined(_DEBUG)
        CHECK(container == m_container);
#endif
        CHECK_OK;
    }

};


// --------------------------------------------------------------------------------
// Enumerator, Scanner, and Indexer provide a template to produce an iterator 
// on an existing class with a single iteration variable.
//
// The template takes 3 type parameters:
// CONTAINER - type of object being interated on 
// ELEMENT - type of iteration
// BASE - base type of the iteration.  This type must follow the pattern described 
//      by the above Prototypes
// --------------------------------------------------------------------------------

template <typename ELEMENT, typename SUBTYPE>
class Enumerator
{
 private:
    const SUBTYPE *This() const
    {
        return (const SUBTYPE *) this;
    }

    SUBTYPE *This()
    {
        return (SUBTYPE *)this;
    }

  public:

    Enumerator()
    {
    }

    CHECK CheckIndex() const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck());
#endif
        CHECK_OK;
    }

    ELEMENT &operator*() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return This()->Get();
    }
    ELEMENT *operator->() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return &(This()->Get());
    }
    SUBTYPE &operator++()
    {
        PRECONDITION(CheckPointer(This()));

        This()->Next();
        return *This();
    }
    SUBTYPE operator++(int)
    {
        PRECONDITION(CheckPointer(This()));

        SUBTYPE i = *This();
        This()->Next();
        return i;
    }
    bool operator==(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Equal(i);
    }
    bool operator!=(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return !This()->Equal(i);
    }
};

template <typename ELEMENT, typename SUBTYPE>
class Scanner 
{
 private:
    const SUBTYPE *This() const
    {
        return (const SUBTYPE *)this;
    }

    SUBTYPE *This()
    {
        return (SUBTYPE *)this;
    }

  public:

    Scanner()
    {
    }

    CHECK CheckIndex() const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck());
#endif
        CHECK_OK;
    }

    ELEMENT &operator*() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return This()->Get();
    }
    ELEMENT *operator->() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return &This()->Get();
    }
    SUBTYPE &operator++()
    {
        PRECONDITION(CheckPointer(This()));

        This()->Next();
        return *This();
    }
    SUBTYPE operator++(int)
    {
        PRECONDITION(CheckPointer(This()));

        SUBTYPE i = *This();
        This()->Next();
        return i;
    }
    SUBTYPE &operator--()
    {
        PRECONDITION(CheckPointer(This()));

        This()->Previous();
        return *This();
    }
    SUBTYPE operator--(int)
    {
        PRECONDITION(CheckPointer(this));

        SUBTYPE i = *This();
        This()->Previous();
        return i;
    }
    bool operator==(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Equal(i);
    }
    bool operator!=(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return !This()->Equal(i);
    }
};

template <typename ELEMENT, typename SUBTYPE>
class Indexer
{
 private:
    const SUBTYPE *This() const
    {
        return (const SUBTYPE *)this;
    }

    SUBTYPE *This()
    {
        return (SUBTYPE *)this;
    }

  public:

    Indexer()
    {
        LEAF_CONTRACT;
    }
    
    CHECK CheckIndex() const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck(0));
#endif
        CHECK_OK;
    }

    CHECK CheckIndex(int index) const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck(index));
#endif
        CHECK_OK;
    }

    ELEMENT &operator*() const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex(0));

        return *(ELEMENT*)&This()->GetAt(0);
    }
    ELEMENT *operator->() const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex(0));

        return &This()->GetAt(0);
    }
    ELEMENT &operator[](int index) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex(index));
        return *(ELEMENT*)&This()->GetAt(index);
    }
    SUBTYPE &operator++()
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(1);
        return *This();
    }
    SUBTYPE operator++(int)
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        This()->Skip(1);
        return i;
    }
    SUBTYPE &operator--()
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(-1);
        return *This();
    }
    SUBTYPE operator--(int)
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        This()->Skip(-1);
        return i;
    }
    SUBTYPE &operator+=(SCOUNT_T index)
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(index);
        return *This();
    }
    SUBTYPE operator+(SCOUNT_T index) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        i.Skip(index);
        return i;
    }
    SUBTYPE &operator-=(SCOUNT_T index)
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(-index);
        return *This();
    }
    SUBTYPE operator-(SCOUNT_T index) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        i.Skip(-index);
        return i;
    }
    SCOUNT_T operator-(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        
        return This()->Subtract(i);
    }
    bool operator==(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Subtract(i) == 0;
    }
    bool operator!=(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Subtract(i) != 0;
    }
    bool operator<(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) < 0;
    }
    bool operator<=(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) <= 0;
    }
    bool operator>(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) > 0;
    }
    bool operator>=(const SUBTYPE &i) const
    {
        WRAPPER_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) >= 0;
    }
};

#endif  // ITERATOR_H_