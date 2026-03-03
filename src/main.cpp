#include <Arduino.h>

const char* message = "foobar";

const uint32_t totalTransmissionTime = 2000; // 2 seconds per transmission
const uint32_t breakDuration = 1000;          // 1 second break

// Morse timing units will be calculated dynamically to fit totalTransmissionTime

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

// Precompute total units in the message for timing calculation
uint32_t countUnits(const char* msg) {
    uint32_t units = 0;
    size_t msgLen = strlen(msg);
    for (size_t i = 0; i < msgLen; i++) {
        char c = msg[i];
        if (c == ' ') {
            units += 7; // word gap units
            continue;
        }
        const char* code = getMorseCode(c);
        if (!code || code[0] == '\0') continue;

        size_t len = strlen(code);
        for (size_t j = 0; j < len; j++) {
            if (code[j] == '.') units += 1;
            else if (code[j] == '-') units += 3;
            if (j < len - 1) units += 1; // intra-character gap
        }
        // after character gap (except last char or before space)
        if (i < msgLen - 1 && msg[i+1] != ' ') {
            units += 3;
        }
    }
    return units;
}

uint32_t dotDuration = 100;      // will be recalculated
uint32_t dashDuration = 0;
uint32_t intraCharGap = 0;
uint32_t interCharGap = 0;
uint32_t interWordGap = 0;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // wait for serial for up to 3 seconds
    }
    Serial.println("Teensy 4.1 test: hello from MCU dev agent!");

    // Calculate timing units to fit totalTransmissionTime
    uint32_t totalUnits = countUnits(message);
    if (totalUnits == 0) totalUnits = 1; // avoid div by zero

    dotDuration = totalTransmissionTime / totalUnits;
    dashDuration = dotDuration * 3;
    intraCharGap = dotDuration;
    interCharGap = dotDuration * 3;
    interWordGap = dotDuration * 7;

    Serial.print("Timing units (ms): dot=");
    Serial.print(dotDuration);
    Serial.print(" dash=");
    Serial.print(dashDuration);
    Serial.print(" intraCharGap=");
    Serial.print(intraCharGap);
    Serial.print(" interCharGap=");
    Serial.print(interCharGap);
    Serial.print(" interWordGap=");
    Serial.println(interWordGap);
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
                        duration = breakDuration;
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
                    duration = breakDuration;
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
                Serial.print("CHAR_OFF at ");
                Serial.println(now);
            } else {
                // output current symbol ON
                digitalWrite(LED_BUILTIN, HIGH);
                if (currentCode[codeIndex] == '.') {
                    duration = dotDuration;
                    Serial.print(".");
                    Serial.print(" ON ");
                    Serial.print(duration);
                    Serial.println("ms");
                } else { // '-'
                    duration = dashDuration;
                    Serial.print("-");
                    Serial.print(" ON ");
                    Serial.print(duration);
                    Serial.println("ms");
                }
                state = SYMBOL_OFF;
            }
            break;

        case SYMBOL_OFF:
            digitalWrite(LED_BUILTIN, LOW);
            duration = intraCharGap;
            Serial.print(" OFF ");
            Serial.print(duration);
            Serial.println("ms");
            codeIndex++;
            state = SYMBOL_ON;
            break;

        case CHAR_OFF:
            // gap between characters
            digitalWrite(LED_BUILTIN, LOW);
            Serial.print("CHAR GAP ");
            Serial.print(duration);
            Serial.println("ms");
            state = SYMBOL_ON;
            break;

        case WORD_OFF:
            // gap between words
            digitalWrite(LED_BUILTIN, LOW);
            Serial.print("WORD GAP ");
            Serial.print(duration);
            Serial.println("ms");
            msgIndex++; // skip space
            state = SYMBOL_ON;
            break;

        case MESSAGE_OFF:
            digitalWrite(LED_BUILTIN, LOW);
            Serial.print("MESSAGE GAP ");
            Serial.print(duration);
            Serial.println("ms");
            delay(duration);
            state = SYMBOL_ON;
            break;
    }
}