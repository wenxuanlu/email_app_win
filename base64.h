#include <iostream>
#include <string>
#include <sstream>
#include <codecvt>

using namespace std;

bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

// Base64 decode function
std::string base64_decode(const std::string& input) {
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::istringstream iss(input);
    std::string output;

    char a, b, c, d;

    while (iss.get(a) && is_base64(a)) {
        if (!(iss.get(b) && is_base64(b))) break;
        if (!(iss.get(c) && is_base64(c))) break;
        if (!(iss.get(d) && is_base64(d))) break;

        unsigned char b1 = base64_chars.find(a);
        unsigned char b2 = base64_chars.find(b);
        unsigned char b3 = base64_chars.find(c);
        unsigned char b4 = base64_chars.find(d);

        unsigned char combined = (b1 << 2) | (b2 >> 4);
        output += combined;

        if (c != '=') {
            combined = ((b2 & 0x0F) << 4) | (b3 >> 2);
            output += combined;
        }

        if (d != '=') {
            combined = ((b3 & 0x03) << 6) | b4;
            output += combined;
        }
    }

    return output;
}