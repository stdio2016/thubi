Thubi
===
*English is my second language, so expect some grammar errors*

Thubi is an esoteric language created by Ihope127, however he didn't implement this language, so I try to implement it.

Its official document is <https://esolangs.org/wiki/Thubi>.

Building
---

Requirements:
1. Make
2. GCC (Clang, MinGW and Visual C++ are not tested)

To build Thubi interpretor, just type `make` in your favorite terminal, then you'll see `thubi` executable in `bin` folder.

To run Thubi program: (assume you are in top folder of this repo)
```
bin/thubi [Thubi program file]
```

Notes
---
1. The document didn't say if digits in hexadecimal escape character (`\xnn`) should be uppercase or lowercase. My implementation accepts both uppercase and lowercase hexadecimal digits.

2. My implementation has some extensions:
    1. output like in Thue
        ```
        :<original string>
        ~<string to output>
        ```
        It will work like `<original string>::=~<string to output>` in Thue, except that you can use defined symbols in both side of the production rule.

    2. input like in Thue
        ```
        :<original string>
        :::
        ```
        It will work like `<original string>::=:::` in Thue, except that you can use defined symbols in original string.

    3. comments

        You can have comments in rule section of program.

        A comment is a line starting with `#`. However, you cannot have comments in initial program state section because they will be treated as normal initial strings. For example:
        ```
        #This Thubi program outputs hello world
        #This is a comment
        # This is also a comment
        :start
        # a comment too
        ~hello world
        
        # however, this is not a comment
        start
        # because here is initial program state section
        ```
    4. multiline initial string

        When there are many lines of initial string, they will be concatenated.

        So the above example program has initial string `# however, this is not a commentstart# because here is initial program state section`

Todo
---
I think storing program string as flat array is not fast enough, because it has O(n) complexity for string replacing, n is the length of current program string. Maybe I'll switch to better data structure.
