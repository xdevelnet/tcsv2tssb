# tcsv2tssb

tcsv2tssb is a converter from tcsv to tssb, which may be useful if you're going to store strings with translations.

Please read about (T)SSB project before using this converter.

### How to use it?

1. Clone cccsvparser library. From here https://github.com/xdevelnet/cccsvparser or from here http://sourceforge.net/projects/cccsvparser/
2. Clone this repository
3. ```
    cd tcsv2tssb
    make
    ```
4. Now you can use tcsv2tssb program on any csv file with strings. E.g.
    ```
    ./tcsv2tssb examples/data.csv
    ```
5. You have generated two files in `examples` directory. First one is your `.ssb` file, second is `.h` file which may be useful in your target application if you're planning to know exact location of each table cell at compile time.

### What's the point of using TSSB as place to store strings with translations? You can use gettext!

First of all, gettext is a great library. Easy to use, well tested, with big number of libraries around it and even more.

But! I need library with following requirements:

1) Ability to retrieve pointer to translated string of any language as fast as possible (few CPU cycles), especially if you know exactly what languages and strings you have at application's compile time;
2) Ability to retrieve amount of bytes that is used for such string. So your program can avoid finding null terminator and you can operate with it like with any binary data.

As you see, using gettext is possible, but I don't want to change locale every time when I need different language, I don't want to spent CPU cycles to find out proper translation string when it's already-known data (e.g. rodata). Also, I don't like strlen()ing every time when I need to push somewhere these string.

### Errata

 * The overall quality of this program is not that good as `libtssb`. Also, relying `cccsvparser` libray, from my (and valgrind's) POV could be much better. But it's not my job to reinvent bicycle and make another-nobodyneedsit-csv-parsing library. If you have better replacement -- create PR or new issue.
 * Despite the fact that program was developed taking in account that it could be used under BIG ENDIAN machine, developer haven't such machine. So, there was no testing performed under BIG ENDIAN machine. Hope someone will do it for me.

### How to use generated .ssb and .h files?

1) .ssb: take a look at https://github.com/xdevelnet/ssb repo and tssb parser with it.
2) .h: just include it to your project where tssb library will be used. Therefore you could acces 2dimensional array like this:
   ```
   arr[SSBLANG_A][SSBBASE_B]
   ```

   If you're getting compilation error, you should take a look at generated header file and check if it's correct. Most likely you got macro redefinitions, which may be caused by incorrect csv. Go fix your csv if you're going to use header file.
