#include "helpers.hpp"

char *Helpers::itoa(int value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36)
    {
        *result = '\0';
        return result;
    }

    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do
    {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);

    // Apply negative sign
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void split(const std::string &str, const std::string &splitBy, std::vector<std::string> &tokens)
{
    /* Store the original string in the array, so we can loop the rest
     * of the algorithm. */
    tokens.push_back(str);

    // Store the split index in a 'size_t' (unsigned integer) type.
    size_t splitAt;
    // Store the size of what we're splicing out.
    size_t splitLen = splitBy.size();
    // Create a string for temporarily storing the fragment we're processing.
    std::string frag;
    // Loop infinitely - break is internal.
    while (true)
    {
        /* Store the last string in the vector, which is the only logical
         * candidate for processing. */
        frag = tokens.back();
        /* The index where the split is. */
        splitAt = frag.find(splitBy);
        // If we didn't find a new split point...
        if (splitAt == std::string::npos)
        {
            // Break the loop and (implicitly) return.
            break;
        }
        /* Put everything from the left side of the split where the string
         * being processed used to be. */
        tokens.back() = frag.substr(0, splitAt);
        /* Push everything from the right side of the split to the next empty
         * index in the vector. */
        tokens.push_back(frag.substr(splitAt + splitLen, frag.size() - (splitAt + splitLen)));
    }
}

std::vector<std::string> Helpers::split(const std::string &s, char delimiter)
{
    std::vector<std::string> parts;
    std::string part;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, part, delimiter))
    {
        parts.push_back(part);
    }
    return parts;
}

char *Helpers::appendChartoChar(const char *hostname, const char *def_host)
{
    // create hostname
    int numBytes = strlen(hostname) + strlen(def_host) + 1; // +1 for the null terminator | allocate a buffer of the required size
    char *hostname_str = NULL;
    resizeBuff(numBytes, &hostname_str);
    strcpy(hostname_str, hostname);
    strcat(hostname_str, def_host); // append default hostname to hostname
    return hostname_str;
}

char *Helpers::StringtoChar(const std::string &inputString)
{
    char *outputString;
    outputString = NULL;
    resizeBuff(inputString.length() + 1, &outputString);
    strcpy(outputString, inputString.c_str());
    return outputString;
}
