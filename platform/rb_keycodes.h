#ifndef PICOCALC_KEYS_H
#define PICOCALC_KEYS_H

#define KEY_BACKSPACE   8      // The backspace key
#define KEY_TAB         9      // The tab key
#define KEY_ENTER       10     // The enter/return key
#define KEY_SPACE       999    // The space key, TODO: CHECK CODE
#define KEY_EXCLAMATION 33     // !
#define KEY_DOUBLEQUOTE 34     // "
#define KEY_HASH        35     // #
#define KEY_DOLLAR      36     // $
#define KEY_PERCENT     37     // %
#define KEY_AMPERSAND   38     // &
#define KEY_SINGLEQUOTE 39     // '
#define KEY_PAREN_OPEN  40     // (
#define KEY_PAREN_CLOSE 41     // )
#define KEY_ASTERISK    42     // *
#define KEY_PLUS        43     // +
#define KEY_COMMA       44     // ,
#define KEY_MINUS       45     // -
#define KEY_PERIOD      46     // .
#define KEY_SLASH       47     // /

// Individual Digit Keys (ASCII 48-57)
#define KEY_0           48
#define KEY_1           49
#define KEY_2           50
#define KEY_3           51
#define KEY_4           52
#define KEY_5           53
#define KEY_6           54
#define KEY_7           55
#define KEY_8           56
#define KEY_9           57

#define KEY_COLON       58     // :
#define KEY_SEMICOLON   59     // ;
#define KEY_EQUALS      61     // =
#define KEY_QUESTION    63     // ?
#define KEY_AT          64     // @

// Individual Uppercase Letter Keys (ASCII 65-90)
#define KEY_A           65
#define KEY_B           66
#define KEY_C           67
#define KEY_D           68
#define KEY_E           69
#define KEY_F           70
#define KEY_G           71
#define KEY_H           72
#define KEY_I           73
#define KEY_J           74
#define KEY_K           75
#define KEY_L           76
#define KEY_M           77
#define KEY_N           78
#define KEY_O           79
#define KEY_P           80
#define KEY_Q           81
#define KEY_R           82
#define KEY_S           83
#define KEY_T           84
#define KEY_U           85
#define KEY_V           86
#define KEY_W           87
#define KEY_X           88
#define KEY_Y           89
#define KEY_Z           90

#define KEY_BRACKET_OPEN  91   // [
#define KEY_BACKSLASH     92   // "\"
#define KEY_BRACKET_CLOSE 93   // ]
#define KEY_CARET         94   // ^
#define KEY_UNDERSCORE    95   // _
#define KEY_BACKTICK      96   // `

// Individual Lowercase Letter Keys (ASCII 97-122)
#define KEY_a           97
#define KEY_b           98
#define KEY_c           99
#define KEY_d           100
#define KEY_e           101
#define KEY_f           102
#define KEY_g           103
#define KEY_h           104
#define KEY_i           105
#define KEY_j           106
#define KEY_k           107
#define KEY_l           108
#define KEY_m           109
#define KEY_n           110
#define KEY_o           111
#define KEY_p           112
#define KEY_q           113
#define KEY_r           114
#define KEY_s           115
#define KEY_t           116
#define KEY_u           117
#define KEY_v           118
#define KEY_w           119
#define KEY_x           120
#define KEY_y           121
#define KEY_z           122

#define KEY_CURLY_OPEN  123    // {
#define KEY_PIPE        124    // |
#define KEY_CURLY_CLOSE 125    // }
#define KEY_TILDE       126    // ~

// Individual Function Keys (F1-F10)
#define KEY_F1          129
#define KEY_F2          130
#define KEY_F3          131
#define KEY_F4          132
#define KEY_F5          133
#define KEY_F6          134
#define KEY_F7          135
#define KEY_F8          136
#define KEY_F9          137
#define KEY_F10         144

// Modifier Keys
#define KEY_ALT         161     // The Alt key
#define KEY_SHIFT       163     // The Shift key
#define KEY_CTRL        165     // The Control key
#define KEY_ESC         177     // The Escape key

// Arrow Keys
#define KEY_LEFT        180     // Left arrow key
#define KEY_RIGHT       183     // Right arrow key
#define KEY_UP          181     // Up arrow key
#define KEY_DOWN        182     // Down arrow key

// Other Special Keys
#define KEY_CAPSLOCK    193     // Caps Lock key
#define KEY_BREAK       208     // Break/Pause key
#define KEY_HOME        210     // Home key
#define KEY_DELETE      212     // Delete key
#define KEY_END         213     // End key

// Missing keys as functions keys
#define KEY_PAGE_UP       KEY_F9
#define KEY_PAGE_DOWN     KEY_F10

#endif // PICOCALC_KEYS_H
