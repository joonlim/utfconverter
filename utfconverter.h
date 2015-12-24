#ifndef __UTFCONVERTER_H
#define __UTFCONVERTER_H
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdbool.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/stat.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <stdint.h>
    #include <ctype.h>

    #ifdef CSE320
        #define debug(host, input_path, ino, dev, size, output_path, input_encoding, output_encoding) {\
           printf("CSE320: Host: %s\n", host);\
           printf("CSE320: Input: %s, %d, %d, %d bytes(s)\n", input_path, ino, dev, size);\
           printf("CSE320: Output: %s\n", output_path);\
           printf("CSE320: Input encoding: %s\n", input_encoding);\
           printf("CSE320: Output encoding: %s\n", output_encoding);\
        }
    #else
        #define debug(fmt, ...)
    #endif

    /* Constants for validate_args return values. */
    #define VALID_ARGS  0
    #define SAME_FILE   1
    #define FILE_DNE    2
    #define INVALID_BOM 3
    #define SAME_BOM    4
    #define FAILED      5

    /* Input / Output Encoding */
    #define UTF8        1
    #define UTF16LE     2
    #define UTF16BE     3

    #define UTF8_4_BYTE 0xF0
    #define UTF8_3_BYTE 0xE0
    #define UTF8_2_BYTE 0xC0
    #define UTF8_CONT   0x80

    /* # of bytes a UTF-16 codepoint takes up */
    #define CODE_UNIT_SIZE 2

    #define SURROGATE_PAIR 0x10000

    /**
     * Checks to make sure the input arguments meet the following constraints.
     * 1. input_path is a path to an existing file.
     * 2. output_path is not the same path as the input path.
     * 3. output_format is a correct format as accepted by the program.
     * @param input_path Path to the input file being converted.
     * @param output_path Path to where the output file should be created.
     * @param in_encoding [out] Encoding of input file using its BOM.
     * @param out_encoding Desired encoding of output file to be compared with in_encoding.
     * @return Returns 0 if there was no errors, 1 if they are the same file, 2
     *         if the input file doesn't exist, 3 if something went wrong.
     */
    int validate_args(const char *input_path, const char *output_path, int *in_encoding, const int out_encoding);

    /**
     * Converts the input file and converts it to the proper format.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @param in_encoding The encoding of the input file.
     * @param out_encoding The desired encoding of the output file.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert(const int input_fd, const int output_fd, const int in_encoding, const int out_encoding);

    /**
     * Converts a UTF-8 file to UTF-16BE.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert_UTF8_to_UTF16BE(const int input_fd, const int output_fd);

    /**
     * Converts a UTF-8 file to UTF-16LE.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert_UTF8_to_UTF16LE(const int input_fd, const int output_fd);

    /**
     * Converts a UTF-16LE file to UTF-8.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert_UTF16LE_to_UTF8(const int input_fd, const int output_fd);

    /**
     * Converts a UTF-16LE file to UTF-16BE.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert_UTF16LE_to_UTF16BE(const int input_fd, const int output_fd);

    /**
     * Converts a UTF-16BE file to UTF-8.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert_UTF16BE_to_UTF8(const int input_fd, const int output_fd);

    /**
     * Converts a UTF-16BE file to UTF-16LE.
     * @param input_fd The input files file descriptor.
     * @param output_fd The output files file descriptor.
     * @return Returns true if the conversion was a success else false.
     */
    bool convert_UTF16BE_to_UTF16LE(const int input_fd, const int output_fd);

    /**
     * Writes bytes to output_fd and reports the success of the operation.
     * @param input_fd File descriptor of the input file.
     * @param output_fd File descriptor of the output file.
     * @param value Value to be written to file.
     * @param size Size of the value in bytes to write.
     * @return Returns true if the write was a success, else false.
     */
    bool safe_write(const int input_fd, const int output_fd, void *value, size_t size);

    /**
     * Print out the program usage string
     */
    #define USAGE(name) do {                                                                                                \
        fprintf(stderr,                                                                                                     \
            "\nUsage: %s [-h] [-v | -vv | -vvv] -e OUTPUT_ENCODING INPUT_FILE OUTPUT_FILE \n"                               \
            "\n"                                                                                                            \
            "Command line utility for converting files to and from UTF-8, UTF-16LE, or UTF-16BE.\n"                         \
            "\n"                                                                                                            \
            "Option arguments:\n\n"                                                                                         \
            "-h                             Displays this usage menu.\n"                                                    \
            "\n"                                                                                                            \
            "-v                             Enables verbose output.\n"                                                      \
            "                               This argument can be used up to\n"                                              \
            "                               three times for a noticeable effect.\n"                                         \
            "\n"                                                                                                            \
            "-e OUTPUT_ENCODING             Format to encode the output file.\n"                                            \
            "                               Accepted values:\n"                                                             \
            "                                        UTF-8\n"                                                               \
            "                                        UTF-16LE\n"                                                            \
            "                                        UTF-16BE\n"                                                            \
            "                               If this flag is not provided or an\n"                                           \
            "                               invalid value is given the program\n"                                           \
            "                               should exit with the EXIT_FAILURE\n"                                            \
            "                               return code.\n"                                                                 \
            "\n"                                                                                                            \
            "\nPositional arguments:\n"                                                                                     \
            "\n"                                                                                                            \
            "INPUT_FILE                     File to convert. Must contain a \n"                                             \
            "                               valid BOM. If it does not contain a\n"                                          \
            "                               valid BOM the program should exit\n"                                            \
            "                               with the EXIT_FAILURE return code.\n"                                           \
            "\n"                                                                                                            \
            "OUTPUT_FILE                    Output file to create. If the file\n"                                           \
            "                               already exists and its not the input\n"                                         \
            "                               file, it should be overwritten. If\n"                                           \
            "                               the OUTPUT_FILE is the same as the\n"                                           \
            "                               INPUT_FILE the program should exit\n"                                           \
            "                               with the EXIT_FAILURE return code.\n"                                           \
            ,(name)                                                                                                         \
        );                                                                                                                  \
    } while(0)
#endif
