#include "utfconverter.h"

int vflag = 0; // verbose flag. up to 3

int main(int argc, char *argv[])
{
    int opt; // used to check flags and arguments
    int return_code = EXIT_FAILURE;

    char *input_path = NULL;
    char *output_path = NULL;

    char *encoding = NULL;
    int out_encoding = 0; 
    int eflag = 0; // encoding flag

    /* Parse short options */
    while((opt = getopt(argc, argv, "vhe:")) != -1) {
        switch(opt) {
            case 'h':
                /* The help menu was selected */
                USAGE(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case 'e':
                eflag++;

                encoding = optarg;

                if (strcmp(optarg, "UTF-8") == 0)
                    out_encoding = UTF8;
                else if (strcmp(optarg, "UTF-16LE") == 0)
                    out_encoding = UTF16LE;
                else if (strcmp(optarg, "UTF-16BE") == 0)
                    out_encoding = UTF16BE;
                
                break;
            case 'v':
                vflag++;
                break;
            case '?':
                /* Let this case fall down to default;
                 * handled during bad option.
                 */
            default:
                /* A bad option was provided. */
                fprintf(stderr, "The file output.txt was not created.\n");
                USAGE(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }

    // vflag greater than 3 should be treated as -vvv.
    if (vflag > 3)
        vflag = 3;

    // Check e flag
    if (eflag == 0) {
        // user did not write '-e'.
        fprintf(stderr, "Missing the `-e` flag. This flag is required.\n");
        fprintf(stderr, "The file output.txt was not created.\n");
        USAGE(argv[0]);
        exit(EXIT_FAILURE); 
    }
    else {
        // check to see that encoding is valid.
        if (out_encoding == 0) {
            /* A bad option was provided. */
            fprintf(stderr, "The flag `-e` has an incorrect value `%s`.\n", encoding);
            fprintf(stderr, "The file output.txt was not created.\n");
            USAGE(argv[0]);
            exit(EXIT_FAILURE); 
        }
    }

    /* Get position arguments */
    if(optind < argc && (argc - optind) == 2) {
        input_path = argv[optind++];
        output_path = argv[optind++];
    } else {
        if((argc - optind) <= 0) {
            fprintf(stderr, "Missing INPUT_FILE and OUTPUT_FILE.\n");
        } else if((argc - optind) == 1) {
            fprintf(stderr, "Missing OUTPUT_FILE.\n");
        } else {
            fprintf(stderr, "Too many arguments provided.\n");
        }
        USAGE(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Make sure all the arguments were provided */
    if(input_path != NULL && output_path != NULL) {

        int input_fd = -1, output_fd = -1;
        bool success = false;

        int in_encoding = 0; // this is store the encoding of input depending on its BOM.

        switch(validate_args(input_path, output_path, &in_encoding, out_encoding)) {

                case VALID_ARGS:
                    /* Attempt to open the input file */
                    if((input_fd = open(input_path, O_RDONLY)) < 0) {
                        fprintf(stderr, "Failed to open the file %s\n", input_path);
                        perror(NULL);
                        goto conversion_done;
                    }
                    /* Delete the output file if it exists; Don't care about return code. */
                    unlink(output_path);
                    /* Attempt to create the file */
                    if((output_fd = open(output_path, O_CREAT | O_WRONLY,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) {
                        /* Tell the user that the file failed to be created */
                        fprintf(stderr, "Failed to open the file %s\n", input_path);
                        perror(NULL);
                        goto conversion_done;
                    }

#ifdef CSE320
        char *input_encoding = NULL;
        if (in_encoding == UTF8)
            input_encoding = "UTF-8";
        else if (in_encoding == UTF16LE)
            input_encoding = "UTF-16LE";
        else 
            input_encoding = "UTF-16BE";

        struct stat *instat = (struct stat*)malloc(sizeof(struct stat));
        /* zero out the memory */
        memset(instat, 0, sizeof(struct stat));
        /* now check to see if the file exists */
        stat(input_path, instat);
        // output file exists. check to make sure it is not the same as input.
        int ino = (int)instat->st_ino;
        int dev = (int)instat->st_dev;
        int size = (int)instat->st_size;

        free(instat);

        char host[41];
        gethostname(host, 40);

        debug(host, input_path, ino, dev, size, output_path, input_encoding, encoding);
#endif


                    /* Start the conversion */
                    success = convert(input_fd, output_fd, in_encoding, out_encoding);
conversion_done:
                    if(success) {
                        /* We got here so it must of worked right? */
                        printf("The file %s was successfully created.\n", output_path);
                        return_code = EXIT_SUCCESS;
                    } else {
                        /* Conversion failed; clean up */
                        if(output_fd < 0 && input_fd >= 0) {
                            close(input_fd);
                        }
                        if(output_fd >= 0) {
                            unlink(output_path);
                        }
                        /* Just being pedantic... */
                        fprintf(stderr, "Conversion failed for an unknown reason.\n");
                        return_code = EXIT_FAILURE;
                    }
                    break;
                case SAME_FILE:
                    fprintf(stderr, "The output file %s was not created. Same as input file.\n", output_path);
                    break;
                case FILE_DNE:
                    fprintf(stderr, "The input file %s does not exist.\n", input_path);
                    fprintf(stderr, "The file %s was not created.\n", output_path);
                    break;
                case INVALID_BOM:
                    fprintf(stderr, "The input file %s does not have a valid BOM.\n", input_path);
                    fprintf(stderr, "The file %s was not created.\n", output_path);
                    break;
                case SAME_BOM:
                    fprintf(stderr, "The input file %s is already encoded in %s.\n", input_path, encoding);
                    fprintf(stderr, "The file %s was not created.\n", output_path);
                    break;
                default:
                    fprintf(stderr, "An unknown error occurred\n");
                    break;
        }
    } else {
        /* Alert the user what was not set before quitting. */
        if(input_path == NULL) {
            fprintf(stderr, "INPUT_FILE was not set.\n");
        }
        if(output_path == NULL) {
            fprintf(stderr, "OUTPUT_FILE was not set.\n");
        }
        // Print out the program usage
        USAGE(argv[0]);
    }

    return return_code;
}

int validate_args(const char *input_path, const char *output_path, int *in_encoding, const int out_encoding) {

    int return_code = FAILED;

    /* Make sure both strings are not NULL */
    if(input_path != NULL && output_path != NULL) {
        /* Check to see if the the input and output are two different files. */
        if(strcmp(input_path, output_path) != 0) {
            /* Check to see if the input file exists */
            struct stat *instat = (struct stat*)malloc(sizeof(struct stat));
            /* zero out the memory */
            memset(instat, 0, sizeof(struct stat));
            /* now check to see if the file exists */
            if(stat(input_path, instat) == -1) {
                /* something went wrong */
                if(errno == ENOENT) {
                    /* File does not exist. */
                    return_code = FILE_DNE;
                } else {
                    /* No idea what the error is. */
                    perror(NULL);
                }
            } else {
                return_code = VALID_ARGS;
            }

            // Check to see that output file is not a symbolic link of input file.
            struct stat *outstat = (struct stat*)malloc(sizeof(struct stat));
            /* zero out the memory */
            memset(outstat, 0, sizeof(struct stat));
            /* now check to see if the file exists */
            if(stat(output_path, outstat) == -1) {
                // file might not exist
                if(errno == ENOENT) {
                    /* File does not exist. */
                    // file does not exist. everything is fine.
                } else {
                    /* No idea what the error is. */
                   // perror(NULL);
                }
            } else {
                // output file exists. check to make sure it is not the same as input.
                int in_ino = (int)instat->st_ino;
                int in_dev = (int)instat->st_dev;
                int out_ino = (int)outstat->st_ino;
                int out_dev = (int)outstat->st_dev;

                if (in_ino == out_ino && in_dev == out_dev) {
                    // symbolic link!
                    return_code = SAME_FILE;
                }
            }

            free(instat);
            free(outstat);

        } else {
            // the are the same file
            return_code = SAME_FILE;
        }
    }

    // If everything is good so far, we can check to see if input has a valid BOM.
    if (return_code == VALID_ARGS) {
        int input_fd = -1;
        input_fd = open(input_path, O_RDONLY);

        ssize_t bytes_read;
        unsigned char bytes[3];

        if(input_fd >= 0) {
            int count = 0; // want to stop reading once count = 3;
            while((bytes_read = read(input_fd, &bytes[count], 1)) == 1) {
                if (count >= 3)
                    break;
                count++;
            }

            // now we can check bytes.
            if (bytes[0] == 0XEF && bytes[1] == 0XBB && bytes[2] == 0xBF) 
                *in_encoding = UTF8;
            else if (bytes[0] == 0xFF && bytes[1] == 0xFE)
                *in_encoding = UTF16LE;
            else if (bytes[0] == 0xFE && bytes[1] == 0xFF)
                *in_encoding = UTF16BE;
            else {
                // invalid BOM.
                return_code = INVALID_BOM;
            }

            /*
            int i;
            printf("BOM: ");
            for (i = 0; i<count; i++) {
                printf("%x", bytes[i]);
            }

            printf("\n");
            */
        } else {
            return_code = INVALID_BOM;
        }

        // Check to see if encodings are the same
        if (*in_encoding == out_encoding)
            return_code = SAME_BOM;

        //return_code = INVALID_BOM;
        /*
        printf("inBOM: %d\n", *in_encoding);
        printf("outBOM: %d\n", out_encoding);
        */

        close(input_fd);
    }

    return return_code;
}

bool convert(const int input_fd, const int output_fd, const int in_encoding, const int out_encoding) {
    if (in_encoding == out_encoding)
        return false;

    if (vflag == 1) {
        printf("+---------+------------+----------------+\n");
        printf("|  ASCII  | # of bytes | codepoint      |\n");
        printf("+---------+------------+----------------+\n");

    } else if (vflag == 2) {
        printf("+---------+------------+----------------+---------------+\n");
        printf("|  ASCII  | # of bytes | codepoint      |  input        |\n");
        printf("+---------+------------+----------------+---------------+\n");
    } else if (vflag >= 3) {
        printf("+---------+------------+----------------+---------------+---------------+\n");
        printf("|  ASCII  | # of bytes | codepoint      |  input        |  output       |\n");
        printf("+---------+------------+----------------+---------------+---------------+\n");
    }

    bool success = false;

    // UTF-8 to UTF-16LE
    switch(in_encoding) {
        case UTF8:
            if (out_encoding == UTF16LE)
                success = convert_UTF8_to_UTF16LE(input_fd, output_fd);
            else if (out_encoding == UTF16BE)
                success = convert_UTF8_to_UTF16BE(input_fd, output_fd);
            break;
        case UTF16LE:
            if (out_encoding == UTF8)
                success = convert_UTF16LE_to_UTF8(input_fd, output_fd);
            else if (out_encoding == UTF16BE)
                success = convert_UTF16LE_to_UTF16BE(input_fd, output_fd);
            break;
        case UTF16BE:
            if (out_encoding == UTF8)
                success = convert_UTF16BE_to_UTF8(input_fd, output_fd);
            else if (out_encoding == UTF16LE)
                success = convert_UTF16BE_to_UTF16LE(input_fd, output_fd);
        default:
            break;
    }

    return success;
}

bool convert_UTF8_to_UTF16LE(const int input_fd, const int output_fd) {
    bool success = false;
    if(input_fd >= 0 && output_fd >= 0) {
        /* UTF-8 encoded text can be @ most 4-bytes */
        uint8_t bytes[4];
        uint8_t read_value;
        size_t count = 0;
        ssize_t bytes_read;
        bool encode = false, parse_error = false;
        uint32_t input = 0;
        int num_bytes_allowed = 0; // number of bytes allowed in the character
        /* Read in UTF-8 Bytes */
        while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
            /* Mask the most significate bit of the byte */
            unsigned char masked_value = read_value & 0x80;
            if(masked_value != 0x80) { 
                /* US-ASCII */
                if(count == 0) {
                    /* US-ASCII */
                    bytes[count++] = read_value;
                    input = read_value;
                    encode = true;
                } else {
                    /* Found an ASCII character but theres other characters
                     * in the buffer already.
                     * Set the file position back 1 byte.
                     */
                    if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                        /* failed to move the file pointer back */
                        parse_error = true;
                        perror(NULL);
                        goto conversion_done;
                    }
                    /* Enocde the current values into UTF-16LE */
                    encode = true;
                }
            } else {
                if((read_value & UTF8_4_BYTE) == UTF8_4_BYTE ||
                   (read_value & UTF8_3_BYTE) == UTF8_3_BYTE ||
                   (read_value & UTF8_2_BYTE) == UTF8_2_BYTE) {
                    // set num_bytes_allowed
                    if ((read_value & UTF8_4_BYTE) == UTF8_4_BYTE)
                        num_bytes_allowed = 4;
                    else if ((read_value & UTF8_3_BYTE) == UTF8_3_BYTE)
                        num_bytes_allowed = 3;
                    else if ((read_value & UTF8_2_BYTE) == UTF8_2_BYTE)
                        num_bytes_allowed = 2;

                    // Check to see which byte we have encountered
                    if(count == 0) {
                        bytes[count++] = read_value;
                        input = (input << 4) | read_value;
                    } else {
                        /* Set the file position back 1 byte */
                        if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                            /* failed to move the file pointer back */
                            parse_error = true;
                            perror(NULL);
                            goto conversion_done;
                        }
                        /* Encode the current values into UTF-16LE */
                        encode = true;
                    }
                } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                    /* continuation byte */
                    bytes[count++] = read_value;
                    input = (input << 4) | read_value;

                    // Check if we are done with this character by comparing count with the number of bytes allowed;
                    if (count == num_bytes_allowed)
                        encode = true;
                }
            }
            /* If its time to encode do it here */
            if(encode) {

                char ascii = '\0';
                int32_t output = 0;

                int i;
                uint32_t value = 0; // code point value
                bool isAscii;
                for(i = 0; i < count; i++) {
                    isAscii = false; // set to false at the beginning of every loop.
                    if(i == 0) {
                        if((bytes[i] & UTF8_4_BYTE) == UTF8_4_BYTE) {
                            value = bytes[i] & 0x7;
                        } else if((bytes[i] & UTF8_3_BYTE) == UTF8_3_BYTE) {
                            value =  bytes[i] & 0xF;
                        } else if((bytes[i] & UTF8_2_BYTE) == UTF8_2_BYTE) {
                            value =  bytes[i] & 0x1F;
                        } else if((bytes[i] & 0x80) == 0) {
                            /* Value is an ASCII character */
                            value = bytes[i];
                            isAscii = true;
                        } else {
                            /* Marker byte is incorrect */
                            parse_error = true;
                            goto conversion_done;
                        }
                    } else {
                        if(!isAscii) {
                            value = (value << 6) | (bytes[i] & 0x3F);
                        } else {
                            /* How is there more bytes if we have an ascii char? */
                            parse_error = true;
                            goto conversion_done;
                        }
                    }
                }
                /* Now we can use the code point to convert get the UTF-16 value. */
                /* Handle the value if its a surrogate pair*/
                if(value >= SURROGATE_PAIR) {
                    /*
                    printf("value: %x\n", value);
                    printf("CODE POINT: %x\n", value);
                    */
                    uint32_t vprime; /* v` = v - 0x10000 **/
                    vprime = value - SURROGATE_PAIR; /* subtract the constant from value */
                    uint32_t w1 = (vprime >> 10) + 0xD800; // MSB
                    uint32_t w2 =  (vprime & 0x3FF) + 0xDC00; // LSB

                    /*
                    printf("w1: %x\n", w1);
                    printf("w2: %x\n", w2);
                    */

                    /* write the surrogate pair w1 to file */
                    if(!safe_write(input_fd, output_fd, &w1, 2)) {
                        parse_error = true;
                        goto conversion_done;
                    }
                    /* write the surrogate pair w2 to file */
                    if(!safe_write(input_fd, output_fd, &w2, 2)) {
                        parse_error = true;
                        goto conversion_done;
                    }
                } else {
                    
                    /* write the codeunit to file */
                    // must write 'count' times.
                    // store value in byte array so we can print it
                    uint8_t value_array[4];
                    value_array[2] = (value & 0xFF); 
                    value_array[3] = ((value >> 8) & 0xFF); 
                    value_array[0] = ((value >> 16) & 0xFF); 
                    value_array[1] = ((value >> 24) & 0xFF); 

                    // we must always print at least 2 bytes. However, if we have more, we must print more(up to 3).
                    if (value_array[1] != 0) {

                        int i = 1;

                        output = (value_array[1] << 16) | (value_array[2] << 8) | value_array[3];

                        for (; i <= 3; i++) {
                            if(!safe_write(input_fd, output_fd, &value_array[i], 1)) {
                                parse_error = true;
                                goto conversion_done;
                            }
                        }
                    } else {
                        int i = 2;

                        if (value_array[2] <= 0x7F)
                            ascii = value_array[2];

                        output = (value_array[2] << 8) | value_array[3];

                        for (; i <= 3; i++) {
                            if(!safe_write(input_fd, output_fd, &value_array[i], 1)) {
                                parse_error = true;
                                goto conversion_done;
                            }
                            //printf("value: %x\n", value_array[i]);
                        }
                    }                   

                }

                // ascii
                // #ofbytes = num_bytes_allowed
                // codepoint = value
                // input
                // output
                if (vflag == 1) {

                    if (ascii == '\0')
                        printf("|  NONE   +     %d      |   U+%04x\t|\n", num_bytes_allowed, value);
                    else if (ascii == '\n')
                        printf("|    \\n   +     %d      |   U+%04x\t|\n", num_bytes_allowed, value);
                    else if (ascii == '\t')
                        printf("|    \\t   +     %d      |   U+%04x\t|\n", num_bytes_allowed, value);
                    else
                        printf("|    %c    +     %d      |   U+%04x\t|\n", ascii, num_bytes_allowed, value);
                    printf("+---------+------------+----------------+\n");
                } else if (vflag == 2) {
                    if (ascii == '\0')
                        printf("|  NONE   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes_allowed, value, input);
                    else if (ascii == '\n')
                        printf("|    \\n   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes_allowed, value, input);
                    else if (ascii == '\t')
                        printf("|    \\t   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes_allowed, value, input);
                    else
                        printf("|    %c    +     %d      |   U+%04x\t|  0x%x   \t|\n", ascii, num_bytes_allowed, value, input);

                    printf("+---------+------------+----------------+---------------+\n");
                } else if (vflag >= 3) {
                    if (ascii == '\0')
                        printf("|  NONE   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes_allowed, value, input, output );
                    else if (ascii == '\n')
                        printf("|    \\n   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes_allowed, value, input, output);
                    else if (ascii == '\t')
                        printf("|    \\t   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes_allowed, value, input, output);
                    else
                        printf("|    %c    +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", ascii, num_bytes_allowed, value, input, output);
                    printf("+---------+------------+----------------+---------------+---------------+\n");
                }



                /* Done encoding the value to UTF-16BE */
                encode = false;
                count = 0;
                num_bytes_allowed = 0;
            }
        }
        /* If we got here the operation was a success! */
        success = true;
    }
conversion_done:
    return success;
}

bool convert_UTF8_to_UTF16BE(const int input_fd, const int output_fd)
{
    bool success = false;
    if(input_fd >= 0 && output_fd >= 0) {
        /* UTF-8 encoded text can be @ most 4-bytes */
        uint8_t bytes[4];
        uint8_t read_value;
        size_t count = 0;
        uint32_t input = 0;
        ssize_t bytes_read;
        bool encode = false, parse_error = false;
        int num_bytes_allowed = 0; // number of bytes allowed in the character
        /* Read in UTF-8 Bytes */
        while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
            /* Mask the most significate bit of the byte */
            unsigned char masked_value = read_value & 0x80;
            if(masked_value != 0x80) { 
                /* US-ASCII */
                if(count == 0) {
                    /* US-ASCII */
                    bytes[count++] = read_value;
                    input = read_value;
                    encode = true;
                } else {
                    /* Found an ASCII character but theres other characters
                     * in the buffer already.
                     * Set the file position back 1 byte.
                     */
                    if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                        /* failed to move the file pointer back */
                        parse_error = true;
                        perror(NULL);
                        goto conversion_done;
                    }
                    /* Enocde the current values into UTF-16LE */
                    encode = true;
                }
            } else {
                if((read_value & UTF8_4_BYTE) == UTF8_4_BYTE ||
                   (read_value & UTF8_3_BYTE) == UTF8_3_BYTE ||
                   (read_value & UTF8_2_BYTE) == UTF8_2_BYTE) {
                    // set num_bytes_allowed
                    if ((read_value & UTF8_4_BYTE) == UTF8_4_BYTE)
                        num_bytes_allowed = 4;
                    else if ((read_value & UTF8_3_BYTE) == UTF8_3_BYTE)
                        num_bytes_allowed = 3;
                    else if ((read_value & UTF8_2_BYTE) == UTF8_2_BYTE)
                        num_bytes_allowed = 2;

                    // Check to see which byte we have encountered
                    if(count == 0) {
                        bytes[count++] = read_value;
                        input = (input << 4) | read_value;
                    } else {
                        /* Set the file position back 1 byte */
                        if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                            /* failed to move the file pointer back */
                            parse_error = true;
                            perror(NULL);
                            goto conversion_done;
                        }
                        /* Encode the current values into UTF-16LE */
                        encode = true;
                    }
                } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                    /* continuation byte */
                    bytes[count++] = read_value;
                    input = (input << 4) | read_value;

                    // Check if we are done with this character by comparing count with the number of bytes allowed;
                    if (count == num_bytes_allowed)
                        encode = true;
                }
            }
            /* If its time to encode do it here */
            if(encode) {
                char ascii = '\0';
                int i;
                int32_t output = 0;
                uint32_t value = 0; // code point value
                bool isAscii;
                for(i = 0; i < count; i++) {
                    isAscii = false; // set to false at the beginning of every loop.
                    if(i == 0) {
                        if((bytes[i] & UTF8_4_BYTE) == UTF8_4_BYTE) {
                            value = bytes[i] & 0x7;
                        } else if((bytes[i] & UTF8_3_BYTE) == UTF8_3_BYTE) {
                            value =  bytes[i] & 0xF;
                        } else if((bytes[i] & UTF8_2_BYTE) == UTF8_2_BYTE) {
                            value =  bytes[i] & 0x1F;
                        } else if((bytes[i] & 0x80) == 0) {
                            /* Value is an ASCII character */
                            value = bytes[i];
                            isAscii = true;
                        } else {
                            /* Marker byte is incorrect */
                            parse_error = true;
                            goto conversion_done;
                        }
                    } else {
                        if(!isAscii) {
                            value = (value << 6) | (bytes[i] & 0x3F);
                        } else {
                            /* How is there more bytes if we have an ascii char? */
                            parse_error = true;
                            goto conversion_done;
                        }
                    }
                }
                /* Now we can use the code point to convert get the UTF-16 value. */
                /* Handle the value if its a surrogate pair*/
                if(value >= SURROGATE_PAIR) {
                    /*
                    printf("value: %x\n", value);
                    printf("CODE POINT: %x\n", value);
                    */
                    uint32_t vprime; /* v` = v - 0x10000 **/
                    vprime = value - SURROGATE_PAIR; /* subtract the constant from value */
                    uint32_t w1 = (vprime >> 10) + 0xD800; // MSB
                    uint32_t w2 =  (vprime & 0x3FF) + 0xDC00; // LSB

                    /*
                    printf("w1: %x\n", w1);
                    printf("w2: %x\n", w2);
                    */

                    // reformat for UTF-16BE
                    uint32_t msb = (w1 >> 8) | ((w1 & 0xFF) << 8);
                    uint32_t lsb = (w2 >> 8) | ((w2 & 0xFF) << 8);

                    /*
                    printf("MSB: %x\n", msb);
                    printf("LSB: %x\n", lsb);
                    */

                    /* write the surrogate pair w1 to file */
                    if(!safe_write(input_fd, output_fd, &msb, 2)) {
                        parse_error = true;
                        goto conversion_done;
                    }
                    /* write the surrogate pair w2 to file */
                    if(!safe_write(input_fd, output_fd, &lsb, 2)) {
                        parse_error = true;
                        goto conversion_done;
                    }
                } else {
                    
                    /* write the codeunit to file */
                    // must write 'count' times.
                    // store value in byte array so we can print it
                    uint8_t value_array[4];
                    value_array[3] = (value & 0xFF); 
                    value_array[2] = ((value >> 8) & 0xFF); 
                    value_array[1] = ((value >> 16) & 0xFF); 
                    value_array[0] = ((value >> 24) & 0xFF); 

                    // we must always print at least 2 bytes. However, if we have more, we must print more(up to 3).
                    if (value_array[1] != 0) {

                        int i = 1;

                        output = (value_array[1] << 16) | (value_array[2] << 8) | value_array[3];

                        for (; i <= 3; i++) {
                            if(!safe_write(input_fd, output_fd, &value_array[i], 1)) {
                                parse_error = true;
                                goto conversion_done;
                            }
                        }
                    } else {
                        int i = 2;

                        output = (value_array[2] << 8) | value_array[3];

                        if (value_array[3] <= 0x7F)
                            ascii = value_array[3];

                        for (; i <= 3; i++) {
                            if(!safe_write(input_fd, output_fd, &value_array[i], 1)) {
                                parse_error = true;
                                goto conversion_done;
                            }
                            //printf("value: %x\n", value_array[i]);
                        }
                    }                   

                }

                // ascii
                // #ofbytes = num_bytes_allowed
                // codepoint = value
                // input
                // output
                if (vflag == 1) {

                    if (ascii == '\0')
                        printf("|  NONE   +     %d      |   U+%04x\t|\n", num_bytes_allowed, value);
                    else if (ascii == '\n')
                        printf("|    \\n   +     %d      |   U+%04x\t|\n", num_bytes_allowed, value);
                    else if (ascii == '\t')
                        printf("|    \\t   +     %d      |   U+%04x\t|\n", num_bytes_allowed, value);
                    else
                        printf("|    %c    +     %d      |   U+%04x\t|\n", ascii, num_bytes_allowed, value);
                    printf("+---------+------------+----------------+\n");
                } else if (vflag == 2) {
                    if (ascii == '\0')
                        printf("|  NONE   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes_allowed, value, input);
                    else if (ascii == '\n')
                        printf("|    \\n   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes_allowed, value, input);
                    else if (ascii == '\t')
                        printf("|    \\t   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes_allowed, value, input);
                    else
                        printf("|    %c    +     %d      |   U+%04x\t|  0x%x   \t|\n", ascii, num_bytes_allowed, value, input);

                    printf("+---------+------------+----------------+---------------+\n");
                } else if (vflag >= 3) {
                    if (ascii == '\0')
                        printf("|  NONE   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes_allowed, value, input, output );
                    else if (ascii == '\n')
                        printf("|    \\n   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes_allowed, value, input, output);
                    else if (ascii == '\t')
                        printf("|    \\t   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes_allowed, value, input, output);
                    else
                        printf("|    %c    +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", ascii, num_bytes_allowed, value, input, output);
                    printf("+---------+------------+----------------+---------------+---------------+\n");
                }





                /* Done encoding the value to UTF-16BE */
                encode = false;
                count = 0;
                num_bytes_allowed = 0;
            }
        }
        /* If we got here the operation was a success! */
        success = true;
    }
conversion_done:
    return success;
}


bool convert_UTF16LE_to_UTF8(const int input_fd, const int output_fd) {
    bool success = false;
    // first, print the BOM for UTF08.
    uint8_t bom[3];
    bom[0] = 0xEF;
    bom[1] = 0xBB;
    bom[2] = 0xBF;

    char ascii = '\0';
    uint32_t input = 0;
    uint32_t output = 0;

    int i = 0;
    for(; i < 3; i++) {
        safe_write(input_fd, output_fd, &bom[i], 1);
    }

    // now we do the converting
    /* UTF-16 encoded text can be @ most 4-bytes */
    int8_t bytes[4]; 
    uint32_t read_value;
    uint32_t read_value2;
    //size_t count = 0;
    ssize_t bytes_read;
    //bool encode = false;
    //int num_bytes_allowed = 0; // number of bytes allowed in the character

    // skip the BOM
    bytes_read = read(input_fd, &read_value, 2);

    int num_bytes = 0; // number of bytes we are converting.

    // read 2 bytes at a time
    while((bytes_read = read(input_fd, &read_value, 2)) == 2) {
        int32_t masked_value = read_value & 0xFF00;
        if (masked_value == 0x0) {
            // 1 byte value padded with 00
            num_bytes = 1;

            input = read_value;

            // printf("1 byte: %x\n", read_value);

        } else if ((masked_value & 0x8000) == 0x8000) {
            // must read next 2 bytes.
            bytes_read = read(input_fd, &read_value2, 2);
            read_value2 = read_value2 & 0xFFFF;
            // 4 byte value
            num_bytes = 4;

            input = (read_value << 4) | read_value2;

            // printf("4 byte: %x", read_value);
            // printf(" %x\n", read_value2);
        } else {
            // 2 byte value
            num_bytes = 2;

            input = read_value;

            // printf("2 byte: %x\n", read_value);
        }

        // now we know how many bytes this thing took.
        if (num_bytes == 4) {
            // surrogate
            // convert to proper code point

            uint32_t msb = read_value & 0xFFFF;
            uint32_t lsb = read_value2 & 0xFFFF;

            uint32_t vprime;
            vprime = (lsb - 0xDC00)  | ((msb - 0xD800) << 10);

            uint32_t value = vprime + 0x10000;

            read_value = value;

            // printf("MSB = %x\n", msb);
            // printf("LSB = %x\n", lsb);


            // 000xxxyy yyyyzzzz zzwwwwww
            // start out by making bytes 11110xxx 10yyyyyy 10zzzzzz 10wwwwww
            bytes[0] = UTF8_4_BYTE;
            bytes[1] = UTF8_CONT;
            bytes[2] = UTF8_CONT;
            bytes[3] = UTF8_CONT;

            bytes[3] = bytes[3] | (value & 0x3F);
            bytes[2] = bytes[2] | ((value & 0x0FC0) >> 6);
            bytes[1] = bytes[1] | ((value & 0x3F000) >> 12);
            bytes[0] = bytes[0] | ((value & 0x1C0000) >> 18);

            output = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

            if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                goto conversion_done;
            }
            if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                goto conversion_done;
            }
            if(!safe_write(input_fd, output_fd, &bytes[2], 1)) {
                goto conversion_done;
            }
            if(!safe_write(input_fd, output_fd, &bytes[3], 1)) {
                goto conversion_done;
            }           

        } else if (num_bytes == 2) {
            // proper code point
            // check to see if we are converting to 2 bytes or 3 bytes

            if ((read_value & 0xFFFF) <= 0x7FF ) {
                // 2 bytes
                // 00000xxx xxyyyyyy
                // start out by making bytes 110xxxxx 10yyyyyy
                bytes[0] = UTF8_2_BYTE;
                bytes[1] = UTF8_CONT;

                bytes[1] = bytes[1] | (read_value & 0x3F);
                bytes[0] = bytes[0] | ((read_value & 0x07C0) >> 6);

                if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                    goto conversion_done;
                }
            } else {
                // 3 bytes
                // xxxxyyyy yyzzzzzz
                // start out by making bytes 1110xxxx 10yyyyyy 10zzzzzz
                bytes[0] = UTF8_3_BYTE;
                bytes[1] = UTF8_CONT;
                bytes[2] = UTF8_CONT;

                bytes[2] = bytes[2] | (read_value & 0x3F);
                bytes[1] = bytes[1] | ((read_value & 0x0FC0) >> 6);
                bytes[0] = bytes[0] | ((read_value & 0xF000) >> 12);

                output = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];

                if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[2], 1)) {
                    goto conversion_done;
                }
            }


        } else { // num_bytes == 1
            // proper code point
            // ASCII goes up to 7F
            if ((read_value & 0x0000FF) <= 0x7F) {
                ascii = read_value;

                output = ascii;

                // just print normally
                if(!safe_write(input_fd, output_fd, &read_value, 1)) {
                    goto conversion_done;
                }
            } else {
                // convert
                //xxyyyyyy
                // start out by making bytes 110xxxxx 10yyyyyy
                bytes[0] = UTF8_2_BYTE;
                bytes[1] = UTF8_CONT;

                bytes[1] = bytes[1] | (read_value & 0x3F);
                bytes[0] = bytes[0] | ((read_value & 0xC0) >> 6);

                output = (bytes[0] << 8) | bytes[1];

                if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                    goto conversion_done;
                }
            }



        }

        // ascii
        // #ofbytes = num_bytes_allowed
        // codepoint = value
        // input
        // output
        if (vflag == 1) {

            if (ascii == '\0')
                printf("|  NONE   +     %d      |   U+%04x\t|\n", num_bytes, read_value);
            else if (ascii == '\n')
                printf("|    \\n   +     %d      |   U+%04x\t|\n", num_bytes, read_value);
            else if (ascii == '\t')
                printf("|    \\t   +     %d      |   U+%04x\t|\n", num_bytes, read_value);
            else
                printf("|    %c    +     %d      |   U+%04x\t|\n", ascii, num_bytes, read_value);
            printf("+---------+------------+----------------+\n");
        } else if (vflag == 2) {
            if (ascii == '\0')
                printf("|  NONE   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes, read_value, input);
            else if (ascii == '\n')
                printf("|    \\n   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes, read_value, input);
            else if (ascii == '\t')
                printf("|    \\t   +     %d      |   U+%04x\t|  0x%x   \t|\n", num_bytes, read_value, input);
            else
                printf("|    %c    +     %d      |   U+%04x\t|  0x%x   \t|\n", ascii, num_bytes, read_value, input);

            printf("+---------+------------+----------------+---------------+\n");
        } else if (vflag >= 3) {
            if (ascii == '\0')
                printf("|  NONE   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes, read_value, input, output);
            else if (ascii == '\n')
                printf("|    \\n   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes, read_value, input, output);
            else if (ascii == '\t')
                printf("|    \\t   +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", num_bytes, read_value, input, output);
            else
                printf("|    %c    +     %d      |   U+%04x\t|  0x%x   \t| 0x%04x\t|\n", ascii, num_bytes, read_value, input, output);
            printf("+---------+------------+----------------+---------------+---------------+\n");
        }
        

    }



    success = true;

conversion_done:
    return success;
}

bool convert_UTF16LE_to_UTF16BE(const int input_fd, const int output_fd) {
    bool success = false;
    if(input_fd >= 0 && output_fd >= 0) {

        // swap every by pair of bytes. 
        // ex: ff fe becomes fe ff.
        ssize_t bytes_read;
        unsigned char bytes[2];
        unsigned char read_value;

        int pair = 0;

        while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
            if (pair == 0) {
                bytes[1] = read_value;
                pair = 1;
            }
            else {
                bytes[0] = read_value;
                pair = 0;

                // output to file at the end of every pair
                if(!safe_write(input_fd, output_fd, &bytes, 2)) {
                    goto conversion_done;
                }

            }
        }
        /* If we got here the operation was a success! */
        success = true;
    }
conversion_done:
    return success;
}

bool convert_UTF16BE_to_UTF8(const int input_fd, const int output_fd) {
    bool success = false;
    // first, print the BOM for UTF08.
    uint8_t bom[3];
    bom[0] = 0xEF;
    bom[1] = 0xBB;
    bom[2] = 0xBF;

    int i = 0;
    for(; i < 3; i++) {
        safe_write(input_fd, output_fd, &bom[i], 1);
    }

    // now we do the converting
    /* UTF-16 encoded text can be @ most 4-bytes */
    uint8_t bytes[4]; 
    uint32_t read_value;
    uint32_t read_value2;
    //size_t count = 0;
    ssize_t bytes_read;
    //bool encode = false;
    //int num_bytes_allowed = 0; // number of bytes allowed in the character

    // skip the BOM
    bytes_read = read(input_fd, &read_value, 2);

    int num_bytes = 0; // number of bytes we are converting.

    // read 2 bytes at a time
    while((bytes_read = read(input_fd, &read_value, 2)) == 2) {
        // swap read_value's byte pairs and treat as if LE
        read_value = read_value & 0xFFFF;
        read_value = ((read_value >> 8) | (read_value << 8)) & 0xFFFF;

        int32_t masked_value = read_value & 0xFF00;
        if (masked_value == 0x0) {
            // 1 byte value padded with 00
            num_bytes = 1;

            // printf("1 byte: %x\n", read_value);

        } else if ((masked_value & 0x8000) == 0x8000) {
            // must read next 2 bytes.
            bytes_read = read(input_fd, &read_value2, 2);
            read_value2 = read_value2 & 0xFFFF;
            read_value2 = ((read_value2 >> 8) | (read_value2 << 8)) & 0xFFFF;

            // 4 byte value
            num_bytes = 4;

            // printf("4 byte: %x", read_value);
            // printf(" %x\n", read_value2);
        } else {
            // 2 byte value
            num_bytes = 2;

            // printf("2 byte: %x\n", read_value);
        }

        // now we know how many bytes this thing took.
        if (num_bytes == 4) {
            // surrogate
            // convert to proper code point


            uint32_t msb = read_value & 0xFFFF;
            uint32_t lsb = read_value2 & 0xFFFF;

            uint32_t vprime;
            vprime = (lsb - 0xDC00)  | ((msb - 0xD800) << 10);

            uint32_t value = vprime + 0x10000;

            // printf("MSB = %x\n", msb);
            // printf("LSB = %x\n", lsb);


            // 000xxxyy yyyyzzzz zzwwwwww
            // start out by making bytes 11110xxx 10yyyyyy 10zzzzzz 10wwwwww
            bytes[0] = UTF8_4_BYTE;
            bytes[1] = UTF8_CONT;
            bytes[2] = UTF8_CONT;
            bytes[3] = UTF8_CONT;

            bytes[3] = bytes[3] | (value & 0x3F);
            bytes[2] = bytes[2] | ((value & 0x0FC0) >> 6);
            bytes[1] = bytes[1] | ((value & 0x3F000) >> 12);
            bytes[0] = bytes[0] | ((value & 0x1C0000) >> 18);


            if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                goto conversion_done;
            }
            if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                goto conversion_done;
            }
            if(!safe_write(input_fd, output_fd, &bytes[2], 1)) {
                goto conversion_done;
            }
            if(!safe_write(input_fd, output_fd, &bytes[3], 1)) {
                goto conversion_done;
            }           

        } else if (num_bytes == 2) {
            // proper code point
            // check to see if we are converting to 2 bytes or 3 bytes

            if ((read_value & 0xFFFF) <= 0x7FF ) {
                // 2 bytes
                // 00000xxx xxyyyyyy
                // start out by making bytes 110xxxxx 10yyyyyy
                bytes[0] = UTF8_2_BYTE;
                bytes[1] = UTF8_CONT;

                bytes[1] = bytes[1] | (read_value & 0x3F);
                bytes[0] = bytes[0] | ((read_value & 0x07C0) >> 6);

                if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                    goto conversion_done;
                }
            } else {
                // 3 bytes
                // xxxxyyyy yyzzzzzz
                // start out by making bytes 1110xxxx 10yyyyyy 10zzzzzz
                bytes[0] = UTF8_3_BYTE;
                bytes[1] = UTF8_CONT;
                bytes[2] = UTF8_CONT;

                bytes[2] = bytes[2] | (read_value & 0x3F);
                bytes[1] = bytes[1] | ((read_value & 0x0FC0) >> 6);
                bytes[0] = bytes[0] | ((read_value & 0xF000) >> 12);

                if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[2], 1)) {
                    goto conversion_done;
                }
            }


        } else { // num_bytes == 1
            // proper code point
            // ASCII goes up to 7F
            if ((read_value & 0x0000FF) <= 0x7F) {
                // just print normally
                if(!safe_write(input_fd, output_fd, &read_value, 1)) {
                    goto conversion_done;
                }
            } else {
                // convert
                //xxyyyyyy
                // start out by making bytes 110xxxxx 10yyyyyy
                bytes[0] = UTF8_2_BYTE;
                bytes[1] = UTF8_CONT;

                bytes[1] = bytes[1] | (read_value & 0x3F);
                bytes[0] = bytes[0] | ((read_value & 0xC0) >> 6);

                if(!safe_write(input_fd, output_fd, &bytes[0], 1)) {
                    goto conversion_done;
                }
                if(!safe_write(input_fd, output_fd, &bytes[1], 1)) {
                    goto conversion_done;
                }
            }
        }

    }


    success = true;

conversion_done:
    return success;
}

bool convert_UTF16BE_to_UTF16LE(const int input_fd, const int output_fd) {
    // same thing as converting from UTF-16LE TO utf-16BE
    return convert_UTF16LE_to_UTF16BE(input_fd, output_fd);
}

bool safe_write(const int input_fd, const int output_fd, void *value, size_t size)
{
    bool success = true;
    ssize_t bytes_written;
    if((bytes_written = write(output_fd, value, size)) != size) {
        /* The write operation failed */
        fprintf(stderr, "Write to file failed. Expected %zu bytes but got %zd\n", size, bytes_written);
        success = false;
    }
    return success;
}
