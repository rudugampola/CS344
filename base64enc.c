#include <stdio.h>
#include <stdint.h> // *should* typedef uint8_t
// Check that uint8_t type exists
#ifndef UINT8_MAX
#error "No support for uint8_t"
#endif
#include <string.h>
#include <errno.h>
#include <err.h>

// Base 64 encoder
int main(int argc, char *argv[])
{
    // Read the input data in blocks of three characters at a time
    // With no FILE, or when FILE is -, read standard input
    // Otherwise, read the file specified by the first argument
    FILE *fp = NULL;

    // Program prints error if invalid number of arguments is provided
    if (argc > 2)
    {
        fprintf(stderr, "Invalid number of arguments! "
                        "Usage: base64enc [FILE] or base64enc - for stdin.");
        return 1;
    }

    if (argc == 1 || strcmp(argv[1], "-") == 0)
    {
        // Read from standard input
        fp = stdin;
    }
    else
    {
        // Read from file
        fp = fopen(argv[1], "r");
    }

    if (fp == NULL || ferror(fp) || feof(fp))
    {
        fprintf(stderr, "Unable to open file '%s' for reading data to encode in base64!", argv[1]);
        return 1;
    }

    static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/=";

    // Read the data in blocks of three characters at a time
    uint8_t block[3];
    int numRead = 0; // Number of characters read
    int padding = 0; // Number of padding characters to add

    // Read the data in blocks of three characters at a time
    while (1)
    {
        // Read the next block of data from the file
        numRead = fread(block, 1, 3, fp);
        // Program prints error if file/stdin read fails
        if (ferror(fp) && !feof(fp))
        {
            err(errno, "fread");
        }
        if (numRead == 0 && feof(fp))
        {
            break;
        }

        uint8_t b0 = block[0];
        uint8_t b1 = block[1];
        uint8_t b2 = block[2];

        // Encode the first 6 bits of the first character
        putchar(alphabet[b0 >> 2]);

        // Encode the last 2 bits of the first character and the first 4 bits of the 
        // second character and OR the two together
        // 0x03 = 0 0 0 0   0 0 1 1
        putchar(alphabet[((b0 & 0x03) << 4) | (b1 >> 4)]);

        if (numRead == 1)
        {
            padding = 2;
        }
        else
        {
            // Encode the last 4 bits of the second character and the first 2 bits 
            // of the third character
            // 0x0F = 0 0 0 0   1 1 1 1
            putchar(alphabet[((b1 & 0x0F) << 2) | (b2 >> 6)]);
            if (numRead == 2)
            {
                padding = 1;
            }
            else
            {
                // Encode the last 6 bits of the third character
                // 0x3F = 0 0 1 1   1 1 1 1
                putchar(alphabet[b2 & 0x3F]);
            }
        }

        // Add padding if necessary
        if (padding > 0)
        {
            for (int i = 0; i < padding; i++)
            {
                putchar('=');
            }
        }

        // Add a newline every 76 characters
        static int count = 0;
        if (++count == 19)
        {
            putchar('\n');
            count = 0;
        }

        // Reset the padding
        padding = 0;

        // Reset the block
        block[0] = 0;
        block[1] = 0;
        block[2] = 0;
    }
    
    // Append a newline to the end of the output
    putchar('\n');

    // Close the FILE*
    fclose(fp);

    return 0;
}
