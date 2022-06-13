/*******************************************************************************

  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

  Mostly rewritten by couriersud in 2008

  Public documentation:

  - http://privatfrickler.de/blick-auf-den-chip-soundchip-general-instruments-ay-3-8910/
	Die pictures of the AY8910

  - US Patent 4933980

  Games using ADSR: gyruss

  A list with more games using ADSR can be found here:
		http://mametesters.org/view.php?id=3043

  TODO:
  * Measure volume / envelope parameters for AY8930 expanded mode
  * YM2610 & YM2608 will need a separate flag in their config structures
	to distinguish between legacy and discrete mode.

  The rewrite also introduces a generic model for the DAC. This model is
  not perfect, but allows channel mixing based on a parametrized approach.
  This model also allows to factor in different loads on individual channels.
  If a better model is developed in the future or better measurements are
  available, the driver should be easy to change. The model is described
  later.

  In order to not break hundreds of existing drivers by default the flag
  AY8910_LEGACY_OUTPUT is used by drivers not changed to take into account the
  new model. All outputs are normalized to the old output range (i.e. 0 .. 7ffff).
  In the case of channel mixing, output range is 0...3 * 7fff.

  The main difference between the AY-3-8910 and the YM2149 is, that the
  AY-3-8910 datasheet mentions, that fixed volume level 0, which is set by
  registers 8 to 10 is "channel off". The YM2149 mentions, that the generated
  signal has a 2V DC component. This is confirmed by measurements. The approach
  taken here is to assume the 2V DC offset for all outputs for the YM2149.
  For the AY-3-8910, an offset is used if envelope is active for a channel.
  This is backed by oscilloscope pictures from the datasheet. If a fixed volume
  is set, i.e. envelope is disabled, the output voltage is set to 0V. Recordings
  I found on the web for gyruss indicate, that the AY-3-8910 offset should
  be around 0.2V. This will also make sound levels more compatible with
  user observations for scramble.

  The Model:
					 5V     5V
					  |      |
					  /      |
  Volume Level x >---|       Z
					  >      Z Pullup Resistor RU
					   |     Z
					   Z     |
					Rx Z     |
					   Z     |
					   |     |
					   '-----+-------->  >---+----> Output signal
							 |               |
							 Z               Z
			   Pulldown RD   Z               Z Load RL
							 Z               Z
							 |               |
							GND             GND

Each Volume level x will select a different resistor Rx. Measurements from fpgaarcade.com
where used to calibrate channel mixing for the YM2149. This was done using
a least square approach using a fixed RL of 1K Ohm.

For the AY measurements cited in e.g. openmsx as "Hacker Kay" for a single
channel were taken. These were normalized to 0 ... 65535 and consequently
adapted to an offset of 0.2V and a VPP of 1.3V. These measurements are in
line e.g. with the formula used by pcmenc for the volume: vol(i) = exp(i/2-7.5).

The following is documentation from the code moved here and amended to reflect
the changes done:

Careful studies of the chip output prove that the chip counts up from 0
until the counter becomes greater or equal to the period. This is an
important difference when the program is rapidly changing the period to
modulate the sound. This is worthwhile noting, since the datasheets
say, that the chip counts down.
Also, note that period = 0 is the same as period = 1. This is mentioned
in the YM2203 data sheets. However, this does NOT apply to the Envelope
period. In that case, period = 0 is half as period = 1.

Envelope shapes:
	C AtAlH
	0 0 x x  \___
	0 1 x x  /___
	1 0 0 0  \\\\
	1 0 0 1  \___
	1 0 1 0  \/\/
	1 0 1 1  \```
	1 1 0 0  ////
	1 1 0 1  /```
	1 1 1 0  /\/\
	1 1 1 1  /___

The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
has twice the steps, happening twice as fast.

****************************************************************************

	The bus control and chip selection signals of the AY PSGs and their
	pin-compatible clones such as YM2149 are somewhat unconventional and
	redundant, having been designed for compatibility with GI's CP1610
	series of microprocessors. Much of the redundancy can be finessed by
	tying BC2 to Vcc; AY-3-8913 and AY8930 do this internally.

							/A9   A8    /CS   BDIR  BC2   BC1
				AY-3-8910   24    25    n/a   27    28    29
				AY-3-8912   n/a   17    n/a   18    19    20
				AY-3-8913   22    23    24    2     n/a   3
							------------------------------------
				Inactive            NACT      0     0     0
				Latch address       ADAR      0     0     1
				Inactive            IAB       0     1     0
				Read from PSG       DTB       0     1     1
				Latch address       BAR       1     0     0
				Inactive            DW        1     0     1
				Write to PSG        DWS       1     1     0
				Latch address       INTAK     1     1     1

***************************************************************************/

/**
AY-3-8910(A)/8914/8916/8917/8930/YM2149 (others?):
						_______    _______
					  _|       \__/       |_
   [4] VSS (GND) --  |_|1  *            40|_|  -- VCC (+5v)
					  _|                  |_
			 [5] NC  |_|2               39|_|  <- TEST 1 [1]
					  _|                  |_
ANALOG CHANNEL B <-  |_|3               38|_|  -> ANALOG CHANNEL C
					  _|                  |_
ANALOG CHANNEL A <-  |_|4               37|_|  <> DA0
					  _|                  |_
			 [5] NC  |_|5               36|_|  <> DA1
					  _|                  |_
			IOB7 <>  |_|6               35|_|  <> DA2
					  _|                  |_
			IOB6 <>  |_|7               34|_|  <> DA3
					  _|   /---\          |_
			IOB5 <>  |_|8  \-/ |   A    33|_|  <> DA4
					  _|   .   .   Y      |_
			IOB4 <>  |_|9  |---|   - S  32|_|  <> DA5
					  _|   '   '   3 O    |_
			IOB3 <>  |_|10   8     - U  31|_|  <> DA6
					  _|     3     8 N    |_
			IOB2 <>  |_|11   0     9 D  30|_|  <> DA7
					  _|     8     1      |_
			IOB1 <>  |_|12         0    29|_|  <- BC1
					  _|     P            |_
			IOB0 <>  |_|13              28|_|  <- BC2
					  _|                  |_
			IOA7 <>  |_|14              27|_|  <- BDIR
					  _|                  |_                     Prelim. DS:   YM2149/8930:
			IOA6 <>  |_|15              26|_|  <- TEST 2 [2,3]   CS2           /SEL
					  _|                  |_
			IOA5 <>  |_|16              25|_|  <- A8 [3]         CS1
					  _|                  |_
			IOA4 <>  |_|17              24|_|  <- /A9 [3]        /CS0
					  _|                  |_
			IOA3 <>  |_|18              23|_|  <- /RESET
					  _|                  |_
			IOA2 <>  |_|19              22|_|  == CLOCK
					  _|                  |_
			IOA1 <>  |_|20              21|_|  <> IOA0
					   |__________________|

[1] Based on the decap, TEST 1 connects to the Envelope Generator and/or the
	frequency divider somehow. Is this an input or an output?
[2] The TEST 2 input connects to the same selector as A8 and /A9 do on the 8910
	and acts as just another active high enable like A8(pin 25).
	The preliminary datasheet calls this pin CS2.
	On the 8914, it performs the same above function but additionally ?disables?
	the DA0-7 bus if pulled low/active. This additional function was removed
	on the 8910.
	This pin has an internal pullup.
	On the AY8930 and YM2149, this pin is /SEL; if low, clock input is halved.
[3] These 3 pins are technically enables, and have pullups/pulldowns such that
	if the pins are left floating, the chip remains enabled.
[4] On the AY-3-8910 the bond wire for the VSS pin goes to the substrate frame,
	and then a separate bond wire connects it to a pad between pins 21 and 22.
[5] These pins lack internal bond wires entirely.


AY-3-8912(A):
						_______    _______
					  _|       \__/       |_
ANALOG CHANNEL C <-  |_|1  *            28|_|  <> DA0
					  _|                  |_
		  TEST 1 ->  |_|2               27|_|  <> DA1
					  _|                  |_
	   VCC (+5V) --  |_|3               26|_|  <> DA2
					  _|                  |_
ANALOG CHANNEL B <-  |_|4               25|_|  <> DA3
					  _|    /---\         |_
ANALOG CHANNEL A <-  |_|5   \-/ |   A   24|_|  <> DA4
					  _|    .   .   Y     |_
	   VSS (GND) --  |_|6   |---|   - S 23|_|  <> DA5
					  _|    '   '   3 O   |_
			IOA7 <>  |_|7    T 8    - U 22|_|  <> DA6
					  _|     A 3    8 N   |_
			IOA6 <>  |_|8    I 1    9 D 21|_|  <> DA7
					  _|     W 1    1     |_
			IOA5 <>  |_|9    A  C   2   20|_|  <- BC1
					  _|     N  D         |_
			IOA4 <>  |_|10      A       19|_|  <- BC2
					  _|                  |_
			IOA3 <>  |_|11              18|_|  <- BDIR
					  _|                  |_
			IOA2 <>  |_|12              17|_|  <- A8
					  _|                  |_
			IOA1 <>  |_|13              16|_|  <- /RESET
					  _|                  |_
			IOA0 <>  |_|14              15|_|  == CLOCK
					   |__________________|


AY-3-8913:
						_______    _______
					  _|       \__/       |_
   [1] VSS (GND) --  |_|1  *            24|_|  <- /CHIP SELECT [2]
					  _|                  |_
			BDIR ->  |_|2               23|_|  <- A8
					  _|                  |_
			 BC1 ->  |_|3               22|_|  <- /A9
					  _|    /---\         |_
			 DA7 <>  |_|4   \-/ |   A   21|_|  <- /RESET
					  _|    .   .   Y     |_
			 DA6 <>  |_|5   |---|   -   20|_|  == CLOCK
					  _|    '   '   3     |_
			 DA5 <>  |_|6    T 8    -   19|_|  -- VSS (GND) [1]
					  _|     A 3    8     |_
			 DA4 <>  |_|7    I 3    9   18|_|  -> ANALOG CHANNEL C
					  _|     W 2    1     |_
			 DA3 <>  |_|8    A      3   17|_|  -> ANALOG CHANNEL A
					  _|     N C          |_
			 DA2 <>  |_|9      -        16|_|  NC(?)
					  _|       A          |_
			 DA1 <>  |_|10              15|_|  -> ANALOG CHANNEL B
					  _|                  |_
			 DA0 <>  |_|11              14|_|  ?? TEST IN [3]
					  _|                  |_
	[4] TEST OUT ??  |_|12              13|_|  -- VCC (+5V)
					   |__________________|

[1] Both of these are ground, they are probably connected together internally. Grounding either one should work.
[2] This is effectively another enable, much like TEST 2 is on the AY-3-8910 and 8914, but active low
[3] This is claimed to be equivalent to TEST 1 on the datasheet
[4] This is claimed to be equivalent to TEST 2 on the datasheet


GI AY-3-8910/A Programmable Sound Generator (PSG): 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
	factory mask adjustment but default was 0000 for the "common" part shipped
	(probably die "-100").
  Pins 24, 25, and 26 are /A9, A8, and TEST2, which are an active low, high
	and high chip enable, respectively.
  AY-3-8910:  Unused bits in registers have unknown behavior.
  AY-3-8910A: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8910 die is labeled "90-32033" with a 1979 copyright and a "-100" die
	code.
  AY-3-8910A die is labeled "90-32128" with a 1983 copyright.
GI AY-3-8912/A: 1 I/O port
  /A9 pin doesn't exist and is considered pulled low.
  TEST2 pin doesn't exist and is considered pulled high.
  IOB pins do not exist and have unknown behavior if driven high/low and read
	back.
  A7 thru A4 enable state for selecting a register can be changed with a
	factory mask adjustment but default was 0000 for the "common" part shipped
  AY-3-8912:  Unused bits in registers have unknown behavior.
  AY-3-8912A: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8912 die is unknown.
  AY-3-8912A or A/P die is unknown.
AY-3-8913: 0 I/O ports
  BC2 pin doesn't exist and is considered pulled high.
  IOA/B pins do not exist and have unknown behavior if driven high/low and read back.
  A7 thru A4 enable state for selecting a register can be changed with a
	factory mask adjustment but default was 0000 for the "common" part shipped
  AY-3-8913:  Unused bits in registers have unknown behavior.
  AY-3-8913 die is unknown.
GI AY-3-8914/A: 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
	factory mask adjustment but was 0000 for the part shipped with the
	Intellivision.
  Pins 24, 25, and 26 are /A9, A8, and TEST2, which are an active low, high
	and high chip enable, respectively.
  TEST2 additionally ?disables? the data bus if pulled low.
  The register mapping is different from the AY-3-8910, the AY-3-8914 register
	mapping matches the "preliminary" 1978 AY-3-8910 datasheet.
  The Envelope/Volume control register is 6 bits wide instead of 5 bits, and
	the additional bit combines with the M bit to form a bit pair C0 and C1,
	which shift the volume output of the Envelope generator right by 0, 1 or 2
	bits on a given channel, or allow the low 4 bits to drive the channel
	volume.
  AY-3-8914:  Unused bits in registers have unknown behavior.
  AY-3-8914A: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8914 die is labeled "90-32022" with a 1978 copyright.
  AY-3-8914A die is unknown.
GI AY-3-8916: 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
	factory mask adjustment; its mask is unknown. This chip was shipped
	with certain later Intellivision II systems.
  Pins 24, 25, and 26 are /A9, /A8(!), and TEST2, which are an active low,
	low(!) and high chip enable, respectively.
	NOTE: the /A8 enable polarity may be mixed up with AY-3-8917 below.
  AY-3-8916: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8916 die is unknown.
GI AY-3-8917: 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
	factory mask adjustment but was 1111 for the part shipped with the
	Intellivision ECS module.
  Pins 24, 25, and 26 are /A9, A8, and TEST2, which are an active low, high
	and high chip enable, respectively.
	NOTE: the A8 enable polarity may be mixed up with AY-3-8916 above.
  AY-3-8917: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8917 die is unknown.
Microchip AY8930 Enhanced Programmable Sound Generator (EPSG): 2 I/O ports
  BC2 pin exists but is always considered pulled high. The pin might have no
	bond wire at all.
  Pins 2 and 5 might be additional test pins rather than being NC.
  A7 thru A4 enable state for selecting a register are 0000 for all? parts
	shipped.
  Pins 24 and 25 are /A9, A8 which are an active low and high chip enable.
  Pin 26 is /SELECT which if driven low divides the input clock by 2.
  Writing 0xAn or 0xBn to register 0x0D turns on extended mode, which enables
	an additional 16 registers (banked using 0x0D bit 0), and clears the
	contents of all of the registers except the high 3 bits of register 0x0D
	(according to the datasheet).
  If the AY8930's extended mode is enabled, it gains higher resolution
	frequency and volume control, separate volume per-channel, and the duty
	cycle can be adjusted for the 3 channels.
  If the mode is not enabled, it behaves almost exactly like an AY-3-8910(A?),
	barring the BC2 and /SELECT differences.
  AY8930: Unused bits in registers have unknown behavior, but the datasheet
	explicitly states that unused bits always read as 0.
  I/O current source/sink behavior is unknown.
  AY8930 die is unknown.
Yamaha YM2149 Software-Controlled Sound Generator (SSG): 2 I/O ports
  A7 thru A4 enable state for selecting a register are 0000 for all? parts
	shipped.
  Pins 24 and 25 are /A9, A8 which are an active low and high chip enable.
  Pin 26 is /SEL which if driven low divides the input clock by 2.
  The YM2149 envelope register has 5 bits of resolution internally, allowing
  for smoother volume ramping, though the register for setting its direct
  value remains 4 bits wide.
  YM2149: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  YM2149 die is unknown; only one die revision, 'G', has been observed
	from Yamaha chip/datecode silkscreen surface markings.
Yamaha YM2203: 2 I/O ports
  The pinout of this chip is completely different from the AY-3-8910.
  The entire way this chip is accessed is completely different from the usual
	AY-3-8910 selection of chips, so there is a /CS and a /RD and a /WR and
	an A0 pin; The chip status can be read back by reading the register
	select address.
  The chip has a 3-channel, 4-op FM synthesis sound core in it, not discussed
	in this source file.
  The first 16 registers are the same(?) as the YM2149.
  YM2203: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  YM2203 die is unknown; three die revisions, 'D', 'F' and 'H', have been
	observed from Yamaha chip/datecode silkscreen surface markings. It is
	unknown what behavioral differences exist between these revisions.
	The 'D' revision only appears during the first year of production, 1984, on chips marked 'YM2203B'
	The 'F' revision exists from 1984?-1991, chips are marked 'YM2203C'
	The 'H' revision exists from 1991 onward, chips are marked 'YM2203C'
Yamaha YM3439: limited info: CMOS version of YM2149?
Yamaha YMZ284: limited info: 0 I/O port, different clock divider
  The chip selection logic is again simplified here: pin 1 is /WR, pin 2 is
	/CS and pin 3 is A0.
  D0-D7 are conveniently all on one side of the 16-pin package.
  Pin 8 is /IC (initial clear), with an internal pullup.
Yamaha YMZ294: limited info: 0 I/O port
  Pinout is identical to YMZ284 except for two additions: pin 8 selects
	between 4MHz (H) and 6MHz (L), while pin 10 is /TEST.
OKI M5255, Winbond WF19054, JFC 95101, File KC89C72, Toshiba T7766A : differences to be listed

AY8930 Expanded mode registers :
	Bank Register Bits
	A    0        xxxx xxxx Channel A Tone period fine tune
	A    1        xxxx xxxx Channel A Tone period coarse tune
	A    2        xxxx xxxx Channel B Tone period fine tune
	A    3        xxxx xxxx Channel B Tone period coarse tune
	A    4        xxxx xxxx Channel C Tone period fine tune
	A    5        xxxx xxxx Channel C Tone period coarse tune
	A    6        xxxx xxxx Noise period
	A    7        x--- ---- I/O Port B input(0) / output(1)
				  -x-- ---- I/O Port A input(0) / output(1)
				  --x- ---- Channel C Noise enable(0) / disable(1)
				  ---x ---- Channel B Noise enable(0) / disable(1)
				  ---- x--- Channel A Noise enable(0) / disable(1)
				  ---- -x-- Channel C Tone enable(0) / disable(1)
				  ---- --x- Channel B Tone enable(0) / disable(1)
				  ---- ---x Channel A Tone enable(0) / disable(1)
	A    8        --x- ---- Channel A Envelope mode
				  ---x xxxx Channel A Tone volume
	A    9        --x- ---- Channel B Envelope mode
				  ---x xxxx Channel B Tone volume
	A    A        --x- ---- Channel C Envelope mode
				  ---x xxxx Channel C Tone volume
	A    B        xxxx xxxx Channel A Envelope period fine tune
	A    C        xxxx xxxx Channel A Envelope period coarse tune
	A    D        101- ---- 101 = Expanded mode enable, other AY-3-8910A Compatiblity mode
				  ---0 ---- 0 for Register Bank A
				  ---- xxxx Channel A Envelope Shape/Cycle
	A    E        xxxx xxxx 8 bit Parallel I/O on Port A
	A    F        xxxx xxxx 8 bit Parallel I/O on Port B

	B    0        xxxx xxxx Channel B Envelope period fine tune
	B    1        xxxx xxxx Channel B Envelope period coarse tune
	B    2        xxxx xxxx Channel C Envelope period fine tune
	B    3        xxxx xxxx Channel C Envelope period coarse tune
	B    4        ---- xxxx Channel B Envelope Shape/Cycle
	B    5        ---- xxxx Channel C Envelope Shape/Cycle
	B    6        ---- xxxx Channel A Duty Cycle
	B    7        ---- xxxx Channel B Duty Cycle
	B    8        ---- xxxx Channel C Duty Cycle
	B    9        xxxx xxxx Noise "And" Mask
	B    A        xxxx xxxx Noise "Or" Mask
	B    B        Reserved (Read as 0)
	B    C        Reserved (Read as 0)
	B    D        101- ---- 101 = Expanded mode enable, other AY-3-8910A Compatiblity mode
				  ---1 ---- 1 for Register Bank B
				  ---- xxxx Channel A Envelope Shape
	B    E        Reserved (Read as 0)
	B    F        Test (Function unknown)

Decaps:
AY-3-8914 - http://siliconpr0n.org/map/gi/ay-3-8914/mz_mit20x/
AY-3-8910 - http://privatfrickler.de/blick-auf-den-chip-soundchip-general-instruments-ay-3-8910/
AY-3-8910A - https://seanriddledecap.blogspot.com/2017/01/gi-ay-3-8910-ay-3-8910a-gi-8705-cba.html (TODO: update this link when it has its own page at seanriddle.com)

Links:
AY-3-8910 'preliminary' datasheet (which actually describes the AY-3-8914) from 1978:
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_100.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_101.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_102.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_103.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_104.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_105.png
AY-3-8910/8912 Feb 1979 manual: https://web.archive.org/web/20140217224114/http://dev-docs.atariforge.org/files/GI_AY-3-8910_Feb-1979.pdf
AY-3-8910/8912/8913 post-1983 manual: http://map.grauw.nl/resources/sound/generalinstrument_ay-3-8910.pdf or http://www.ym2149.com/ay8910.pdf
AY-8930 datasheet: http://www.ym2149.com/ay8930.pdf
YM2149 datasheet: http://www.ym2149.com/ym2149.pdf
YM2203 English datasheet: http://www.appleii-box.de/APPLE2/JonasCard/YM2203%20datasheet.pdf
YM2203 Japanese datasheet contents, translated: http://www.larwe.com/technical/chip_ymopn.html

*******************************************************************************/

#include "SoundChip.h"
#include <string.h>
#include <math.h>

////////////////////////////////////////////////////////////////////////////////
// Constants And Defines
////////////////////////////////////////////////////////////////////////////////

#define BIT(x,n) (((x)>>(n))&1)

namespace
{
	enum
	{
		AY_AFINE    = 0x00,
		AY_ACOARSE  = 0x01,
		AY_BFINE    = 0x02,
		AY_BCOARSE  = 0x03,
		AY_CFINE    = 0x04,
		AY_CCOARSE  = 0x05,
		AY_NOISEPER = 0x06,
		AY_ENABLE   = 0x07,
		AY_AVOL     = 0x08,
		AY_BVOL     = 0x09,
		AY_CVOL     = 0x0a,
		AY_EAFINE   = 0x0b,
		AY_EACOARSE = 0x0c,
		AY_EASHAPE  = 0x0d,
		AY_PORTA    = 0x0e,
		AY_PORTB    = 0x0f,
		AY_EBFINE   = 0x10,
		AY_EBCOARSE = 0x11,
		AY_ECFINE   = 0x12,
		AY_ECCOARSE = 0x13,
		AY_EBSHAPE  = 0x14,
		AY_ECSHAPE  = 0x15,
		AY_ADUTY    = 0x16,
		AY_BDUTY    = 0x17,
		AY_CDUTY    = 0x18,
		AY_NOISEAND = 0x19,
		AY_NOISEOR  = 0x1a,
		AY_TEST     = 0x1f
	};

	const uint8_t MAP_8914_TO_8910[]
	{ 
		AY_AFINE,   AY_BFINE,    AY_CFINE,   AY_EAFINE,
		AY_ACOARSE, AY_BCOARSE,  AY_CCOARSE, AY_EACOARSE,
		AY_ENABLE,  AY_NOISEPER, AY_EASHAPE, AY_AVOL, 
		AY_BVOL,    AY_CVOL,     AY_PORTA,   AY_PORTB
	};

	const double AY_DAC_TABLE[]
	{
		0.0, 0.0,
		0.00999465934234, 0.00999465934234,
		0.01445029373620, 0.01445029373620,
		0.02105745021740, 0.02105745021740,
		0.03070115205620, 0.03070115205620,
		0.04554818036160, 0.04554818036160,
		0.06449988555730, 0.06449988555730,
		0.10736247806500, 0.10736247806500,
		0.12658884565500, 0.12658884565500,
		0.20498970016000, 0.20498970016000,
		0.29221026932200, 0.29221026932200,
		0.37283894102400, 0.37283894102400,
		0.49253070878200, 0.49253070878200,
		0.63532463569100, 0.63532463569100,
		0.80558480201400, 0.80558480201400,
		1.0, 1.0
	};

	const double YM_DAC_TABLE[] =
	{
		0.0, 0.0,
		0.00465400167849, 0.00772106507973,
		0.01095597772180, 0.01396200503550,
		0.01699855039290, 0.02001983672850,
		0.02436865796900, 0.02969405661100,
		0.03506523231860, 0.04039063096060,
		0.04853894865340, 0.05833524071110,
		0.06805523765930, 0.07777523460750,
		0.09251544975970, 0.11108567940800,
		0.12974746318800, 0.14848554207700,
		0.17666895552000, 0.21155107957600,
		0.24638742656600, 0.28110170138100,
		0.33373006790300, 0.40042725261300,
		0.46738384069600, 0.53443198291000,
		0.63517204547200, 0.75800717174000,
		0.87992675669500, 1.0
	};

	// duty cycle used for AY8930 expanded mode
	const uint32_t DUTY_CYCLES[9] =
	{
		0x80000000, // 03.125 %
		0xC0000000, // 06.250 %
		0xF0000000, // 12.500 %
		0xFF000000, // 25.000 %
		0xFFFF0000, // 50.000 %
		0xFFFFFF00, // 75.000 %
		0xFFFFFFF0, // 87.500 %
		0xFFFFFFFC, // 93.750 %
		0xFFFFFFFE  // 96.875 %
	};
}

////////////////////////////////////////////////////////////////////////////////
// Emulation Implementation
////////////////////////////////////////////////////////////////////////////////

SoundChip::SoundChip(ChipType chipType, PSGType psgType, int clockRate, int sampleRate)
	: m_chipType(chipType)
	, m_dacTable((!(m_chipType == ChipType::AY8930)) && (psgType == PSGType::AY) ? AY_DAC_TABLE : YM_DAC_TABLE)
	, m_counter(0)
	, m_interpolatorL{}
	, m_interpolatorR{}
	, m_firL{}
	, m_firR{}
	, m_firIndex(0)
	, m_dcFilterL{}
	, m_dcFilterR{}
	, m_dcFilterIndex(0)
	, m_step(double(clockRate) / (sampleRate * 8 * DECIMATE_FACTOR))
{
	Reset();
}

void SoundChip::Reset()
{
	m_mode = 0; // AY-3-8910 compatible mode
	memset(&m_regs, 0, sizeof(m_regs));

	for (int chan = 0; chan < NUM_CHANNELS; chan++)
	{
		m_tone[chan].Reset();
		m_envelope[chan].Reset();
	}
	m_noise.Reset();
}

void SoundChip::Write(uint8_t reg, uint8_t data)
{
	if (m_chipType == ChipType::AY8914)
	{
		// AY8914 has different register map
		reg = MAP_8914_TO_8910[reg & 0xF];
	}

	if ((reg & 0xf) == AY_EASHAPE) reg &= 0xf;
	m_regs[reg] = data;

	switch (reg)
	{
	case AY_AFINE:
	case AY_ACOARSE:
		m_tone[0].SetPeriod(is_expanded_mode(), m_regs[AY_AFINE], m_regs[AY_ACOARSE]);
		break;

	case AY_BFINE:
	case AY_BCOARSE:
		m_tone[1].SetPeriod(is_expanded_mode(), m_regs[AY_BFINE], m_regs[AY_BCOARSE]);
		break;

	case AY_CFINE:
	case AY_CCOARSE:
		m_tone[2].SetPeriod(is_expanded_mode(), m_regs[AY_CFINE], m_regs[AY_CCOARSE]);
		break;

	case AY_NOISEPER:
		m_noise.SetPeriod(is_expanded_mode(), m_regs[AY_NOISEPER]);
		break;

	case AY_AVOL:
		m_tone[0].SetVolume(m_regs[AY_AVOL]);
		break;

	case AY_BVOL:
		m_tone[1].SetVolume(m_regs[AY_BVOL]);
		break;

	case AY_CVOL:
		m_tone[2].SetVolume(m_regs[AY_CVOL]);
		break;

	case AY_EACOARSE:
	case AY_EAFINE:
		m_envelope[0].SetPeriod(m_regs[AY_EAFINE], m_regs[AY_EACOARSE]);
		break;

	case AY_ENABLE:
		// No action required
		break;

	case AY_EASHAPE:
		if (m_chipType == ChipType::AY8930)
		{
			const uint8_t old_mode = m_mode;
			m_mode = (data >> 4) & 0x0F;
			if (old_mode != m_mode)
			{
				// AY8930 expanded mode
				if (((old_mode & 0x0E) == 0x0A) ^ ((m_mode & 0x0E) == 0x0A))
				{
					for (int i = 0; i < AY_EASHAPE; i++)
					{
						SoundChip::Write(i, 0);
						SoundChip::Write(i + 0x10, 0);
					}
				}
			}
		}
		m_envelope[0].SetShape(m_regs[AY_EASHAPE]);
		break;

	case AY_EBFINE:
	case AY_EBCOARSE:
		m_envelope[1].SetPeriod(m_regs[AY_EBFINE], m_regs[AY_EBCOARSE]);
		break;

	case AY_ECFINE:
	case AY_ECCOARSE:
		m_envelope[2].SetPeriod(m_regs[AY_ECFINE], m_regs[AY_ECCOARSE]);
		break;

	case AY_EBSHAPE:
		m_envelope[1].SetShape(m_regs[AY_EBSHAPE]);
		break;

	case AY_ECSHAPE:
		m_envelope[2].SetShape(m_regs[AY_ECSHAPE]);
		break;

	case AY_ADUTY:
		m_tone[0].SetDuty(m_regs[AY_ADUTY]);
		break;

	case AY_BDUTY:
		m_tone[1].SetDuty(m_regs[AY_BDUTY]);
		break;

	case AY_CDUTY:
		m_tone[2].SetDuty(m_regs[AY_CDUTY]);
		break;

	case AY_NOISEAND:
		m_noise.SetMaskAND(m_regs[AY_NOISEAND]);
		break;

	case AY_NOISEOR:
		m_noise.SetMaskOR(m_regs[AY_NOISEOR]);
		break;

	default:
		m_regs[reg] = 0; // reserved, set as 0
		break;
	}
}

void SoundChip::Process(double& outL, double& outR)
{
	// The 8910 has three outputs, each output is the mix of one of the three
	// tone generators and of the (single) noise generator. The two are mixed
	// BEFORE going into the DAC. The formula to mix each channel is:
	// (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable).
	// Note that this means that if both tone and noise are disabled, the output
	// is 1, not 0, and can be modulated changing the volume.

	bool isExp = is_expanded_mode();
	m_noise.Update(isExp, m_chipType == ChipType::AY8930);

	for (int chan = 0; chan < NUM_CHANNELS; chan++)
	{
		m_tone[chan].Update(isExp);
		m_envelope[chan].Update();

		uint8_t enableT = BIT(m_regs[AY_ENABLE], 0 + chan);
		uint8_t enableN = BIT(m_regs[AY_ENABLE], 3 + chan);
#if 0
		int output = (m_tone[chan].GetOutput() & ~tone_enable(chan)) | (m_noise.GetOutput() & ~noise_enable(chan));
#else
		int output = (m_tone[chan].GetOutput() | enableT) & (m_noise.GetOutput() | enableN);
#endif
		if (output)
		{
			int tone_envelope = m_tone[chan].GetEField(isExp, m_chipType == ChipType::AY8914);

			if (tone_envelope != 0)
			{
				EnvelopeUnit& envelope = m_envelope[isExp ? chan : 0];
				int env_volume = envelope.GetVolume();

				if (m_chipType == ChipType::AY8930)
				{
					if (!isExp)
					{
						// AY8930 has 16 step envelope in comp mode
						env_volume |= 0x01;
					}
				}
				else
				{
					if (m_chipType == ChipType::AY8914)
					{
						// AY8914 has a two bit tone_envelope field
						env_volume >>= (3 - tone_envelope);
					}
				}
				output = env_volume;
			}
			else
			{
				output = m_tone[chan].GetVolume(isExp);
			}
		}

		outL += m_dacTable[output] * m_panL[chan];
		outR += m_dacTable[output] * m_panR[chan];
	}
}

////////////////////////////////////////////////////////////////////////////////
// Tone Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void SoundChip::ToneUnit::Reset()
{
	SetPeriod(false, 0, 0);
	SetVolume(0);
	SetDuty(0);

	m_counter = 0;
	m_dutyCounter = 0;
	m_output = 0;
}

void SoundChip::ToneUnit::Update(bool isExp)
{
	m_counter += (isExp ? 16 : 1);
	while (m_counter >= m_period)
	{
		m_counter -= m_period;
		m_dutyCounter = (m_dutyCounter - 1) & 0x1F;
		m_output = isExp ? BIT(m_dutyCycle, m_dutyCounter) : BIT(m_dutyCounter, 0);
	}
}

void SoundChip::ToneUnit::SetPeriod(bool isExp, uint8_t fine, uint8_t coarse)
{
	coarse &= (isExp ? 0xFF : 0x0F);
	m_period = fine | (coarse << 8);
	m_period |= (m_period == 0);
}

void SoundChip::ToneUnit::SetVolume(uint8_t volume)
{
	m_volume = volume;
}

void SoundChip::ToneUnit::SetDuty(uint8_t duty)
{
	m_dutyCycle = DUTY_CYCLES[(duty & 0x08) ? 0x08 : (duty & 0x0F)];
}

int SoundChip::ToneUnit::GetOutput() const
{
	return m_output;
}

int SoundChip::ToneUnit::GetVolume(bool isExp) const
{
	return (isExp ? (m_volume & 0x1F) : ((m_volume & 0x0F) << 1 | 0x01));
}

int SoundChip::ToneUnit::GetEField(bool isExp, bool isWide) const
{
	return (m_volume >> (isExp ? 5 : 4)) & (isWide ? 3 : 1);
}

////////////////////////////////////////////////////////////////////////////////
// Noise Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void SoundChip::NoiseUnit::Reset()
{
	SetPeriod(false, 0);
	SetMaskAND(0);
	SetMaskOR(0);

	m_prescale = 0;
	m_counter = 0;
	m_value = 0;
	m_shift = 1;
	m_output = 0;
}

void SoundChip::NoiseUnit::Update(bool isExp, bool isNew)
{
	// The Random Number Generator of the 8910 is a 17-bit shift
	// register. The input to the shift register is bit0 XOR bit3
	// (bit0 is the output). This was verified on AY-3-8910 and YM2149 chips.
	// AY8930 LFSR algorithm is slightly different, verified from manual.

	if (++m_counter >= m_period)
	{
		// toggle the prescaler output. Noise is no different to channels
		m_counter = 0;
		m_prescale ^= 1;

		// TODO : verify algorithm for AY8930
		if (isExp)
		{
			// AY8930 noise generator rate is twice compares as compatibility mode
			if (++m_value >= ((uint8_t(m_shift) & m_maskAND) | m_maskOR))
			{
				m_value = 0;
				m_shift = (m_shift >> 1) | ((BIT(m_shift, 0) ^ BIT(m_shift, 2)) << 16);
				m_output ^= 0x01;
			}
		}
		else if (!m_prescale)
		{
			m_shift = (m_shift >> 1) | ((BIT(m_shift, 0) ^ BIT(m_shift, (isNew ? 2 : 3))) << 16);
			m_output = (m_shift & 0x01);
		}
	}
}

void SoundChip::NoiseUnit::SetPeriod(bool isExp, uint8_t period)
{
	m_period = (period & (isExp ? 0xFF : 0x1F));
	m_period |= (m_period == 0);
}

void SoundChip::NoiseUnit::SetMaskAND(uint8_t mask)
{
	m_maskAND = (mask & 0xFF);
}

void SoundChip::NoiseUnit::SetMaskOR(uint8_t mask)
{
	m_maskOR = (mask & 0xFF);
}

int SoundChip::NoiseUnit::GetOutput() const
{
	return m_output;
}

////////////////////////////////////////////////////////////////////////////////
// Envelope Unit Implementation
////////////////////////////////////////////////////////////////////////////////

namespace
{
	enum { SU = +1, SD = -1, HT = 0, HB = 0, RT = 0x1F, RB = 0x00 };

	int envelopes[16][2][2]
	{
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SD, RT }, { SD, RT }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { SU, RB }, },
		{ { SD, RT }, { HT, RT }, },
		{ { SU, RB }, { SU, RB }, },
		{ { SU, RB }, { HT, RT }, },
		{ { SU, RB }, { SD, RT }, },
		{ { SU, RB }, { HB, RB }, },
	};
}

void SoundChip::EnvelopeUnit::Reset()
{
	SetPeriod(0, 0);
	SetShape(0);
}

void SoundChip::EnvelopeUnit::Update()
{
	if (++m_counter >= m_period)
	{
		m_counter = 0;

		m_volume += envelopes[m_shape][m_segment][0];
		if (m_volume < RB || m_volume > RT)
		{
			m_segment ^= 1;
			m_volume = envelopes[m_shape][m_segment][1];
		}
	}
}

void SoundChip::EnvelopeUnit::SetPeriod(uint8_t fine, uint8_t coarse)
{
	m_period = fine | (coarse << 8);
	m_period |= (m_period == 0);
}

void SoundChip::EnvelopeUnit::SetShape(uint8_t shape)
{
	m_shape = (shape & 0x0F);
	m_counter = 0;
	m_segment = 0;
	m_volume = envelopes[m_shape][m_segment][1];
}

int SoundChip::EnvelopeUnit::GetVolume() const
{
	return m_volume;
}

////////////////////////////////////////////////////////////////////////////////
// Resampling Implementation
////////////////////////////////////////////////////////////////////////////////

void SoundChip::SetPan(int chan, double pan, int is_eqp)
{
	if (is_eqp)
	{
		m_panL[chan] = sqrt(1 - pan);
		m_panR[chan] = sqrt(pan);
	}
	else
	{
		m_panL[chan] = 1 - pan;
		m_panR[chan] = pan;
	}
}

void SoundChip::Process()
{
	double* c_L = m_interpolatorL.c;
	double* y_L = m_interpolatorL.y;
	double* c_R = m_interpolatorR.c;
	double* y_R = m_interpolatorR.y;

	double* firL = &m_firL[FIR_SIZE - m_firIndex * DECIMATE_FACTOR];
	double* firR = &m_firR[FIR_SIZE - m_firIndex * DECIMATE_FACTOR];
	m_firIndex = (m_firIndex + 1) % (FIR_SIZE / DECIMATE_FACTOR - 1);

	double y;
	for (int i = DECIMATE_FACTOR - 1; i >= 0; --i)
	{
		m_counter += m_step;
		if (m_counter >= 1)
		{
			m_counter -= 1;

			y_L[0] = y_L[1];
			y_L[1] = y_L[2];
			y_L[2] = y_L[3];

			y_R[0] = y_R[1];
			y_R[1] = y_R[2];
			y_R[2] = y_R[3];

			m_outL = m_outR = 0;
			Process(m_outL, m_outR);
			y_L[3] = m_outL;
			y_R[3] = m_outR;

			y = y_L[2] - y_L[0];
			c_L[0] = 0.50 * y_L[1] + 0.25 * (y_L[0] + y_L[2]);
			c_L[1] = 0.50 * y;
			c_L[2] = 0.25 * (y_L[3] - y_L[1] - y);

			y = y_R[2] - y_R[0];
			c_R[0] = 0.50 * y_R[1] + 0.25 * (y_R[0] + y_R[2]);
			c_R[1] = 0.50 * y;
			c_R[2] = 0.25 * (y_R[3] - y_R[1] - y);
		}

		firL[i] = (c_L[2] * m_counter + c_L[1]) * m_counter + c_L[0];
		firR[i] = (c_R[2] * m_counter + c_R[1]) * m_counter + c_R[0];
	}

	m_outL = Decimate(firL);
	m_outR = Decimate(firR);
}

void SoundChip::RemoveDC()
{
	m_outL = FilterDC(m_dcFilterL, m_dcFilterIndex, m_outL);
	m_outR = FilterDC(m_dcFilterR, m_dcFilterIndex, m_outR);
	m_dcFilterIndex = (m_dcFilterIndex + 1) & (DC_FILTER_SIZE - 1);
}

double SoundChip::GetOutL() const
{
	return m_outL;
}

double SoundChip::GetOutR() const
{
	return m_outR;
}

double SoundChip::Decimate(double* x) const
{
	double y =
		-0.0000046183113992051936 * (x[ 1] + x[191]) +
		-0.0000111776164088722500 * (x[ 2] + x[190]) +
		-0.0000186102645020054320 * (x[ 3] + x[189]) +
		-0.0000251345861356310120 * (x[ 4] + x[188]) +
		-0.0000284942816906661970 * (x[ 5] + x[187]) +
		-0.0000263968287932751590 * (x[ 6] + x[186]) +
		-0.0000170942125588021560 * (x[ 7] + x[185]) +
		+0.0000237981935769668660 * (x[ 9] + x[183]) +
		+0.0000512811602422021830 * (x[10] + x[182]) +
		+0.0000776219782624342700 * (x[11] + x[181]) +
		+0.0000967594266641204160 * (x[12] + x[180]) +
		+0.0001024022930039340200 * (x[13] + x[179]) +
		+0.0000893446142180771060 * (x[14] + x[178]) +
		+0.0000548757001189491830 * (x[15] + x[177]) +
		-0.0000698390822106801650 * (x[17] + x[175]) +
		-0.0001447966132360757000 * (x[18] + x[174]) +
		-0.0002115845291770830800 * (x[19] + x[173]) +
		-0.0002553506910655054400 * (x[20] + x[172]) +
		-0.0002622871437432210400 * (x[21] + x[171]) +
		-0.0002225880592702779900 * (x[22] + x[170]) +
		-0.0001332323049569570400 * (x[23] + x[169]) +
		+0.0001618257876705520600 * (x[25] + x[167]) +
		+0.0003284617538509658100 * (x[26] + x[166]) +
		+0.0004704561157618486300 * (x[27] + x[165]) +
		+0.0005571385145753094400 * (x[28] + x[164]) +
		+0.0005621256512151872600 * (x[29] + x[163]) +
		+0.0004690191855396247800 * (x[30] + x[162]) +
		+0.0002762486683895298600 * (x[31] + x[161]) +
		-0.0003256417948683862200 * (x[33] + x[159]) +
		-0.0006518231028671038800 * (x[34] + x[158]) +
		-0.0009212778730931929800 * (x[35] + x[157]) +
		-0.0010772534348943575000 * (x[36] + x[156]) +
		-0.0010737727700273478000 * (x[37] + x[155]) +
		-0.0008855664539039263400 * (x[38] + x[154]) +
		-0.0005158189609076553400 * (x[39] + x[153]) +
		+0.0005954876719379527700 * (x[41] + x[151]) +
		+0.0011803558710661009000 * (x[42] + x[150]) +
		+0.0016527320270369871000 * (x[43] + x[149]) +
		+0.0019152679330965555000 * (x[44] + x[148]) +
		+0.0018927324805381538000 * (x[45] + x[147]) +
		+0.0015481870327877937000 * (x[46] + x[146]) +
		+0.0008947069583494130600 * (x[47] + x[145]) +
		-0.0010178225878206125000 * (x[49] + x[143]) +
		-0.0020037400552054292000 * (x[50] + x[142]) +
		-0.0027874356824117317000 * (x[51] + x[141]) +
		-0.0032103299880219430000 * (x[52] + x[140]) +
		-0.0031540624117984395000 * (x[53] + x[139]) +
		-0.0025657163651900345000 * (x[54] + x[138]) +
		-0.0014750752642111449000 * (x[55] + x[137]) +
		+0.0016624165446378462000 * (x[57] + x[135]) +
		+0.0032591192839069179000 * (x[58] + x[134]) +
		+0.0045165685815867747000 * (x[59] + x[133]) +
		+0.0051838984346123896000 * (x[60] + x[132]) +
		+0.0050774264697459933000 * (x[61] + x[131]) +
		+0.0041192521414141585000 * (x[62] + x[130]) +
		+0.0023628575417966491000 * (x[63] + x[129]) +
		-0.0026543507866759182000 * (x[65] + x[127]) +
		-0.0051990251084333425000 * (x[66] + x[126]) +
		-0.0072020238234656924000 * (x[67] + x[125]) +
		-0.0082672928192007358000 * (x[68] + x[124]) +
		-0.0081033739572956287000 * (x[69] + x[123]) +
		-0.0065831115395702210000 * (x[70] + x[122]) +
		-0.0037839040415292386000 * (x[71] + x[121]) +
		+0.0042781252851152507000 * (x[73] + x[119]) +
		+0.0084176358598320178000 * (x[74] + x[118]) +
		+0.0117256605746305500000 * (x[75] + x[117]) +
		+0.0135504766477886720000 * (x[76] + x[116]) +
		+0.0133881893699974960000 * (x[77] + x[115]) +
		+0.0109795012423412590000 * (x[78] + x[114]) +
		+0.0063812749416854130000 * (x[79] + x[113]) +
		-0.0074212296041538880000 * (x[81] + x[111]) +
		-0.0148645630434021300000 * (x[82] + x[110]) +
		-0.0211435846221781040000 * (x[83] + x[109]) +
		-0.0250427505875860900000 * (x[84] + x[108]) +
		-0.0254735309425472010000 * (x[85] + x[107]) +
		-0.0216273100178821960000 * (x[86] + x[106]) +
		-0.0131043233832255430000 * (x[87] + x[105]) +
		+0.0170651339899804760000 * (x[89] + x[103]) +
		+0.0369789192644519520000 * (x[90] + x[102]) +
		+0.0582331806209395800000 * (x[91] + x[101]) +
		+0.0790720120814059490000 * (x[92] + x[100]) +
		+0.0976759987169523170000 * (x[93] + x[99 ]) +
		+0.1123604593695093200000 * (x[94] + x[98 ]) +
		+0.1217634357728773100000 * (x[95] + x[97 ]) +
		+0.125 * x[96];

	memcpy(&x[FIR_SIZE - DECIMATE_FACTOR], x, DECIMATE_FACTOR * sizeof(double));
	return y;
}

double SoundChip::FilterDC(DCFilter& filter, int index, double x) const
{
	filter.sum += -filter.delay[index] + x;
	filter.delay[index] = x;
	return (x - (filter.sum / DC_FILTER_SIZE));
}
