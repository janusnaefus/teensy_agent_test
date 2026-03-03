#include <Arduino.h>

const char* message = "hello world";

const uint32_t dotDuration = 100;      // 1 unit
const uint32_t dashDuration = dotDuration * 3; // 3 units
const uint32_t intraCharGap = dotDuration;     // between dots/dashes in a char (1 unit)
const uint32_t interCharGap = dotDuration * 3; // between characters (3 units)
const uint32_t interWordGap = dotDuration * 7; // between words (7 units)

struct MorseSymbol {
    char symbol;
    const char* code;
};

const MorseSymbol morseTable[] = {
    {'a', ".-"},
    {'b', "-..."},
    {'c', "-.-."},
    {'d', "-.."},
    {'e', "."},
    {'f', "..-."},
    {'g', "--."},
    {'h', "...."},
    {'i', ".."},
    {'j', ".---"},
    {'k', "-.-"},
    {'l', ".-.."},
    {'m', "--"},
    {'n', "-."},
    {'o', "---"},
    {'p', ".--."},
    {'q', "--.-"},
    {'r', ".-."},
    {'s', "..."},
    {'t', "-"},
    {'u', "..-"},
    {'v', "...-"},
    {'w', ".--"},
    {'x', "-..-"},
    {'y', "-.--"},
    {'z', "--.."},
    {'0', "-----"},
    {'1', ".----"},
    {'2', "..---"},
    {'3', "...--"},
    {'4', "....-"},
    {'5', "....."},
    {'6', "-...."},
    {'7', "--..."},
    {'8', "---.."},
    {'9', "----."},
    {' ', " "}, // space handled separately
};

const char* getMorseCode(char c) {
    c = tolower(c);
    for (uint8_t i = 0; i < sizeof(morseTable)/sizeof(morseTable[0]); i++) {
        if (morseTable[i].symbol == c) return morseTable[i].code;
    }
    return ""; // unknown char -> no code
}

enum State {
    SYMBOL_ON,
    SYMBOL_OFF,
    CHAR_OFF,
    WORD_OFF,
    MESSAGE_OFF
};

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // wait for serial for up to 3 seconds
    }
    Serial.println("Teensy 4.1 test: hello from MCU dev agent!");
}

void loop() {
    static uint32_t last = 0;
    static State state = SYMBOL_ON;
    static uint8_t msgIndex = 0;       // index in message string
    static const char* currentCode = nullptr;
    static uint8_t codeIndex = 0;      // index in current morse code string
    static uint32_t duration = 0;

    uint32_t now = millis();

    if (now - last < duration) return;

    last = now;

    switch(state) {
        case SYMBOL_ON:
            if (!currentCode) {
                currentCode = getMorseCode(message[msgIndex]);
                codeIndex = 0;
                if (currentCode[0] == '\0') {
                    // unknown char, skip
                    msgIndex++;
                    if (msgIndex >= strlen(message)) {
                        state = MESSAGE_OFF;
                        duration = interWordGap;
                        Serial.print("repeat at ");
                        Serial.println(now);
                        msgIndex = 0;
                        currentCode = nullptr;
                        break;
                    }
                    currentCode = getMorseCode(message[msgIndex]);
                }
            }
            if (currentCode[codeIndex] == '\0') {
                // end of current character
                msgIndex++;
                currentCode = nullptr;
                codeIndex = 0;
                if (msgIndex >= strlen(message)) {
                    state = MESSAGE_OFF;
                    duration = interWordGap;
                    Serial.print("repeat at ");
                    Serial.println(now);
                    msgIndex = 0;
                } else if (message[msgIndex] == ' ') {
                    state = WORD_OFF;
                    duration = interWordGap;
                } else {
                    state = CHAR_OFF;
                    duration = interCharGap;
                }
                digitalWrite(LED_BUILTIN, LOW);
            } else {
                // output current symbol ON
                digitalWrite(LED_BUILTIN, HIGH);
                if (currentCode[codeIndex] == '.') {
                    duration = dotDuration;
                } else { // '-'
                    duration = dashDuration;
                }
                state = SYMBOL_OFF;
            }
            break;

        case SYMBOL_OFF:
            digitalWrite(LED_BUILTIN, LOW);
            duration = intraCharGap;
            codeIndex++;
            state = SYMBOL_ON;
            break;

        case CHAR_OFF:
            // gap between characters
            digitalWrite(LED_BUILTIN, LOW);
            state = SYMBOL_ON;
            break;

        case WORD_OFF:
            // gap between words
            digitalWrite(LED_BUILTIN, LOW);
            msgIndex++; // skip space
            state = SYMBOL_ON;
            break;

        case MESSAGE_OFF:
            digitalWrite(LED_BUILTIN, LOW);
            state = SYMBOL_ON;
            break;
    }
}