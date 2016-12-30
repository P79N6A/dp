#include "StringSplitTools.h"

void StringSplitTools::trimString(const std::string& input,
        const std::string& trim_chars,
        std::string& output)
{
    // Find the edges of leading/trailing whitespace as desired.
    size_t first_good_char = input.find_first_not_of(trim_chars);
    size_t last_good_char = input.find_last_not_of(trim_chars);

    // When the string was all whitespace, report that we stripped off whitespace
    // from whichever position the caller was interested in.  For empty input, we
    // stripped no whitespace, but we still need to clear |output|.
    if (input.empty() ||
            (first_good_char == std::string::npos) || 
            (last_good_char == std::string::npos))
    {
        output.clear();
    }
    else
    {
        // Trim the whitespace.
        output =
            input.substr(first_good_char, last_good_char - first_good_char + 1);
    }

    return;
}

void StringSplitTools::splitString(const std::string& str,
        char delim,
        std::vector<std::string>& r,
        bool trim_whitespace)
{
    size_t last = 0;
    size_t i;
    size_t num = str.size();
    for (i = 0; i <= num; ++i)
    {
        if (i == num || str[i] == delim)
        {
            size_t len = i - last;
            std::string tmp = str.substr(last, len);
            if(trim_whitespace)
            {
                std::string t_tmp;
                //trim write space char
                trimString(tmp, WHITESPACE_STRING, t_tmp);
                r.push_back(t_tmp);
            }
            else
            {
                r.push_back(tmp);
            }
            last = i + 1;
        }
    }
}

bool StringSplitTools::splitStringIntoKeyValues(
        const std::string& line,
        char key_value_delimiter,
        std::string& key, std::vector<std::string>& values)
{
    key.clear();
    values.clear();

    // Find the key string.
    size_t end_key_pos = line.find_first_of(key_value_delimiter);
    if (end_key_pos == std::string::npos)
    {
        printf("cannot parse string from line: %s\n", line.c_str());
        return false;    // no key
    }
    std::string key_tmp;
    key_tmp.assign(line, 0, end_key_pos);
    trimString(key_tmp, WHITESPACE_STRING, key);

    // Find the values string.
    std::string remains(line, end_key_pos, line.size() - end_key_pos);
    size_t begin_values_pos = remains.find_first_not_of(key_value_delimiter);
    if (begin_values_pos == std::string::npos)
    {
        printf("cannot parse value from line: %s\n", line.c_str());
        return false;   // no value
    }
    std::string values_string(remains, begin_values_pos,
            remains.size() - begin_values_pos);
    std::string dst_string;
    trimString(values_string, WHITESPACE_STRING, dst_string);
    // Construct the values vector.
    values.push_back(dst_string);
    return true;
}

bool StringSplitTools::splitStringIntoKeyValuePairs(
        const std::string& line,
        char key_value_delimiter,
        char key_value_pair_delimiter,
        std::vector<std::pair<std::string, std::string> >& kv_pairs)
{

    kv_pairs.clear();

    std::vector<std::string> pairs;
    splitString(line, key_value_pair_delimiter, pairs);

    bool success = true;
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        // Empty pair. SplitStringIntoKeyValues is more strict about an empty pair
        // line, so continue with the next pair.
        if (pairs[i].empty())
        {
            continue;
        }
        std::string key;
        std::vector<std::string> value;
        if (!splitStringIntoKeyValues(pairs[i],
                    key_value_delimiter,
                    key, value))
        {
            // Don't return here, to allow for keys without associated
            // values; just record that our split failed.
            success = false;
        }
        kv_pairs.push_back(make_pair(key, value.empty()? "" : value[0]));
    }
    return success;
}

