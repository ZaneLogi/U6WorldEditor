#pragma once

/*
the text pattern is used in the script as below

@ : following word is highlighted
* : stop and wait for ENTER/SPACE/ESC key (similiar to `wait' command)

$G : gender title ("milord", "milady")
$P : player name
$N : NPC name
$T : time of day ("morning", "afternoon", "evening")
$Y : set-able name, of any NPC
$Z : previous input
$X : the value of string variable X
#X : the value of variable X
<> : upper-case text between greater-than & less-than brackets is printed as
        Runic; lower-case text is printed as Gargish
/\ : !?? related to plural word inflections, text after \ is printed if some
        variable is not 1, text after / is printed if some variable is 1 ??
&& : ! a block of text before the first ampersand may be "translated" - replaced
        by a block of text before the second ampersand
*/

#define U6OP_GT         0x81        // >
#define U6OP_GE         0x82        // >=
#define U6OP_LT         0x83        // <
#define U6OP_LE         0x84        // <=
#define U6OP_NE         0x85        // !=
#define U6OP_EQ         0x86        // ==

#define U6OP_ADD        0x90        // +
#define U6OP_SUB        0x91        // -
#define U6OP_MUL        0x92        // *
#define U6OP_DIV        0x93        // /
#define U6OP_LOR        0x94        // |
#define U6OP_LAND       0x95        // &

#define U6OP_CANCARRY   0x9a
#define U6OP_WEIGHT     0x9b

#define U6OP_RAND       0xa0        // generate a random number (min, max)
#define U6OP_IF         0xa1
#define U6OP_ENDIF      0xa2
#define U6OP_ELSE       0xa3
#define U6OP_SETF       0xa4        // set flag
#define U6OP_CLEARF     0xa5        // clear flag
#define U6OP_DECL       0xa6        // declare variable
#define U6OP_EVAL       0xa7        // evaluate
#define U6OP_ASSIGN     0xa8        // assign value to the declared variable

#define U6OP_FLAG       0xab        // get npc flags

#define U6OP_JUMP       0xb0        // jump to the address

#define U6OP_VAR        0xb2        // integer data
#define U6OP_SVAR       0xb3        // string data
#define U6OP_DATA       0xb4        // list data

#define U6OP_BYE        0xb6

#define U6OP_NEW        0xb9        // create a new object for npc
#define U6OP_DELETE     0xba        // delete an object from npc
#define U6OP_OBJCOUNT   0xbb

#define U6OP_PORTRAIT   0xbf

#define U6OP_INPARTY    0xc6
#define U6OP_OBJINPARTY 0xc7

#define U6OP_GIVE       0xc9
#define U6OP_JOIN       0xca        // return 3: ALREADY IN PARTY, 2: PARTY TOO LARGE, 1: NOT ON LAND (vehicle), 0: SUCCESS
#define U6OP_PAUSE      0xcb        // pause the script and wait to hit any key
#define U6OP_LEAVE      0xcc        // return 2: NOT IN PARTY, 1: NOT ON LAND, 0: SUCCESS
#define U6OP_WORKTYPE   0xcd

#define U6OP_FUNC       0xd1        // execute the specific functions which are hard-coded.
#define U6OP_NUM32      0xd2
#define U6OP_NUM8       0xd3
#define U6OP_NUM16      0xd4

#define U6OP_REVIVE     0xd6

#define U6OP_SET_Y      0xd8        // set NPC name for $Y in the text
#define U6OP_HEAL       0xd9
#define U6OP_WOUNDED    0xda
#define U6OP_CURE       0xdb
#define U6OP_POISONED   0xdc
#define U6OP_NPC        0xdd        // return NPC id based on the index

#define U6OP_EXP        0xe0
#define U6OP_LVL        0xe1
#define U6OP_STR        0xe2
#define U6OP_INT        0xe3
#define U6OP_DEX        0xe4

#define U6OP_ENDANSWER  0xee
#define U6OP_KEYWORDS   0xef

#define U6OP_LOOK       0xf1
#define U6OP_CONVERSE   0xf2

#define U6OP_ANSWER     0xf6
#define U6OP_ASK        0xf7
#define U6OP_ASKC       0xf8

#define U6OP_INPUT      0xfb
#define U6OP_INPUTNUM   0xfc

#define U6OP_ID         0xff


