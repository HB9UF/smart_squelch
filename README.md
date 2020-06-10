HB9UF Smart Squelch -- A squelch board with clever logic
========================================================

![3D Rendering](/documentation/3d_rendering.jpg)

A typical FM squelch for voice operation quantifies the amount of noise at frequencies above the voice frequency content and opens if said noise energy is sufficiently low. This works because without any input RF signal present, the FM detector produces wideband noise, reaching frequencies of several dozen kHz. A sufficiently strong RF input reduces that noise, the modulation emerges between typically 0 and 3 kHz and similarly, the noise above 3 kHz is reduced. Traditionally, a high-pass filter (HPF) attenuates the modulation while keeping the noise energy, and a subsequent peak-detector quantifies the amount of energy present, and finally a comparator circuit uses the peak detector output voltage to actuate the squelch.

Our squelch circuit is different. First, we use a microcontroller for the squelch logic. This allows us not only to change squelch thresholds without hardware access, but we can also implement more sophisticated squelching logic. For example, we can implement a hard threshold that is used when strong stations key off: The squelch is closed immediately, minimizing the squelch crash that is nothing but the manifestation of discriminator noise during the time-window after key-off and the closing of the squelch. At the same time, we can implement a second threshold for weak stations, e.g. stations far away, with low RF power, fading stations, or mobile stations with picket-fencing. Such stations may overcome the weak, but not the strong threshold. In this case, the squelch remains open for a predetermined time-window (e.g. 1 second) in order not to cut off the weak station. Note that this is superior to simply adding hysteresis to the aforementioned comparator, and that this is mimicking the behaviour of the [MICOR Squelch](http://www.repeater-builder.com/micor/micor-bi-level-squelch-theory.html) system.

The board also contains a number of extra circuitry such as an anti-aliasing low pass filter, a DC/DC converter etc. for integration with our repeater controller. This project is fully open source, the hardware is released under the CC-BY-SA license and the firmware is released under the GPL 2 license. Instructions for compiling and flashing the firmware are documented in a `README` file in the `firmware` subdirectory. The necessary tools and frameworks (ChibiOS and stm32flash) are also included there as git submodules. The circuitry was developed to improve the poor squelch performance of the Yaesu DR-1X repeater and is also successfully used in a number of homebrew 1.3 GHz repeaters.

![Schematic](/documentation/schematic.png)
