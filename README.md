
><em>"If you wish to make an apple pie from scratch, you must first invent the universe."</em>
> -- Carl Sagan

# cogs
Cogs is a lock-free, cross-platform, C++ template class library and application framework, developed by Colen M. Garoutte-Carson.  This project is available under the MIT license.

Cogs has been my personal project for many years, as a learning process and creative outlet.

(Lots more doc pending.  A very dated set of doxygen generated docs are [here](https://www.cogmine.com/CogsDocs/). )

# Current Status
<p>Under Construction.  A lot still needs to be revisited and updated to use new C++ features.  Lots of tests and doc to write.  Lots more code and comments to write.</p>  
<p>A major refactor is done, and I'm re-porting to Linux, then plan to re-port to MacOS.</p>

# Concepts

## Volatile

Cogs leverages 'volatile' to qualify a type as atomic, in much the same way as the 'const' qualifier allows an object to be references in a const manner.  

Like the const qualifier, volatile can be added to member functions, in which in the object and its members are also volatile.  Methods with and without the volatile qualifier can be overloaded separately, to provide versions of the same algorithm with and without the need to consider thread safety.

```cpp
class A
{
private:
   ptr<void> m_ptr;

public:
    // Sees: ptr<void> m_ptr;
    void foo(); // #1

    // Sees: const ptr<void> m_ptr;
    void foo() const;   // #2

    // Sees: volatile ptr<void> m_ptr;
    void foo() volatile;    // #3

    // Sees: const volatile ptr<void> m_ptr;
    void foo() const volatile;   // #4
};

void bar(A& a1, const A& a2, volatile A& a3, const volatile A& a4)
{
    a1.foo();   // Calls #1
    a2.foo();   // Calls #2
    a3.foo();   // Calls #3
    a4.foo();   // Calls #4
}
```

