package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.ControllerBinding
import com.mrjackspade.kairo.frontend.ControllerBindingsCodec
import com.mrjackspade.kairo.frontend.ControllerLayout
import com.mrjackspade.kairo.frontend.GuestKeyboardLayout
import com.mrjackspade.kairo.frontend.GuestKeyboardPage
import com.mrjackspade.kairo.frontend.UiText
import com.mrjackspade.kairo.frontend.R as FrontendR
import com.mrjackspade.kairo.frontend.JoystickInputRouter
import com.mrjackspade.kairo.frontend.KeyboardKey
import com.mrjackspade.kairo.frontend.SymbolPage
import android.view.KeyEvent

/**
 * The MSX keyboard by its place in the keyboard matrix (the core's keys: row * 8 + column + 1, so
 * row 0 column 0 is 1), with the international legends of the C-BIOS machines, and the joystick in
 * port A.
 */
object MsxKeys {
    private fun key(row: Int, column: Int) = row * 8 + column + 1

    /** Digits 0 to 9: row 0, then row 1. */
    private fun digit(char: Char) = (char - '0').let { if (it < 8) key(0, it) else key(1, it - 8) }
    const val MINUS = 1 * 8 + 2 + 1
    const val EQUALS = 1 * 8 + 3 + 1
    const val BACKSLASH = 1 * 8 + 4 + 1
    const val LEFT_BRACKET = 1 * 8 + 5 + 1
    const val RIGHT_BRACKET = 1 * 8 + 6 + 1
    const val SEMICOLON = 1 * 8 + 7 + 1
    const val QUOTE = 2 * 8 + 0 + 1
    const val BACKQUOTE = 2 * 8 + 1 + 1
    const val COMMA = 2 * 8 + 2 + 1
    const val DOT = 2 * 8 + 3 + 1
    const val SLASH = 2 * 8 + 4 + 1
    const val DEAD = 2 * 8 + 5 + 1
    /** Letters A to Z, from row 2 column 6. */
    private fun letter(char: Char) = 2 * 8 + 6 + 1 + (char.lowercaseChar() - 'a')
    const val SHIFT = 6 * 8 + 0 + 1
    const val CTRL = 6 * 8 + 1 + 1
    const val GRAPH = 6 * 8 + 2 + 1
    const val CAPS = 6 * 8 + 3 + 1
    const val CODE = 6 * 8 + 4 + 1
    private val function = listOf(key(6, 5), key(6, 6), key(6, 7), key(7, 0), key(7, 1))
    const val ESC = 7 * 8 + 2 + 1
    const val TAB = 7 * 8 + 3 + 1
    const val STOP = 7 * 8 + 4 + 1
    const val BS = 7 * 8 + 5 + 1
    const val SELECT = 7 * 8 + 6 + 1
    const val RETURN = 7 * 8 + 7 + 1
    const val SPACE = 8 * 8 + 0 + 1
    const val HOME = 8 * 8 + 1 + 1
    const val INS = 8 * 8 + 2 + 1
    const val DEL = 8 * 8 + 3 + 1
    const val LEFT = 8 * 8 + 4 + 1
    const val UP = 8 * 8 + 5 + 1
    const val DOWN = 8 * 8 + 6 + 1
    const val RIGHT = 8 * 8 + 7 + 1
    const val NUM_STAR = 9 * 8 + 0 + 1
    const val NUM_PLUS = 9 * 8 + 1 + 1
    const val NUM_SLASH = 9 * 8 + 2 + 1
    /** Keypad 0 to 9: row 9 from column 3, then row 10. */
    private fun numpad(digit: Int) = if (digit < 5) key(9, 3 + digit) else key(10, digit - 5)
    const val NUM_MINUS = 10 * 8 + 5 + 1
    const val NUM_COMMA = 10 * 8 + 6 + 1
    const val NUM_DOT = 10 * 8 + 7 + 1
    /** The last row, 10. */
    const val MAX_CODE = 11 * 8

    private const val DIGITS = "1234567890"

    val layout = GuestKeyboardLayout(
        brand = "KAIROMSX",
        pages = listOf(
            GuestKeyboardPage("ABC", alphabetRows()),
            GuestKeyboardPage("?123", symbolRows(), pickerLabel = UiText.res(R.string.keyboard_symbols)),
            GuestKeyboardPage("MSX", msxRows())
        ),
        modifiers = setOf(SHIFT, CTRL, GRAPH, CODE, CAPS),
        shiftCodes = setOf(SHIFT),
        capsCode = CAPS,
        chordShiftCode = SHIFT
    )

    private fun alphabetRows(): List<List<KeyboardKey>> = listOf(
        listOf(KeyboardKey("Esc", ESC, 1.3f)) +
            DIGITS.zip("!@#$%^&*()").map { (char, shifted) -> KeyboardKey("$char", digit(char), shifted = "$shifted") } +
            listOf(KeyboardKey("-", MINUS, shifted = "_"), KeyboardKey("=", EQUALS, shifted = "+"),
                KeyboardKey("\\", BACKSLASH, shifted = "|"), KeyboardKey("BS", BS, 1.5f)),
        listOf(KeyboardKey("Tab", TAB, 1.3f)) + "qwertyuiop".map { KeyboardKey("${it.uppercaseChar()}", letter(it)) } +
            listOf(KeyboardKey("[", LEFT_BRACKET, shifted = "{"), KeyboardKey("]", RIGHT_BRACKET, shifted = "}"),
                KeyboardKey("Return", RETURN, 1.5f)),
        listOf(KeyboardKey("Ctrl", CTRL, 1.4f)) + "asdfghjkl".map { KeyboardKey("${it.uppercaseChar()}", letter(it)) } +
            listOf(KeyboardKey(";", SEMICOLON, shifted = ":"), KeyboardKey("'", QUOTE, shifted = "\""),
                KeyboardKey("`", BACKQUOTE, shifted = "~")),
        listOf(KeyboardKey("Shift", SHIFT, 1.5f)) + "zxcvbnm".map { KeyboardKey("${it.uppercaseChar()}", letter(it)) } +
            listOf(KeyboardKey(",", COMMA, shifted = "<"), KeyboardKey(".", DOT, shifted = ">"),
                KeyboardKey("/", SLASH, shifted = "?"), KeyboardKey("Shift", SHIFT, 1.5f)),
        listOf(KeyboardKey("Caps", CAPS), KeyboardKey("Graph", GRAPH), KeyboardKey("Space", SPACE, 5f),
            KeyboardKey("Code", CODE), KeyboardKey("←", LEFT), KeyboardKey("↑", UP), KeyboardKey("↓", DOWN),
            KeyboardKey("→", RIGHT))
    )

    /** Each symbol on its own key, Shift chorded where the symbol needs it. */
    private fun symbolRows(): List<List<KeyboardKey>> = SymbolPage.rows(
        digits = DIGITS.map { KeyboardKey("$it", digit(it)) },
        delete = KeyboardKey("BS", BS, 1.5f),
        shiftedDigits = DIGITS.zip("!@#$%^&*()").map { (char, symbol) -> KeyboardKey("$symbol", digit(char), chordShift = true) },
        symbols = listOf("-" to MINUS, "=" to EQUALS, "\\" to BACKSLASH, "[" to LEFT_BRACKET, "]" to RIGHT_BRACKET,
            ";" to SEMICOLON, "'" to QUOTE, "`" to BACKQUOTE, "," to COMMA, "." to DOT, "/" to SLASH)
            .map { (label, code) -> KeyboardKey(label, code) },
        shiftedSymbols = listOf("_" to MINUS, "+" to EQUALS, "|" to BACKSLASH, "{" to LEFT_BRACKET, "}" to RIGHT_BRACKET,
            ":" to SEMICOLON, "\"" to QUOTE, "~" to BACKQUOTE, "<" to COMMA, ">" to DOT, "?" to SLASH)
            .map { (label, code) -> KeyboardKey(label, code, chordShift = true) },
        space = KeyboardKey("Space", SPACE, 5f),
        enter = KeyboardKey("Return", RETURN, 1.5f),
        escape = KeyboardKey("Esc", ESC, 1.3f),
        tab = KeyboardKey("Tab", TAB))

    private fun msxRows(): List<List<KeyboardKey>> = listOf(
        function.mapIndexed { index, code -> KeyboardKey("F${index + 1}", code) } +
            listOf(KeyboardKey("Stop", STOP), KeyboardKey("Select", SELECT), KeyboardKey("Home", HOME),
                KeyboardKey("Ins", INS), KeyboardKey("Del", DEL)),
        listOf(KeyboardKey("Ctrl", CTRL), KeyboardKey("Shift", SHIFT), KeyboardKey("Graph", GRAPH),
            KeyboardKey("Code", CODE), KeyboardKey("Dead", DEAD), KeyboardKey("↑", UP)),
        listOf(KeyboardKey("7", numpad(7)), KeyboardKey("8", numpad(8)), KeyboardKey("9", numpad(9)),
            KeyboardKey("/", NUM_SLASH), KeyboardKey("*", NUM_STAR), KeyboardKey("←", LEFT), KeyboardKey("↓", DOWN),
            KeyboardKey("→", RIGHT)),
        listOf(KeyboardKey("4", numpad(4)), KeyboardKey("5", numpad(5)), KeyboardKey("6", numpad(6)),
            KeyboardKey("-", NUM_MINUS), KeyboardKey("+", NUM_PLUS)),
        listOf(KeyboardKey("1", numpad(1)), KeyboardKey("2", numpad(2)), KeyboardKey("3", numpad(3)),
            KeyboardKey("0", numpad(0)), KeyboardKey(",", NUM_COMMA), KeyboardKey(".", NUM_DOT))
    )

    /** Every key a controller binding can send. */
    val bindableCodes: List<Int> = (1..MAX_CODE).filter { code -> label(code) != null }

    fun label(code: Int): String? = when (code) {
        in DIGITS.map(::digit) -> "${DIGITS[DIGITS.map(::digit).indexOf(code)]}"
        MINUS -> "-"
        EQUALS -> "="
        BACKSLASH -> "\\"
        LEFT_BRACKET -> "["
        RIGHT_BRACKET -> "]"
        SEMICOLON -> ";"
        QUOTE -> "'"
        BACKQUOTE -> "`"
        COMMA -> ","
        DOT -> "."
        SLASH -> "/"
        DEAD -> "Dead key"
        in letter('a')..letter('z') -> "${'A' + (code - letter('a'))}"
        SHIFT -> "Shift"
        CTRL -> "Ctrl"
        GRAPH -> "Graph"
        CAPS -> "Caps"
        CODE -> "Code"
        in function -> "F${function.indexOf(code) + 1}"
        ESC -> "Esc"
        TAB -> "Tab"
        STOP -> "Stop"
        BS -> "BS"
        SELECT -> "Select"
        RETURN -> "Return"
        SPACE -> "Space"
        HOME -> "Home"
        INS -> "Ins"
        DEL -> "Del"
        LEFT -> "Left"
        UP -> "Up"
        DOWN -> "Down"
        RIGHT -> "Right"
        NUM_STAR -> "Numpad *"
        NUM_PLUS -> "Numpad +"
        NUM_SLASH -> "Numpad /"
        in (0..9).map(::numpad) -> "Numpad ${(0..9).first { numpad(it) == code }}"
        NUM_MINUS -> "Numpad -"
        NUM_COMMA -> "Numpad ,"
        NUM_DOT -> "Numpad ."
        else -> null
    }

    /** The MSX key an Android key types, by its US legend; the PC keys MSX lacks as openMSX maps them. */
    fun fromAndroid(key: Int): Int? {
        if (key in KeyEvent.KEYCODE_A..KeyEvent.KEYCODE_Z) return letter('a' + (key - KeyEvent.KEYCODE_A))
        if (key in KeyEvent.KEYCODE_0..KeyEvent.KEYCODE_9) return digit('0' + (key - KeyEvent.KEYCODE_0))
        if (key in KeyEvent.KEYCODE_F1..KeyEvent.KEYCODE_F5) return function[key - KeyEvent.KEYCODE_F1]
        if (key in KeyEvent.KEYCODE_NUMPAD_0..KeyEvent.KEYCODE_NUMPAD_9) return numpad(key - KeyEvent.KEYCODE_NUMPAD_0)
        return when (key) {
            KeyEvent.KEYCODE_MINUS -> MINUS
            KeyEvent.KEYCODE_EQUALS -> EQUALS
            KeyEvent.KEYCODE_BACKSLASH -> BACKSLASH
            KeyEvent.KEYCODE_LEFT_BRACKET -> LEFT_BRACKET
            KeyEvent.KEYCODE_RIGHT_BRACKET -> RIGHT_BRACKET
            KeyEvent.KEYCODE_SEMICOLON -> SEMICOLON
            KeyEvent.KEYCODE_APOSTROPHE -> QUOTE
            KeyEvent.KEYCODE_GRAVE -> BACKQUOTE
            KeyEvent.KEYCODE_COMMA -> COMMA
            KeyEvent.KEYCODE_PERIOD -> DOT
            KeyEvent.KEYCODE_SLASH -> SLASH
            KeyEvent.KEYCODE_SHIFT_LEFT, KeyEvent.KEYCODE_SHIFT_RIGHT -> SHIFT
            KeyEvent.KEYCODE_CTRL_LEFT, KeyEvent.KEYCODE_CTRL_RIGHT -> CTRL
            KeyEvent.KEYCODE_ALT_LEFT -> GRAPH
            KeyEvent.KEYCODE_ALT_RIGHT -> CODE
            KeyEvent.KEYCODE_CAPS_LOCK -> CAPS
            KeyEvent.KEYCODE_ESCAPE -> ESC
            KeyEvent.KEYCODE_TAB -> TAB
            KeyEvent.KEYCODE_BREAK, KeyEvent.KEYCODE_F8 -> STOP
            KeyEvent.KEYCODE_DEL -> BS
            KeyEvent.KEYCODE_F7 -> SELECT
            KeyEvent.KEYCODE_ENTER -> RETURN
            KeyEvent.KEYCODE_SPACE -> SPACE
            KeyEvent.KEYCODE_MOVE_HOME -> HOME
            KeyEvent.KEYCODE_INSERT -> INS
            KeyEvent.KEYCODE_FORWARD_DEL -> DEL
            KeyEvent.KEYCODE_DPAD_LEFT -> LEFT
            KeyEvent.KEYCODE_DPAD_UP -> UP
            KeyEvent.KEYCODE_DPAD_DOWN -> DOWN
            KeyEvent.KEYCODE_DPAD_RIGHT -> RIGHT
            KeyEvent.KEYCODE_NUMPAD_MULTIPLY -> NUM_STAR
            KeyEvent.KEYCODE_NUMPAD_ADD -> NUM_PLUS
            KeyEvent.KEYCODE_NUMPAD_DIVIDE -> NUM_SLASH
            KeyEvent.KEYCODE_NUMPAD_SUBTRACT -> NUM_MINUS
            KeyEvent.KEYCODE_NUMPAD_COMMA -> NUM_COMMA
            KeyEvent.KEYCODE_NUMPAD_DOT -> NUM_DOT
            else -> null
        }
    }

    /** The joystick's controls in the core's order (msx_machine.h kJoystickControls). */
    val joystick = JoystickInputRouter.DEFAULT_CONTROLS
    val joystickLabels = listOf(FrontendR.string.kairo_up, FrontendR.string.kairo_down, FrontendR.string.kairo_left,
        FrontendR.string.kairo_right, R.string.joystick_trigger_a, R.string.joystick_trigger_b).map { UiText.res(it) }

    val bindings = ControllerBindingsCodec({ it in bindableCodes }, joystick,
        setOf("menu", "pause", "restart", "exit", "fastForward"))

}
