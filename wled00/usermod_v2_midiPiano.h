#pragma once

#include "wled.h"

// MIDI_CREATE_DEFAULT_INSTANCE();

/*

Notes on LED strips. The pins are in a different order between these 2 strips!

The desk monitor ones are:
GND - YELLOW
DATA - GREEN
5V - RED

The 144LEM/m whomps ones are 
5V - RED
DATA - GREEN
GND - WHITE

*/

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Do whatever you want when a note is pressed.

    // Try to keep your callbacks short (no delays ect)
    // otherwise it would slow down the loop() and have a bad impact
    // on real-time performance.

    // determine which key was pressed
    // 2 leds for white, 1 for black?
    // map this to the relevant LED index
    // udpate mask
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Do something when the note is released.
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
}

class usermod_v2_midiPiano : public Usermod {

  private:

    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = false;
    bool initDone = false;
    unsigned long lastTime = 0;

    // These config variables have defaults set inside readFromConfig()
    int lowestKeyValue;
    int pianoMode;
    int keyPressMode;
    int testR;
    int testG;
    int testB;
    float decay_rate;
    float decay_cutoff;
    unsigned long update_rate;

    int LED1;
    int LED2;

    int counter = 1;

    const uint8_t LED_PAIR_MULTIPLIER = 2;
    const uint8_t LED_OFF = 0;
    const uint8_t LED_ON = 1;

    // string that are used multiple time (this will save some flash memory)
    static const char _name[];
    static const char _enabled[];

    // overall mask to define which LEDs are on
    #define numberOfKeys        72
    float maskLedsOn[numberOfKeys];
    //  =
    // {
    //   0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0
    // };
    float decay_array[numberOfKeys];


    // any private methods should go here (non-inline methosd should be defined out of class)
    void publishMqtt(const char* state, bool retain = false); // example for publishing MQTT message


  public:

    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */

    void setup() {
      // do your set-up here
      // Serial.println("Hello from my usermod!");
      initDone = true;

      // MIDI.setHandleNoteOn(handleNoteOn);

      // MIDI.setHandleNoteOff(handleNoteOff);

      // MIDI.begin(MIDI_CHANNEL_OMNI);  // Listen to all incoming messages
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      // Serial.println("Connected to WiFi!");
    }

    void loop() {

    }

    void mainLoop() {

      // Read incoming messages
      // MIDI.read();

      pianoEffects();

    }

    void pianoEffects() {

      switch (pianoMode)
      {
      case 1:
        fakeKeyPresses();
        no_decay();
        break;

      case 2:
        fakeKeyPresses();
        decay();
        break;

      case 3:
        glissando();
        decay();
        break;
      }

    }

    void no_decay()
    {
      for (int x = 0; x < numberOfKeys; x++)
      {
        if (maskLedsOn[x] == 0)
        {
          LED1 = x * 2;
          LED2 = LED1 + 1;

          strip.setPixelColor(LED1, RGBW32(0,0,0,0));
          strip.setPixelColor(LED2, RGBW32(0,0,0,0));
        }
      }
    }

    void decay()
    {
      for (int x = 0; x < numberOfKeys; x++)
      {
          int LED1 = x * LED_PAIR_MULTIPLIER;
          int LED2 = LED1 + 1;

          // retrieve the color of the current pixel
          uint32_t pixel_color = strip.getPixelColor(LED1);

          // split this into the component colors
          uint8_t pixel_r = (pixel_color >> 16) & 0xFF; // Shift right 16 bits, and mask out the rest
          uint8_t pixel_g = (pixel_color >> 8) & 0xFF;  // Shift right 8 bits, and mask out the rest
          uint8_t pixel_b = pixel_color & 0xFF;         // No need to shift, just mask out the rest

          // lower threshold, stops infinite decay (which would mean pixels never go properly off)
          if (maskLedsOn[x] < decay_cutoff) {
            maskLedsOn[x] = LED_OFF;
            pixel_r = pixel_g = pixel_b = LED_OFF;
          }
          // decay
          else {
            pixel_r = round(pixel_r * maskLedsOn[x]);
            pixel_g = round(pixel_g * maskLedsOn[x]);
            pixel_b = round(pixel_b * maskLedsOn[x]);
            maskLedsOn[x] = maskLedsOn[x] * decay_rate;
          }
          // update the pixels
          strip.setPixelColor(LED1, RGBW32(pixel_r, pixel_g, pixel_b, 0));
          strip.setPixelColor(LED2, RGBW32(pixel_r, pixel_g, pixel_b, 0));

      }
    }

    void fakeKeyPresses() {
      if (millis() - lastTime > update_rate) {
      //     //Serial.println("I'm alive!");
        lastTime = millis();

        for(int i=0; i<numberOfKeys; i++) {
          // get a random number
          int r = random8(0,lowestKeyValue);
          // set pixels to 1
          if (r == 1){ maskLedsOn[i] = 1.0; }
          // set pixels to off if there is no decay
          if ((pianoMode == 1) && (r != 1)) { maskLedsOn[i] = 0; }
        }
      }
    }

    void glissando() {
      if (millis() - lastTime > 50) {
        lastTime = millis();

        if (counter < numberOfKeys) {
          maskLedsOn[counter] = 1.0;
          counter++;
        }
        else{
          counter = 1;
        }
      }
    }

    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    void addToJsonInfo(JsonObject& root)
    {
      // if "u" object does not exist yet wee need to create it
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");
    }


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
      if (!initDone || !enabled) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_name));
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      if (!initDone) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (!usermod.isNull()) {
        // expect JSON usermod data in usermod name object: {"dan_Usermod:{"user0":10}"}
        // userVar0 = usermod["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
      }
      // you can as well check WLED state JSON keys
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }


    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will make your settings editable through the Usermod Settings page automatically.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
     *
     * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
     * 
     * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.  
     * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      //save these vars persistently whenever settings are saved
      // top["great"] = userVar0;
      // top["testBool"] = testBool;
      top["lowestKeyValue"] = lowestKeyValue;
      top["Mode"] = pianoMode;
      top["Key press mode"] = keyPressMode;
      top["R"] = testR;
      top["G"] = testG;
      top["B"] = testB;
      top["Decay Cutoff"] = decay_cutoff;
      top["Update Rate"] = update_rate;
      // top["testULong"] = testULong;
      top["Decay Rate"] = decay_rate;
      // top["testString"] = testString;
      // JsonArray pinArray = top.createNestedArray("pin");
      // pinArray.add(testPins[0]);
      // pinArray.add(testPins[1]); 
    }


    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     * 
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "dan_Usermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root)
    {
      // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
      // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

      JsonObject top = root[FPSTR(_name)];

      bool configComplete = !top.isNull();

      // A 3-argument getJsonValue() assigns the 3rd argument as a default value if the Json value is missing
      configComplete &= getJsonValue(top["lowestKeyValue"], lowestKeyValue, 0);
      configComplete &= getJsonValue(top["Mode"], pianoMode, 0);
      configComplete &= getJsonValue(top["Key Press Mode"], keyPressMode, 0);
      configComplete &= getJsonValue(top["R"], testR, 0);
      configComplete &= getJsonValue(top["G"], testG, 0);
      configComplete &= getJsonValue(top["B"], testB, 0);
      configComplete &= getJsonValue(top["Decay Rate"], decay_rate, 0);
      configComplete &= getJsonValue(top["Decay Cutoff"], decay_cutoff, 0);
      configComplete &= getJsonValue(top["Update Rate"], update_rate, 0);

      return configComplete;
    }


    /*
     * appendConfigData() is called when user enters usermod settings page
     * it may add additional metadata for certain entry fields (adding drop down is possible)
     * be careful not to add too much as oappend() buffer is limited to 3k
     */
    void appendConfigData()
    {
      oappend(SET_F("addInfo('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F(":great")); oappend(SET_F("',1,'<i>(this is a great config value)</i>');"));
      oappend(SET_F("addInfo('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F(":testString")); oappend(SET_F("',1,'enter any string you want');"));
      // oappend(SET_F("dd=addDropdown('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F("','lowestKeyValue');"));
      oappend(SET_F("addOption(dd,'Nothing',0);"));
      oappend(SET_F("addOption(dd,'Everything',42);"));
    }


    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
      // pianoEffects();
      mainLoop();
    }


    /**
     * handleButton() can be used to override default button behaviour. Returning true
     * will prevent button working in a default way.
     * Replicating button.cpp
     */
    bool handleButton(uint8_t b) {
      yield();
      // ignore certain button types as they may have other consequences
      if (!enabled
       || buttonType[b] == BTN_TYPE_NONE
       || buttonType[b] == BTN_TYPE_RESERVED
       || buttonType[b] == BTN_TYPE_PIR_SENSOR
       || buttonType[b] == BTN_TYPE_ANALOG
       || buttonType[b] == BTN_TYPE_ANALOG_INVERTED) {
        return false;
      }

      bool handled = false;
      // do your button handling here
      return handled;
    }


    /**
     * onStateChanged() is used to detect WLED state change
     * @mode parameter is CALL_MODE_... parameter used for notifications
     */
    void onStateChange(uint8_t mode) {
      // do something if WLED state changed (color, brightness, effect, preset, etc)
    }


    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_MIDIPIANO;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};


// add more strings here to reduce flash memory usage
const char usermod_v2_midiPiano::_name[]    PROGMEM = "LED Piano";
const char usermod_v2_midiPiano::_enabled[] PROGMEM = "enabled";
