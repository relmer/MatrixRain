#include "pch.h"
#include "CharacterConstants.h"





namespace CharacterConstants
{
    // Katakana characters (72 codepoints from U+30A0 to U+30FF range)
    // These are the iconic characters from The Matrix
    const uint32_t KATAKANA_CODEPOINTS[] = {
        0x30A0, // ゠ Katakana-Hiragana Double Hyphen
        0x30A1, // ァ Small A
        0x30A2, // ア A
        0x30A3, // ィ Small I
        0x30A4, // イ I
        0x30A5, // ゥ Small U
        0x30A6, // ウ U
        0x30A7, // ェ Small E
        0x30A8, // エ E
        0x30A9, // ォ Small O
        0x30AA, // オ O
        0x30AB, // カ Ka
        0x30AC, // ガ Ga
        0x30AD, // キ Ki
        0x30AE, // ギ Gi
        0x30AF, // ク Ku
        0x30B0, // グ Gu
        0x30B1, // ケ Ke
        0x30B2, // ゲ Ge
        0x30B3, // コ Ko
        0x30B4, // ゴ Go
        0x30B5, // サ Sa
        0x30B6, // ザ Za
        0x30B7, // シ Shi
        0x30B8, // ジ Ji
        0x30B9, // ス Su
        0x30BA, // ズ Zu
        0x30BB, // セ Se
        0x30BC, // ゼ Ze
        0x30BD, // ソ So
        0x30BE, // ゾ Zo
        0x30BF, // タ Ta
        0x30C0, // ダ Da
        0x30C1, // チ Chi
        0x30C2, // ヂ Ji
        0x30C3, // ッ Small Tsu
        0x30C4, // ツ Tsu
        0x30C5, // ヅ Zu
        0x30C6, // テ Te
        0x30C7, // デ De
        0x30C8, // ト To
        0x30C9, // ド Do
        0x30CA, // ナ Na
        0x30CB, // ニ Ni
        0x30CC, // ヌ Nu
        0x30CD, // ネ Ne
        0x30CE, // ノ No
        0x30CF, // ハ Ha
        0x30D0, // バ Ba
        0x30D1, // パ Pa
        0x30D2, // ヒ Hi
        0x30D3, // ビ Bi
        0x30D4, // ピ Pi
        0x30D5, // フ Fu
        0x30D6, // ブ Bu
        0x30D7, // プ Pu
        0x30D8, // ヘ He
        0x30D9, // ベ Be
        0x30DA, // ペ Pe
        0x30DB, // ホ Ho
        0x30DC, // ボ Bo
        0x30DD, // ポ Po
        0x30DE, // マ Ma
        0x30DF, // ミ Mi
        0x30E0, // ム Mu
        0x30E1, // メ Me
        0x30E2, // モ Mo
        0x30E3, // ャ Small Ya
        0x30E4, // ヤ Ya
        0x30E5, // ュ Small Yu
        0x30E6, // ユ Yu
        0x30E7  // ョ Small Yo
    };
    const size_t KATAKANA_COUNT = sizeof(KATAKANA_CODEPOINTS) / sizeof(KATAKANA_CODEPOINTS[0]);

    // Latin uppercase letters (A-Z): 26 characters
    const uint32_t LATIN_UPPERCASE_CODEPOINTS[] = {
        0x0041, // A
        0x0042, // B
        0x0043, // C
        0x0044, // D
        0x0045, // E
        0x0046, // F
        0x0047, // G
        0x0048, // H
        0x0049, // I
        0x004A, // J
        0x004B, // K
        0x004C, // L
        0x004D, // M
        0x004E, // N
        0x004F, // O
        0x0050, // P
        0x0051, // Q
        0x0052, // R
        0x0053, // S
        0x0054, // T
        0x0055, // U
        0x0056, // V
        0x0057, // W
        0x0058, // X
        0x0059, // Y
        0x005A  // Z
    };
    const size_t LATIN_UPPERCASE_COUNT = sizeof(LATIN_UPPERCASE_CODEPOINTS) / sizeof(LATIN_UPPERCASE_CODEPOINTS[0]);

    // Latin lowercase letters (a-z): 26 characters
    const uint32_t LATIN_LOWERCASE_CODEPOINTS[] = {
        0x0061, // a
        0x0062, // b
        0x0063, // c
        0x0064, // d
        0x0065, // e
        0x0066, // f
        0x0067, // g
        0x0068, // h
        0x0069, // i
        0x006A, // j
        0x006B, // k
        0x006C, // l
        0x006D, // m
        0x006E, // n
        0x006F, // o
        0x0070, // p
        0x0071, // q
        0x0072, // r
        0x0073, // s
        0x0074, // t
        0x0075, // u
        0x0076, // v
        0x0077, // w
        0x0078, // x
        0x0079, // y
        0x007A  // z
    };
    const size_t LATIN_LOWERCASE_COUNT = sizeof(LATIN_LOWERCASE_CODEPOINTS) / sizeof(LATIN_LOWERCASE_CODEPOINTS[0]);

    // Numerals (0-9): 10 characters
    const uint32_t NUMERAL_CODEPOINTS[] = {
        0x0030, // 0
        0x0031, // 1
        0x0032, // 2
        0x0033, // 3
        0x0034, // 4
        0x0035, // 5
        0x0036, // 6
        0x0037, // 7
        0x0038, // 8
        0x0039  // 9
    };
    constexpr size_t NUMERAL_COUNT = sizeof(NUMERAL_CODEPOINTS) / sizeof(NUMERAL_CODEPOINTS[0]);

    // Half-width Katakana (U+FF65-U+FF9F): single-column-width in terminals
    // These are the characters actually used in the Matrix films
    const uint32_t HALFWIDTH_KATAKANA_CODEPOINTS[] = {
        0xFF66, // ヲ Wo
        0xFF67, // ァ Small A
        0xFF68, // ィ Small I
        0xFF69, // ゥ Small U
        0xFF6A, // ェ Small E
        0xFF6B, // ォ Small O
        0xFF6C, // ャ Small Ya
        0xFF6D, // ュ Small Yu
        0xFF6E, // ョ Small Yo
        0xFF6F, // ッ Small Tsu
        0xFF70, // ー Prolonged Sound Mark
        0xFF71, // ア A
        0xFF72, // イ I
        0xFF73, // ウ U
        0xFF74, // エ E
        0xFF75, // オ O
        0xFF76, // カ Ka
        0xFF77, // キ Ki
        0xFF78, // ク Ku
        0xFF79, // ケ Ke
        0xFF7A, // コ Ko
        0xFF7B, // サ Sa
        0xFF7C, // シ Shi
        0xFF7D, // ス Su
        0xFF7E, // セ Se
        0xFF7F, // ソ So
        0xFF80, // タ Ta
        0xFF81, // チ Chi
        0xFF82, // ツ Tsu
        0xFF83, // テ Te
        0xFF84, // ト To
        0xFF85, // ナ Na
        0xFF86, // ニ Ni
        0xFF87, // ヌ Nu
        0xFF88, // ネ Ne
        0xFF89, // ノ No
        0xFF8A, // ハ Ha
        0xFF8B, // ヒ Hi
        0xFF8C, // フ Fu
        0xFF8D, // ヘ He
        0xFF8E, // ホ Ho
        0xFF8F, // マ Ma
        0xFF90, // ミ Mi
        0xFF91, // ム Mu
        0xFF92, // メ Me
        0xFF93, // モ Mo
        0xFF94, // ヤ Ya
        0xFF95, // ユ Yu
        0xFF96, // ヨ Yo
        0xFF97, // ラ Ra
        0xFF98, // リ Ri
        0xFF99, // ル Ru
        0xFF9A, // レ Re
        0xFF9B, // ロ Ro
        0xFF9C, // ワ Wa
        0xFF9D, // ン N
        0xFF9E, // ゙ Voiced Sound Mark
        0xFF9F  // ゚ Semi-Voiced Sound Mark
    };
    const size_t HALFWIDTH_KATAKANA_COUNT = sizeof(HALFWIDTH_KATAKANA_CODEPOINTS) / sizeof(HALFWIDTH_KATAKANA_CODEPOINTS[0]);





    // Overlay-only symbols: characters needed by overlays but not in main rain set.
    // These are added to the atlas without mirrored variants.
    const uint32_t OVERLAY_SYMBOL_CODEPOINTS[] = {
        0x0020, // space
        0x002B, // +
        0x002D, // -
        0x002F, // /
        0x003F  // ?
    };
    const size_t OVERLAY_SYMBOL_COUNT = sizeof(OVERLAY_SYMBOL_CODEPOINTS) / sizeof(OVERLAY_SYMBOL_CODEPOINTS[0]);





    // Total: 72 + 26 + 26 + 10 = 134 rain characters (mirrored = 268)
    //        + 5 overlay symbols (not mirrored) = 273 glyphs total

    // Helper function to get all codepoints in a single vector
    std::vector<uint32_t> GetAllCodepoints()
    {
        std::vector<uint32_t> allCodepoints;
        allCodepoints.reserve(134);

        // Add katakana
        for (uint32_t cp : KATAKANA_CODEPOINTS)
        {
            allCodepoints.push_back(cp);
        }

        // Add uppercase Latin
        for (uint32_t cp : LATIN_UPPERCASE_CODEPOINTS)
        {
            allCodepoints.push_back(cp);
        }

        // Add lowercase Latin
        for (uint32_t cp : LATIN_LOWERCASE_CODEPOINTS)
        {
            allCodepoints.push_back(cp);
        }

        // Add numerals
        for (uint32_t cp : NUMERAL_CODEPOINTS)
        {
            allCodepoints.push_back(cp);
        }

        return allCodepoints;
    }





    std::vector<uint32_t> GetOverlayCodepoints()
    {
        return std::vector<uint32_t> (std::begin (OVERLAY_SYMBOL_CODEPOINTS),
                                      std::end   (OVERLAY_SYMBOL_CODEPOINTS));
    }





    ////////////////////////////////////////////////////////////////////////////////
    //
    //  GetConsoleCodepoints
    //
    //  Returns codepoints suitable for console/terminal rendering.
    //  Uses half-width Katakana (U+FF66-U+FF9F) instead of full-width,
    //  because full-width Katakana occupy 2 columns in terminals and
    //  corrupt the character grid.
    //
    ////////////////////////////////////////////////////////////////////////////////

    std::vector<uint32_t> GetConsoleCodepoints()
    {
        std::vector<uint32_t> codepoints;
        codepoints.reserve(HALFWIDTH_KATAKANA_COUNT + LATIN_UPPERCASE_COUNT + LATIN_LOWERCASE_COUNT + NUMERAL_COUNT);

        for (uint32_t cp : HALFWIDTH_KATAKANA_CODEPOINTS)
        {
            codepoints.push_back(cp);
        }

        for (uint32_t cp : LATIN_UPPERCASE_CODEPOINTS)
        {
            codepoints.push_back(cp);
        }

        for (uint32_t cp : LATIN_LOWERCASE_CODEPOINTS)
        {
            codepoints.push_back(cp);
        }

        for (uint32_t cp : NUMERAL_CODEPOINTS)
        {
            codepoints.push_back(cp);
        }

        return codepoints;
    }
}
