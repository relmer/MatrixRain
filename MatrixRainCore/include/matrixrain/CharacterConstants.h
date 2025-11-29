#pragma once






namespace MatrixRain
{
    namespace CharacterConstants
    {
        // Katakana characters (70 codepoints)
        extern const uint32_t KATAKANA_CODEPOINTS[];
        extern const size_t   KATAKANA_COUNT;

        // Latin uppercase letters (A-Z): 26 characters
        extern const uint32_t LATIN_UPPERCASE_CODEPOINTS[];
        extern const size_t   LATIN_UPPERCASE_COUNT;

        // Latin lowercase letters (a-z): 26 characters
        extern const uint32_t LATIN_LOWERCASE_CODEPOINTS[];
        extern const size_t   LATIN_LOWERCASE_COUNT;

        // Numerals (0-9): 10 characters
        extern const uint32_t NUMERAL_CODEPOINTS[];

        // Get all codepoints in a single vector (132 total)
        std::vector<uint32_t> GetAllCodepoints();
    }
}





