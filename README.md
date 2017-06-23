# Cordar
Cordar is a simple human-readable data representation format that I created. It was inspired by json, but is much simpler and easier to use (in my opinion of course). The name cordar, pronounced like "quarter", is short for *Corbin's Data Representation*.

This repository includes a C++ parser for loading the data from file or stream and easily manipulating it. The parser is called cCordar. It has no external dependencies.

This README will describe how the data is to be structured and how to use the parser.

**Note: The C++ examples in this README have not been tested as of this writing. It's possible there will be syntax errors or other minor issues that would prevent them from being compiled as-is.**

## How to Use the Parser
Copy cCordar.h and cCordar.cpp into your project

## Basic Usage
### Uninteresting Hello World Example
test.cor file:
```
# Lines beginning with a 'hashtag' are considered comments.
# This file contains our data.

Hello World
```

main.cpp file:
```c++
#include "cCordar.h"
//...
cCordar parser;
// boolean status return value being ignored for all examples
parser.LoadFile("test.cor");
std::string value = parser;
std::cout << value;
```
This example would print `Hello World` to the console. Notably, this example shows that the parser object can be implictly casted to a std::string. This becomes more interesting in later examples.

### Key:Value Pairs
test.cor file:
```
Name: Fred
Age: 30
```

main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
std::string name = parser["Name"];
std::string age = parser["Age"];
std::string ssn = parser["SSN"];
std::cout << name << " " << age << " " << ssn;
```
The output would be `Fred 30 `. Notice that strings do not have quotes around them in the cordar file. In fact, all values are only treated as strings. If you would like to treat the Age value in this example as an integer, you would need to convert it using something like `std::stod(parser["Age"]);`.

Also notice that no error is thrown by requesting a property that does not exist (in this case, "SSN"). Calling `parser["SSN"]` will implicitly create that key with an empty value. 

### Named Groups
test.cor file:
```
# Note the colon after the group name
Person:
{
   # Tabs are optional. Line breaks are not.
   Name: Fred
   Age: 30
}
Dog:
{
   Breed: Lab
}
```

main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
std::string name = parser["Person"]["Name"];
```
In this example, `name` would contain `Fred`.

### Named Group Array
test.cor file:
```
Person:
{
   # This is the first element of the Person array (index 0).
   Name: Fred
}
{
   # This is the second element of the Person array (index 1).
   Name: Bob
   HairColor: Brown
}
Dog:
{
   Breed: Lab
}
```

main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
std::string name = parser["Person"][1]["Name"];
```
In this example, `name` would contain `Bob`. A couple of other notable things in this example:
* The cCordar parser does not enforce that each element of the array contain the same key/value pairs.
* `parser["Person"]["Name"]` is equivalent to `parser["Person"][0]["Name"]`. This means that a simple non-array named group, such as the one in the first Named Group example, can also be interpreted as an array of size 1.

### Simple Array
test.cor file:
```
Names:
{
   # Notice that this is a value with no key.
   Fred
}
{
   Bob
}
```
main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
std::string name = parser["Names"][1];
```
In this example, `name` would contain `Bob`.

### Nested Named Groups
test.cor file:
```
Person:
{
   Name: Fred
   Favorites:
   {
      Food: BBQ
      Color: Blue
   }
}
```
main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
std::string food = parser["Person"]["Favorites"]["Food"];
```
In this example, `food` would contain `BBQ`. Other than available memory, there is no limit to the number of nested groups.

## Additional Parser Features
The previous sections contain all of the features of the cordar data format. This section describes some additional features the parser provides in order to better programmatically manipulate the data.

### Size
test.cor file:
```
Person:
{
   Name: Fred
   Age: 40
   HairColor: Brown
}
{
   Name: Bob
   HairColor: Brown
}
Dog:
{
   Breed: Lab
}
```
main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
size_t topLevelSize = parser.Size();
size_t arraySize = parser["Person"].Size();
size_t groupSize = parser["Person"][0].Size();
size_t valueSize = parser["Person"][0]["HairColor"].Size();
```
`topLevelSize` would equal 2 (because of "Person" and "Dog"), `arraySize` would equal 2 (because of the 2 array elements in "Person"), `groupSize` would contain 3, and `valueSize` would contain 1. As the example shows, when requesting the size of an array, the number of indexes in the array is what is returned. It does not include all of the "children". Also, requesting the size of a single element will always return 1 if the element does not have an empty value because there is always at max 1 value. The length of the value string can be obtained after casting the value to a string.

### Getting a Property by Index
test.cor file:
```
Person:
{
   Name: Fred
   Age: 40
   Favorites:
   {
      Food: BBQ
      Color: Blue
   }
}
```
main.cpp file:
```c++
cCordar parser;
parser.LoadFile("test.cor");
std::pair<std::string, cCordar> keyValuePair;
keyValuePair = parser["Person"].GetPropertyByIndex(2);
```
In this example, `keyValuePair.first` would contain the string "Favorites" and `keyValuePair.second` would contain a copy of the remaining Favorites structure. For example, `keyValuePair.second[Food]` could be casted to a std::string to produce "BBQ".
**Note: The order of elements returned by GetPropertyByIndex() is not guaranteed in any way.** Internally cCordar stores these values in an std::map. This function is most useful when combined with `Size()` in order to iterate over all of the elements.

### Serialize/Flatten data to file
main.cpp file:
```c++
cCordar parser;
// Groups and keys can be implicitly created, and strings can be assigned.
parser["Group"]["Key1"] = "Value1";
parser["Group"]["Key2"] = "Value2";
parser.SerializeToFile("Output.cor");
```
output.cor file:
```
Group:
{
   Key1: Value1
   Key2: Value2
}
```
The order of `Key1` and `Key2` is not guaranteed. There is also a function to output the data to an ostream instead with the following function signiture:
```c++
void cCordar::SerializeToStream(std::ostream& a_rStream)
```

### Copying
* *Copy()*
* *Assignment operator considerations*

### Deleting
* *Delete()*
* *Assignment operator considerations*

### Merging Two Data Sets
* *Merge()*
* *Overwriting arrays with values and vice versa*
