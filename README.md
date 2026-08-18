# blink

Animations for an addressable LED strip, running on a Seeed Studio XIAO
ESP32-C6 under ESP-IDF. The board picks an animation at random, runs it to
completion, and picks again, forever. There is no network, no input and nothing
to configure at runtime.

The code is all in `main/`: the driver in `main.cpp`, a header per animation,
and the shared colour headers beside them. `Fibonacci.hpp` is left over from
something else and is included by nothing.

## Hardware

- Seeed Studio XIAO ESP32-C6. RISC-V, ESP32-C6-WROOM-1-N4 module, 4MB flash.
- One strip of 50 pixels (`NUM_PIXELS` in `main/Constants.h`).
- Data on GPIO 0, silkscreened D0 (`PIN_WS2812B`).

Three wires: 5V, ground, and D0 to the strip's data in. Power comes through the
board's USB input.

`configure_led` drives the strip as WS2812 in GRB order. Which part the strip
physically is has never been checked; if the bit timing is wrong, that is the
line to change.

Full white across the strip draws about 3A, past what the board's USB input
carries. Pixels go out below full by default (`BRIGHTNESS_SCALE`).

## Building and flashing

```
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

The port path is the macOS one. The console is the native USB Serial/JTAG
peripheral on the same port used to flash, since this board has no USB-UART
bridge.

`sdkconfig` is not tracked. `sdkconfig.defaults` sets the target, the console
and the flash size, with the reason at each line, and a build that skips it
comes up as an esp32 where GPIO 0 is a strapping pin and there is no console.
`led_strip` comes from the component registry through `main/idf_component.yml`.

## The loop

`app_main` stops wifi, enables the bootloader entropy source (wifi and BLE are
the usual RNG and neither runs here), configures the strip, and loops on one of
three drivers in `main.cpp`. `randomlySelect` ships; `inOrder` and `single` are
commented out.

Per run the driver calls `setup()`, asks `duration()`, and divides by the 10ms
tick for a frame count. Each frame it computes progress from 0 to 65535, passes
it through the animation's curve, blanks the buffer, calls `render(t)`, and
refreshes. The tick is 10ms because `CONFIG_FREERTOS_HZ` is 100, the shortest
interval `vTaskDelay` can hold.

## Writing an animation

`main/Animation.hpp`, four members to implement:

- `setup()` decides the run, and is the only place entropy is drawn.
- `duration()` in milliseconds. It is asked after `setup()`, so it may depend on
  what `setup()` drew.
- `render(uint16_t t)` draws the strip at progress `t`, as a function of `t` and
  of what `setup()` decided and of nothing else. Two calls at the same `t` draw
  the same strip.
- `tag()` returns a per-class number. Nothing reads it.

`curve()` is optional and defaults to the trapezoid below.

`render` carrying no history is what lets a curve hold, double back or
oscillate rather than only ramp, and the driver's per-frame blank enforces it.
An animation with no closed form walks to its state from the opening state on
every call, in locals. A step wanting a random number hashes its own index
against the seed `setup()` drew (`noise()`), so the walk lands the same way
however it is reached while runs still differ from one another. Sort is the
worked example: it runs the real algorithm during `setup()` and records every
operation, then replays that queue from the shuffle.

To add one: write the header, construct an instance in `main.cpp`, put it in a
variants array, and add that array to `groups`. To watch one on its own, point
`single` at it and uncomment it in `app_main`.

## Curves

`main/Curve.hpp`. The default is `CurveTrapezoidInOut`: the rate ramps at each
end between a floor and a plateau, so a run opens and closes slowly without
standing still. Both halves of that are load bearing. A curve reaching zero rate
holds the opening state across many frames, which reads as a pause rather than
as an ease, and a curve running at twice the average rate through the middle
skips logical steps in the animations whose step count is near their frame
count.

Easing puts the middle above the average rate, so a duration tuned for a
constant rate is not the same number as one tuned for the rate the animation
cruises at. `Curve.hpp` states the ratio.

Ripple overrides to linear. Wave speed belongs to the medium.

## Colour

### Palette

The seven anchors in `main/Constants.h` were spaced by eye against this physical
strip, with the angle the arithmetic would have given beside each. Do not
replace them with computed angles.

The intervals that fall out of that differ by more than a factor of ten, so
related colours are read off the list (`analogous` in `main/color_utils.hpp`)
rather than by adding an offset: an offset that steps a whole colour at one end
of the palette lands inside a single colour at the other.

The palette does not close. Violet back to red has never been judged against
this strip and is wider than most of the measured intervals, so nothing steps
across it and the analogous window slides rather than wraps.

### Two complements

Both are in use and they are not the same thing.

- `COMPLEMENT` in `Constants.h` folds within the measured span, red through
  violet. It is less than half the wheel and it stays inside hues that have been
  looked at.
- `complementHue` in `color_utils.hpp` is half the full wheel, which is the
  channel-wise inverse exactly. With `level()` it also matches output across the
  pair.

### Value, and red

`actual_led_strip_set_pixel_hsv` takes a per-pixel value, absolute rather than a
fraction of the default, so an animation can ask for more output than the
default as well as less.

Red reads dimmer than green and blue as output falls, so its duty decays as
value raised to `RED_RESPONSE` rather than in step with theirs. The palette was
judged on the strip against that exponent, so the two move together. What it
means for an animation is that a hue carrying red turns toward red as it dims.
Ripple and Interference draw their hue from the green to blue arc, the only part
of the wheel with no red in it, so their fades to black do not also change
colour.

### Levelling

Two hues at the same value are not at the same output, since each sits on a
different channel mix. `level()` brings a set down to its dimmest member and
returns the level it used, so a hue arrived at later can be matched to the same
one. The die weights it does that with are a prior taken from the datasheet
rather than a measurement, and `color_utils.hpp` says where that is weakest.

The wheel is brought to duty steps before value is applied, and both stages
truncate. That order is load bearing: it is the one arrangement that reproduces
the tuned hues.

## The rotation

`main.cpp` registers an `Entry` per animation-and-variant and a `Group` per
animation. Selection draws a group first and a variant within it second, so
airtime is per animation rather than per entry. Flat, a variant would be a
ticket, and sort, which registers an entry per algorithm and dataset, would take
most of the strip's time while bounce took a single share of it.

Direction is two registrations rather than a flag. The group says the two are
one animation.

In the order the groups are declared:

- Full rainbow (`RainbowFull.hpp`). The wheel laid once across the strip and
  turned. Both directions.
- Rainbow slice (`RainbowSingleColorSlice.hpp`). The whole strip on one hue at a
  time, walking the wheel from red. Both directions.
- Dichromatic rainbow (`RainbowDichromatic.hpp`). Palette bands travelling along
  the strip. A band is one pixel shorter than the strip, so two hues are always
  up at once, and the ramp within a band is quadratic, so the arriving hue is
  crowded into the end of it. Both directions.
- Drop in (`DropIn.hpp`). Blocks sweep in from the far end and land on a pile
  that grows a place each sweep, until the strip is full, then the next colour.
  One colour relationship per run.
- Drop off (`DropOff.hpp`). A block falls the length of the strip and leaves
  nothing behind it, once per anchor.
- Fill in (`FillIn.hpp`). Pixels light one at a time over what the band before
  left standing, one band per anchor. Both directions.
- Bounce (`Bounce.hpp`). A head with a short fading tail runs the strip and
  turns at each end, on a background of its complement. Each bounce drifts the
  hue and the drift stays.
- Multi rainbow (`MultiRainbow.hpp`). Two to six rainbows across the strip, all
  turning together. Both directions.
- Cellular automaton (`CellularAutomaton.hpp`). An elementary Wolfram rule over
  the strip as a ring, from a single live cell. The rule is drawn per run from a
  pool measured on this ring, and logged, so one worth keeping can be pinned by
  number through the constructor. Cells that were alive last generation too take
  a second hue, and a state repeating a rotation of one already seen reseeds at
  random.
- Life automaton (`LifeAutomaton.hpp`). An outer-totalistic B/S rule on a board
  ten strips on a side, seen through a window the size of the strip that stands
  where the board is busiest and crosses to somewhere else when it is not. Cells
  alive last generation too take a second hue and brightness counts how many of
  the last eight generations a cell was alive in, so a glider, a blinker and a
  block read differently. The rule is drawn per run from the 194431 members of
  the family that hold both hues for at least half a run, which are carried as a
  membership blob (`tools/liferules`), and logged so one worth keeping can be
  pinned by its code through the constructor.
- Theater chase (`TheaterChase.hpp`). Lit dots at a fixed spacing running along
  the strip with the gaps unlit, alternating between a hue and its complement so
  the direction of travel is legible. Both directions.
- Interference (`Interference.hpp`). Nodes and antinodes crawling along the
  strip at one hue, from two sine carriers of different wavelength summed and
  rectified onto value. The wavelengths are near the golden ratio apart, so the
  beat does not repeat within a run.
- Collision (`Collision.hpp`). Two dots running the strip, bouncing off the ends
  and off each other, with friction. A hard speed mismatch fuses the pair and
  fuses their hues, so they leave together as one colour, and a pair that has
  been still too long is thrown again.
- Ripple (`Ripple.hpp`). Still water at one hue and a low value, with impulses
  raising fronts that travel out both ways, reflect off the ends, and lose
  amplitude to distance and to each reflection. Crests add where they cross.
- Sort (`Sort.hpp`). The strip is an array of distinct keys drawn as a hue ramp,
  shuffled and then sorted. Seven algorithms (bitonic, quick, radix, merge,
  insertion, selection, heap) over two datasets, the full palette or a few
  adjacent anchors, one entry each. Operation counts differ by an order of
  magnitude between algorithms while the replay window is fixed, so the
  difference reads as how fast the strip moves rather than as how long the run
  takes. The pixels the last operation touched are drawn at full output.

### Not in the rotation

- Twinkle (`Twinkle.hpp`). A background hue with a few pixels lit in a drifted
  complement, never two adjacent. Commented out of the group table: a pixel is
  the shimmer hue or the background and nothing in between, from before there
  was a per-pixel value to fade one out on.
- BlinkComplement (`BlinkComplement.hpp`). Alternating bands of a hue and its
  complement, swapping, with one side optionally drifting per swap. Four
  entries, commented out. Its pair is the `Constants.h` fold and is not matched
  in output. These come back when they use `color_utils.hpp`.
- FlashWhite (`FlashWhite.hpp`). The whole strip strobed white, for checking the
  strip is alive. Its instance is commented out at the top of `main.cpp`. White
  has no hue and cannot go through the usual pixel path, so the brightness
  scales are repeated inside it.
