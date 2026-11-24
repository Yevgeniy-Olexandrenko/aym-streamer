---
name: prepare-commit
description: Prepare the work in the aym-streamer working tree for a commit. Review the new and changed firmware code and algorithms, refactor where it helps, and cut the comments down to the essentials in Simplified Technical English. Use this skill whenever the user gets ready to commit or asks to prepare, clean up, polish or review the changes before a commit ("готовим к коммиту", "подготовь к коммиту", "давай закоммитим", "почисти перед коммитом", "ревью перед коммитом"), even when the skill is not named. The skill never commits; the user commits the work.
---

# Prepare a commit

The user reviews every change and commits it. This skill makes the working
tree ready for that: the changed code is reviewed and refactored where that
helps, and its comments are short and clear. Do not commit, stage, push or
amend, and do not offer to commit (CLAUDE.md forbids it).

## 1. Find the scope

- Run `git status --short` and `git diff --stat`. The modified and the new
  (untracked) source files are the scope.
- Use `git diff` to see which code is new. Review the new and changed code
  and the code it calls; leave older code alone unless the change touches it.
- Read each changed region as a whole, not only the hunks: a refactor needs
  the context.
- `firmware_old/`, `software/`, `datasheet/` and the files in `hardware/`
  (Gerbers, schematics, BOMs) are reference material, not code to review.
  Ask when it is not clear what belongs to the change.
- If the change touches the serial protocol (`firmware/src/uart/`), the
  host side in psg-tools (`psg_audio/sources/player/streamer/Streamer.cpp`,
  `ChipEncoder`) is in scope too: both sides must match.

## 2. Review and refactor

Look for these problems and fix them:

- Bugs: off-by-one, overflow (check the maximum of each fixed-point value
  and of each 8-bit counter or index), signed and unsigned mixes, rounding,
  division by zero, uninitialized members.
- Interrupts: data shared with an ISR is `volatile`; a multi-byte value that
  an ISR changes is read and written with interrupts off (`ATOMIC_BLOCK`);
  ISRs are short and do not wait; ring buffer indexes wrap correctly.
- AVR limits: 2 KB RAM (watch globals, buffers and stack depth), 32 KB
  flash. Constant tables go to `PROGMEM`. No heap, no floating point at run
  time, no 32-bit math in hot paths where 8 or 16 bits do.
- Bus timing: the delays in `psg/driver` keep the datasheet minimums for
  every supported chip.
- Algorithms: wrong results, a simpler equivalent, work per frame that a
  table or a cache removes.
- Leftovers of experiments: unused functions, parameters, members,
  constants and includes, debug output, temporary switches and defaults.
- Duplicated code that a small helper removes, names that do not tell what
  a thing is, magic numbers that need a named constant.
- Differences from the surrounding code: naming (`m_` members, `snake_case`
  private helpers, `PascalCase` public methods), namespaces (`PSG::`), tabs,
  braces on their own line, direct register access instead of the Arduino
  runtime.
- The rules of CLAUDE.md.

Refactor only where the result is clearly better: simpler, safer, smaller,
or faster where speed matters. Keep the behavior; if a fix changes it, say
so in the report.

## 3. Comments

Rewrite the comments of the changed code in Simplified Technical English
(ASD-STE100):

- Short sentences: not more than 20 words for an instruction, 25 for a
  description. One idea in each sentence.
- Simple words with one meaning. Use the same word for the same thing.
- Active voice and present tense.
- Write only what the code does not show: why, units and fixed-point
  formats, the source of a formula or a constant (datasheet, page), limits,
  ISR rules, chip quirks. Remove comments that only repeat the code.
- Keep the comments few: a header for a non-obvious class or function, one
  line before a tricky block. Match the comment style of the file.

Example:

    before: // lock update timer and enable interrupts
            // to handle incoming data in the background
    after:  // Let the UART ISR run while the chip is updated.

## 4. Verify

- Build the firmware with `arduino-builder` (see CLAUDE.md) with
  `-warnings all`. It must build without new warnings.
- Compare flash and RAM usage with the build of the last commit (build it
  from a `git stash` or a `git worktree` copy in the scratchpad) and report
  the difference.
- If psg-tools changed too, build it as its CLAUDE.md tells.
- The firmware runs only on the device: the user checks it on hardware.
  Say what must be checked there (chip types, stereo modes, AY8930).
- Delete the scratch build folders.

## 5. Report

Reply in the user's language. Tell:

- what the review found and fixed, bugs first; what was refactored and removed;
- how the comments changed, in short;
- what was built and checked, the flash/RAM figures, and what was not checked;
- the open questions that the user must decide.

## 6. Commit message

End the report with a short commit message for the user to copy, in a
fenced code block: one line in English, not longer than about 70
characters, in the style of the recent commits (`git log --oneline -10`),
that names the main changes. Add a body of a few short lines only when
one line cannot cover the change. Do not commit with it. If psg-tools
changed too, give a separate message for it.
