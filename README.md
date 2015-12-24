Tool to convert text files between ASCII, UTF-8, UTF-16BE, and UTF-16LE.

features

Octal Dump
    od -t cxC file.txt

Encode
    java Encode -b -e ENCODING

    ENCODING: US-ASCII, ASCII, UTF-8, UTF-16LE, UTF-16BE



                                        * define CSE320:
                                            - The following should be displayed at the top:
                                                - The current name of the host machine using 'gethostname'
                                                - The name of the input file
                                                    - same line The input files inode number using 'stat' and 'st_ino'
                                                    - same line The input files device number using 'stat' and 'st_dev'
                                                    - same line The input files size in bytes using 'stat' and 'st_size'
                                                - The name of the output file
                                                - The encoding of the input file.
                                                - The selected output encoding.
                                        ex:'''
                                        $ make clean && make debug
                                        $ ./utfconverter -e UTF-16BE input.txt output.txt
                                        CSE320: Host: sparky
                                        CSE320: Input: input.txt, 79757314, 152813, 2 byte(s)
                                        CSE320: Output: output.txt
                                        CSE320: Input encoding: UTF-8
                                        CSE320: Output encoding: UTF-16BE
                                        The file output.txt was successfully created.
                                        '''

                                        * Call debug using all the fields.

                                        * Program compiles without -DCSE320 flag as well.

                                        * Upon success, the program should write to stdout:
                                            "The file %s was successfully created.\n", output_file

                                        * Check to see if the input file and output file are the same. Check to see if a file is a symbolic link of the other.
                                            - check to see if they have the same 'inode'.
                                            - The function 'stat' will help determine this information, with fields 'st_dev' and 'st_ino'
                                            - terminate with EXIT_FAILURE

                                        ex:'''
                                        $ ./utfconverter -e UTF-16BE input.txt funky.txt
                                        The file funky.txt was not created. Same as input file.
                                        '''

                                        * Program should check the input file for a BOM marking
                                            - If the file does not contain a valid BOM, it should terminate with EXIT_FAILURE
                                            - Test this using files in text_files/part3
                                        ex:'''
                                        $ ./utfconverter -e UTF-8 input.txt output.txt
                                        The input file input.txt does not have a valid BOM.
                                        The file output.txt was not created.

                                        Usage: ./utf....
                                        ...
                                        '''

                                        * If input BOM is the same as OUTPUT bom then, output that no conversion needed to be done.

                                        * Add a BOM to every file we create.
                                            ENCODING    BOM
                                            ---------------
                                            UTF-8       0XEFBBBF
                                        UTF-16LE    0XFFEE
                                        UTF-16BE    0XFEFF

                                        * use 'getopt' to handle flags and arguments.
                                            - modify the option string in the 'getopt' function in the base code.

                                        * Mandatory -e or --encoding flag
                                            - takes arguments(case-sensitive):
                                                - UTF-8
                                                - UTF-16BE
                                                - UTF-16LE
                                                - any other value should make the program display the USAGE STATEMENT and quit with EXIT_FAILURE.
                                        ex:'''
                                        $ ./utfconverter -e BAD input.txt output.txt
                                        The flag `-e` has an incorrect value `BAD`.
                                        The file output.txt was not created.

                                        Usage: ./utf....
                                        ...
                                        '''

                                        * no -e flag makes the program display the USAGE STATEMENT and quit with EXIT_FAILURE.
                                        ex:'''
                                        $ ./utfconverter -vv -v input.txt output.txt
                                        Missing the `-e` flag. This flag is required.
                                        The file output.txt was not created. 

                                        Usage: ./utf....
                                        ...
                                        '''

* Support for -v flag
    - write the following to stdout:
        - The visible ASCII value of the codepoint if it exists, else the word NONE
        - The number of bytes the input encoding of the glyph takes up
        - The codepoint of each glyph being translated
ex:'''
$ ./utfconverter -v -e UTF-16BE input.txt output.txt
+---------+------------+-----------+
|  ASCII  | # of bytes | codepoint |
+---------+------------+-----------+
|    A    |     1      |   U+0041  |
+---------+------------+-----------+
|    B    |     1      |   U+0042  |
+---------+------------+-----------+
The file output.txt was successfully created.
'''

* Support for -vv or -v -v flag
    - write the following to stdout:
        - everything from -v
        - the hex value of the input glyph being translated
ex:'''
$ ./utfconverter -vv -e UTF-16BE input.txt output.txt
+---------+------------+-----------+-----------+
|  ASCII  | # of bytes | codepoint |  input    |
+---------+------------+-----------+-----------+
|    A    |     1      |   U+0041  | 0x41      |
+---------+------------+-----------+-----------|
|    B    |     1      |   U+0042  | 0x42      |
+---------+------------+-----------+-----------+
The file output.txt was successfully created.
'''

* Support for -vvv or -v -v -v flag
    - write the following to stdout:
        - everything from -vv
        - the hex value of the output glyph being translated
ex:'''
$ ./utfconverter -vvv -e UTF-16BE input.txt output.txt
+---------+------------+-----------+-----------+--------------+
|  ASCII  | # of bytes | codepoint |  input    |  output      |
+---------+------------+-----------+-----------+--------------+
|    A    |     1      |   U+0041  | 0x41      | 0x0041       |
+---------+------------+-----------+-----------+--------------+
|    B    |     1      |   U+0042  | 0x42      | 0x0042       |
+---------+------------+-----------+-----------+--------------+
The file output.txt was successfully created.
'''

* Any more than 3 -v's should be treated as -vvv

* After you finish and push.
    - git tag hw2
    - git push origin hw2
    - Check to see hw2 is in gitlab.

* Program works on Sparky

* Program works on OSX
    - write in README that it works in OSX
