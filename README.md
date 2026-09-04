## Dice Roll BIP-39

Coinflip is an offline, transparent, verifiable, inexpensive and easy to use tool for generating a 24-word BIP-39 mnemonic from coin flips or binary dice rolls.  It runs on either the Waveshare RP2350 4.3inch Capacitive Touch Display Development Board (RP2350-Touch-LCD-4.3B-BOX) or on the STM32F469I Discovery Development Board (STM32F469I-DISCO), using an integrated 800 x 480 touchscreen.  The Waveshare board is the preferred option. The Waveshare board comes with an optional case and is easier to program. At the time of writing the STM32 board has proven difficult to source.

The software supports the official BIP-39 wordlists in English, French, Spanish, Italian, Czech and Portuguese.

![Coinflip running on the Waveshare board](docs/images/waveshare.png)


In terms of security, the most critical part of your Bitcoin wallet is the private master key. So, do you trust the software in your wallet to opaquely generate your keys for you? I’m looking at the bag that my Coldcard hardware wallet came in. On the bag, in bold letters, it says *“DON’T TRUST. VERIFY”*. If you’ve been following the Coldcard fiasco you can see the irony.

BIP-39 defines how to generate your keys from a random selection of 24 words out of a sequenced list of 2048 words. Lose your wallet and you can regenerate it with your 24-word seed phrase (plus your derivation path).

Each of the first 23 words can be randomly selected by flipping a coin eleven times. Let heads be a binary 0 and tails be a binary 1. Or you can use binary dice, eleven of them for one word.  Sequence those eleven bits and convert the resulting binary number to decimal, add 1, then look up the corresponding number in the BIP-39 word list of your preferred language. Do that 23 times and you have 23 randomly selected seed words. The remaining three coin flips are combined with the checksum to determine the 24th word. No computer, no algorithm, no software-generated random number, just *100% randomness, 100% transparency*.


The problem is that it’s tedious to flip a coin 256 times, write the numbers down, convert them to decimal and look them up in the BIP-39 word list. And then you have to calculate the checksum for the 24th word. The coin flip to BIP-39 converter makes that process easier, *transparently*.

## Independently verify the source code

This project is deliberately open source so that you do not need to trust the firmware blindly. Before using it to generate a seed for real funds, consider asking an AI coding tool such as ChatGPT or Codex to audit the GitHub repository directly.

Go to the repository at https://github.com/dgnelsonoz/dice-roll-bip39.  On the right hand side of the page, under **Releases**, click on the latest release.  At the bottom of the page under **Assets** you will a signed file called bip39-vx.x.x.tar.gz.  Download the file and submit it to your AI agent along with the following prompt:

```
Audit this GitHub repository's BIP39 mnemonic-generation code.

https://github.com/dgnelsonoz/dice-roll-bip39

The source code is attached.

Do not modify any files.

Trace the code from the user's coin flips or binary dice rolls all
the way to the displayed 24-word BIP39 mnemonic.

Verify specifically that:

1. Each set of 11 binary inputs is converted to the intended integer
   in the range 0-2047, with the correct bit order.

2. That integer maps to exactly the corresponding entry in the
   official BIP39 wordlist.

3. For a 24-word mnemonic, the first 23 words encode 253 bits of
   user-provided entropy.

4. The remaining 3 entropy bits are handled correctly.

5. SHA-256 is applied according to BIP39 and the correct 8 checksum
   bits are appended to the 256 bits of entropy.

6. The resulting final 11-bit value selects the correct 24th word.

7. Every BIP39 wordlist bundled with this repository exactly matches
   the corresponding official BIP39 wordlists, including ordering,
   spelling and number of entries.

8. There is no RNG, PRNG, hardware random-number generator, or other
   source of entropy mixed into or substituted for the user's coin
   flips/dice rolls.

9. Compare the implementation against official BIP39 test vectors
   where applicable.

Report any discrepancy, even if it appears harmless.

For every conclusion, cite the relevant filename, function and line
numbers in this repository and the corresponding requirement in
BIP39.
```
## How it works

Flip a coin eleven times to generate each seed word.  Or roll a binary dice eleven times, or eleven binary dice in one go and line them up randomly.  Enter the binary digits, 1s and 0s, into the converter and watch a seed word appear along with its decimal position in the word list. The position in the word list is the binary to decimal conversion of your eleven bit binary number plus one.  We add one because computers start counting from zero, we start counting from the number one. 

As an extra security check, you can download a BIP-39 wordlist that has both the word's decimal number position and its binary equivalent.  The middle row of the converter displays the word's binary number (coinflips) and its index into the list.  Look up the word by its index, compare the binary number in the list with the binary number on the display.  If they are the same you can feel confident that the Coinflip converter is genuine.  Looking in an independent word list is optional, but remember, *don't trust, verify.*


You can find the official BIP-39 word lists [here](https://github.com/bitcoin/bips/blob/master/bip-0039/english.txt).

You can find an English list with the binary numbers included [here](https://github.com/hatgit/BIP39-wordlist-printable-en/blob/master/BIP39-en-printable.txt).

For maximum security, plug the power cable into a 5 V USB wall plug and not the USB port on your computer.

The seed phrase is stored in volatile memory and will be lost when powered down; no sensitive data are stored in permanent memory.  You will have to write the phrase down and find a way to store it securely.

## Flashing the Waveshare board

Download the `.uf2` file for your desired language from the **latest GitHub release**.  Connect the Waveshare board to your computer with the USB cable.  Put the board into BOOTSEL mode by holding the BOOT button, press and release the RESET button and then release the BOOT button. Copy
`coinflip-rp2350-<language-version>.uf2` to the mounted `RP2350`/`RPI-RP2` drive. 

## Flashing the STM32 board

Download the `.elf` file for your desired language from the **latest GitHub release** and install [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).

Connect the STM32F469I-DISCO to your computer using a **USB Mini-B data cable** connected to the **ST-LINK USB connector (CN1)**. Do not use the other USB connector on the board.

From a Terminal command line, flash the firmware with:

```bash
STM32_Programmer_CLI -c port=SWD -w coinflip-stm32-<language-version>.elf -v -rst
```

## Building from source

## Prerequisites

Both boards require
* [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`)
* GNU Make
* CMake

The Waveshare board requires
* [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

The STM32 board requires
* [STM32CubeF4](https://www.st.com/en/embedded-software/stm32cubef4.html) — V1.28.0 was used during development
* STM32CubeProgrammer for flashing the board


The repository contains an RP2350 implementation in `RP2350/` and an STM32 implementation in `STM32/`.  The two boards can be built independently, they don't have to be built together.

From the coinflip-bip39 directory:
```bash
make stm
make rp
make flash-stm
make flash-rp
make test
```

The default external SDK layout is:

```text
your-build-location/
├── coinflip-bip39/
├── STM32CubeF4/
└── pico-sdk/
```

Override SDK locations when necessary:

```bash
make stm CUBE=/path/to/STM32CubeF4
make rp PICO_SDK_PATH=/path/to/pico-sdk
```

## Make command summary

From the project root:

```bash
make generate-wordlists       # Generate all six wordlists
make stm                      # Build all six language specific STM32 ELF files
make rp                       # Build all six language specific RP2350 ELF/UF2 files
make flash-stm                # Build and flash English STM32 firmware
make flash-rp                 # Build and flash English RP2350 firmware
make flash-stm LANGUAGE=french
make flash-rp LANGUAGE=french # Flash a selected language, defaults to english
make release-stm VERSION=1.2.0
make release-rp VERSION=1.2.0 # Package all six releases
make test                     # Run tests for both platforms
make audit                    # Audit all variants for flash-write symbols
make clean                    # Remove temporary build files
```
