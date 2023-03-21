# METASERIALIZER
## Serializer/Unserializer powered with Metaprogramming(STL). 

Metaserializer is a simple library which can be used to convert a set of primitive types or complex data types into a contiguous binary data so it can be trasnfered through a socket or an IPC. Combining the power of metaprogramming, simply by using C++ templates the library can define if the class is trivially copiable or its a complex class and must have a unserialize header.

## Features

- 2 simple functions, serialize, unserialize and thats all!
- Using variadic templates can serialize/unserialize many data types at once.
- You can use std string and other simple or complex classes!
- To keep type safe convertion, the library creates a hash of the datatypes being serialized so when we are trying to unserialize the lib does not parse it incorrectly.

## Installation

Metaqueue requires [C++17](https://en.cppreference.com/w/cpp/17) std17+ to compile. Its just a header, import it and you are ready to go!

```sh
g++ -std=c++17 source.cxx -o binary.bin
```
## Usage
Let's create a complex struct to use as an example. 
```c++
#include <Metaserializer.hpp>

struct MyClass
{
    int a;
    int b;
    char c;
    std::string d;

    MyClass(){
        std::memset(this,0,sizeof(MyClass));
    }

    MyClass(const char *str)
    {
        a = 1;
        b = 2;
        c = 'c';
        d = str;
    }

    std::string serialize()
    {
        return MetaSerializer::Serialize<>::apply(a, b, c, d);
    }

    static MyClass unserialize(std::string &data)
    {    
        MyClass tmp;
        MetaSerializer::Unserialize<>::apply(data, tmp.a, tmp.b, tmp.c, tmp.d);
        return tmp;
    }

    friend bool operator==(const A& left, const A& right){
        return left.a == right.a && left.b == right.b && left.c == right.c && left.d == right.d;
    }

    void print(){
        std::cout << "a:<" << a << ">" << std::endl;
        std::cout << "b:<" << b << ">" << std::endl;
        std::cout << "c:<" << c << ">" << std::endl;
        std::cout << "d:<" << d << ">" << std::endl;
    }
};

```

So when we use it in practice we can write something like this:

```c++
MyClass object;
auto serial = object.serialize();
auto unserialize_obj = MyClass::unserialize(serial);

if(object == unserialize_obj){
    std::cout << "Everything OK!" << std::endl;
}else{
    std::cout << "Error! Struct is different :/" << std::endl;
}
```
## License

GPL

**Free Software, Hell Yeah!**
