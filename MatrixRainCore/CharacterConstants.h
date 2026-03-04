#pragma once






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

    // Half-width Katakana (U+FF65-U+FF9F): single-column-width characters
    extern const uint32_t HALFWIDTH_KATAKANA_CODEPOINTS[];
    extern const size_t   HALFWIDTH_KATAKANA_COUNT;

    // Get all codepoints in a single vector (132 total)
    std::vector<uint32_t> GetAllCodepoints();

    // Get codepoints suitable for console rendering (single-width only)
    std::vector<uint32_t> GetConsoleCodepoints();
}




