# far2l keyboard extensions

M-Commander can take its keys from a terminal that speaks the far2l terminal
extensions (far2l's own terminal, and f4's). Such a terminal reports every key
with its virtual key code, character and Windows control key state, so the
combinations that collide in the legacy byte stream (Ctrl-Enter and Enter,
Shift-Tab, Ctrl-Shift-letter, Alt with a function key, and so on) are told apart.

## Negotiation

- At start M-Commander sends `ESC _ far2l1 ESC \` together with its other terminal
  questions, before Primary Device Attributes. A terminal with the extensions
  answers `ESC _ far2lok ESC \` (or ends it with BEL). Every other terminal
  ignores the APC and behaves as before.
- No question is asked when `TERM` is `screen*` or `tmux*`, or when `MC_FAR2L=0`
  is set.
- Whenever M-Commander gives the terminal to a subshell or a child program
  (Ctrl-O, running a command, an external editor or viewer) it sends
  `ESC _ far2l0 ESC \` first, and `ESC _ far2l1 ESC \` when it takes the terminal
  back and at start; the same goes for the exit. The child never gets a
  terminal that is still in far2l mode.
- With the extensions on, M-Commander does not turn on the kitty keyboard
  protocol or Win32 input mode: they would only carry the same information twice.
  A terminal without the extensions keeps using whichever of the two it knows.

## Keys

A key is `ESC _ f2l <base64> ESC \` (or BEL as the terminator). The payload is a
stack popped from the end, values little endian, the last byte the command:

| command | fields, in the order they are popped |
| --- | --- |
| `K` / `k` | character u32, control key state u32, scan code u16, virtual key u16, repeat count u16 |
| `C` / `c` | character u16, control key state u16, virtual key u8 |

Capital is a key press, small a release. A press is turned into a key by the same
rules as a Win32 input mode record (see the Win32 input mode change), and a release
is no key. Replies and resize packets are read and dropped.

## Mouse

far2l's terminal sends its mouse only as `M` packets (and the compact `m`) once the
extensions are on, never as xterm reports. M-Commander turns a packet into the SGR
report (`ESC [ < b ; x ; y M`) it asks for from other terminals and hands it back to
the keyboard, so the mouse code has one input: presses, releases, wheel turns and,
in button-event tracking, moves with a button held. Moves with nothing held and
horizontal wheel turns are not reported, and nothing is if the mouse is off.

A character outside ASCII in a key packet goes back to the keyboard as UTF-8 and is
read as a typed character.

## Plugin authors

Nothing changes for plugins: a key is the same integer code as before, whichever
way the terminal reported it, and a terminal without the extensions behaves exactly
as it did.

## Checking by hand

The tests in `tests/src/tty_far2l_input.c` replay packets through the real decoder.
By hand: in far2l's terminal or f4 run `mcommander`, press Ctrl-Enter, Shift-Tab and
Alt-F4 in the panels and see that each does its own action; run with
`MC_FAR2L=0` to compare with the legacy stream.

## Dropped files

A terminal that also has the far2l drag and drop protocol (far2l with it, f4; the
specification is docs/FAR2L_DND.md of https://github.com/unxed/f4) lets files be dragged
from the desktop onto the M-Commander window. Right after the extensions are switched on
M-Commander binds drop reception (BIND), and unbinds before the extensions go off, so a
child program never gets a drop. A terminal without the protocol answers nothing or an
empty reply, and nothing changes.

A drop is one small event naming an offer. M-Commander lists it (LIST), reads the files
one bounded chunk at a time (READ) and releases the offer (CLOSE): nothing travels unasked,
so the same works over SSH. The files go by M-Commander's own file access into the
directory of the panel under the drop, so a panel that shows an archive or a remote host
is no special case. If the terminal does not say where the drop landed, M-Commander asks
before it copies to the active panel. A drop is taken only while the panels are on top,
not in a dialog, the viewer or the editor. Existing files ask for overwrite, skip,
overwrite all or cancel, and a file that could not be received whole is removed.

Only plain files are taken, and only names that are plain names: a name with a slash, `..`
or a control character is refused. Every request is answered within 20 seconds or the drop
is given up.

Not there yet: directories, a bigger window than one request at a time, a progress bar
and Esc to cancel a long copy, and dropping into the built-in terminal for the programs
that run in it.

### Programs in the built-in terminal

The built-in terminal (mcterm) is a far2l terminal for the program that runs in it, so that
a drop reaches it, and mc inside mc inside a far2l terminal works too. It answers
`ESC _ far2l1 BEL` only while there is something to pass on: the terminal M-Commander itself
runs in has the extensions on and drop reception bound. Any other program sees a plain terminal,
as before.

From then on the program is told as a far2l terminal tells it. Its keys come as far2l key events
(mc's own keys, turned back into a virtual key, a character and the control key state), and a
file dropped on the outer window over the terminal is announced to it as a drop of its own, at
the cell of the terminal it landed on. The program's LIST and READ are answered with LIST and
READ of the outer offer, one chunk at a time, nothing kept in between; its CLOSE releases the
outer offer, and so do `ESC _ far2l0 BEL`, a new binding, the lease running out (600 seconds)
and the terminal closing. The other far2l interactions (the clipboard, images and the like) are
answered with the empty reply, which is how a far2l says it has none.

The terminal serves what the outer one has: plain files, one READ at a time, at most 8 offers and
chunks of at most 32 KiB. Mouse events of the extensions are not passed to the program yet.
