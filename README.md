# ESP32 GateControlServer
## THIS IS NOT A REPLACEMENT FOR A REMOTE KEYFOB, ALWAYS HAVE A DIFFERENT WAY OF OPENING YOUR GATE. I AM NOT RESPONSIBLE FOR YOU GETTING LOCKED OUT OF YOUR OWN PROPERTY.

 ESP32 software that allows you to wirelessly control gates to let yourself in and out of your property

 To configure, edit the `settings_template.h` header file in `./include` directory and rename it to `settings.h`, there you will be adjust the firmware to your specifications.
 I have only implemented 4 actions, as there was no need for more:
 1. Open the gate
 2. Close the gate
 3. Stop the gate
 4. Pedestrian mode (opens the gate slightly)

One thing to note is that I haven't implemented any extra security measures for the gate, so the best way of securing this is to simply use complex URL's for the gate actions, so they cannot be easily guessed, like a password.

There are many different ways of wiring the ESP32 to the gate, in my scenario I connected it to 4 relays, which bridged the 12V line of the gate to its own button contacts, thus acting like if it was wired to a physical button.