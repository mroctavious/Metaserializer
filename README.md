# METASERIALIZER
## Serializer/Deserializer powered with Metaprogramming(STL). 

Metaserializer is a simple library which can be used to convert a set of primitive types or complex data types into a contiguous binary data so it can be trasnfered through a socket or an IPC. Combining the power of metaprogramming, simply by using C++ templates the library can define if the class is trivially copiable or its a complex class and must have a deserialize header.

## Features

- 2 simple functions, serialize, deserialize and thats all!
- Using variadic templates can serialize/deserialize many data types at once.
- You can use std string and other simple or complex classes!
- To keep type safe convertion, the library creates a hash of the datatypes being serialized so when we are trying to deserialize the lib does not parse it incorrectly.

## Installation

Metaqueue requires [C++17](https://en.cppreference.com/w/cpp/17) std17+ to compile. Its just a header, import it and you are ready to go!

```sh
g++ -lrt -std=c++17 source.cxx -o binary.bin
```
## Usage
Let's create a complex struct to use as an example. 
```c++
import <Metaserializer>
struct A
{
    int a;
    int b;
    char c;
    std::string d;

    A()
    {
        a = 1;
        b = 2;
        c = 'c';
        d = "HELLO!";
    }

    std::string serialize()
    {
        return MetaSerializer::Serialize<>::apply(a, b, c, d);
    }

    static A deserialize(std::string& data){
        A tmp;
        MetaSerializer::Deserialize::apply(data, tmp.a, tmp.b, tmp.c, tmp.d);
        return tmp;
    }
}
```

So when we use it in practice we can write something like this:

```c++
A f;
auto serial = f.serialize();
auto res = A::deserialize(serial);

if(f == res){
    std::cout << "Everything OK!" << std::endl;
}else{
    std::cout << "Error! Struct is different :/" << std::endl;
}
```
## License

GPL

**Free Software, Hell Yeah!**
