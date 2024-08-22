light animations used on esp32-s3-wroom to power ws2811's

5v, ground, and a single gpio pin (pin 12) are used in here

the following animations exist and are in good shape (ready for use):
1. Bounce. The background is set to a random hue, and a complementary color is used to create a comet that bounces back and forth across the strip
2. DropIn. The whole strip of lights will light up, and then drop the last 4 lights in place. This repeats until the whole strip is filled up.
3. DropOff. Inverse of DropIn; the whole strip of lights will light up, and drop the last 4 lights off. this process repeats until the whole strip is off.
4. FillIn. All lights start in the off position, with a single light turning on at a time. The next light (following ROYGBIV with some color drift) will then fill in next.
5. MultiRainbow. Show 1-6 rainbows, and move them around the strip slowly.
6. RainbowFull. Show 1 really big rainbow, and move it around the strip slowly.
7. RainbowSingleColorSlice. The whole strip is set to a single color (red or purple) and then transitions through all the colors of the rainbow.
8. Twinkle. The whole strip is set to a single color in the rainbow, and then up to 24 lights will 'twinkle' on or off in a complementary color.

The following animations exist and are in bad shape (not recommended for use):
1. BlinkComplement. Pick a starting color. Find the complementary color. alternate the two colors every 4 lights, swapping every few milliseconds.
2. Blinkvolution. Similar to BlinkComplement, but the colors drift around over time.
3. RainbowDichromatic. A rainbow animation that has 2 colors, with a third animating in and a fourth animating out.

The following animation exists but should only be used for debugging:
1. FlashWhite. This animation will make all the lights bright white, then turn them off. Very quickly. Without any diffusing, this is not a pleasant experience