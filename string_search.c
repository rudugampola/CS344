#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);
int replace_if_at(char **str, size_t offset, char const *substring, char const *replacement);

int main(int argc, char *argv[])
{
    if (argc != 4)
        exit(1);
    char *line = NULL;
    size_t n = 0;
    getline(&line, &n, stdin);
    char *ret = replace_if_at(&line, 0, argv[1], argv[2]);
    if (!ret)
        exit(1);
    line = ret;
    printf("%s", line);
    free(line);

    return 0;
}

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub)
{
    char *str = *haystack;
    size_t haystack_len = strlen(str);
    size_t const needle_len = strlen(needle), sub_len = strlen(sub);

    for (; (str = strstr(str, needle));)
    {
        ptrdiff_t off = str - *haystack;
        if (sub_len > needle_len)
        {
            str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
            if (!str)
                goto exit;
            *haystack = str;
            str = *haystack + off;
        }
        memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
        memcpy(str, sub, sub_len);
        haystack_len = haystack_len + sub_len - needle_len;
        str += sub_len;
    }
    str = *haystack;
    if (sub_len < needle_len)
    {
        str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
        if (!str)
            goto exit;
        *haystack = str;
    }
exit:
    return str;
}

int replace_if_at(char **str, size_t offset, char const *substring, char const *replacement)
{
    size_t str_len = strlen(*str);
    size_t substring_len = strlen(substring);
    size_t replacement_len = strlen(replacement);
    char *temp;

    if (offset + substring_len > str_len)
    {
        return -1;
    }

    if (strncmp(*str + offset, substring, substring_len) != 0)
    {
        return 0;
    }

    if (replacement_len > substring_len)
    {
        temp = realloc(*str, str_len + replacement_len - substring_len + 1);
        if (!temp)
        {
            return -1;
        }
        *str = temp;
    }

    memmove(*str + offset + replacement_len, *str + offset + substring_len, str_len - offset - substring_len + 1);
    memcpy(*str + offset, replacement, replacement_len);

    return 1;
}