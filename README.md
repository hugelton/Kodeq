# Reelia

Reelia is a live-coding environment for sequence generation with a focus on MIDI output and real-time performance. It's designed with a clean, intuitive syntax that makes it easy to create and manipulate patterns during live performances.

## Features

- **Intuitive Syntax**: Simple and clean syntax inspired by modern programming languages
- **Object-Oriented Design**: Create and manipulate sequence objects with attributes and methods
- **Real-Time Performance**: Tick-based system with automatic and manual control
- **Pattern Generation**: Binary patterns, counters, and sequencers
- **Parallel Execution**: Run multiple commands simultaneously with pipe syntax
- **MIDI Output**: Control external hardware or software synthesizers via MIDI
- **Cross-Platform**: Works on macOS, Linux, and Windows

## Syntax Overview

Reelia features a clean, expressive syntax:

### 1. Object Creation

```
$seq = @seq           // Create a sequence object
$cnt = @count         // Create a counter object
$note = @midi_note    // Create a MIDI note object
$cc = @midi_cc        // Create a MIDI CC controller
$mseq = @midi_seq     // Create a MIDI sequence
```

### 2. Attribute Access

```
$seq.data = b10101010   // Set binary pattern data
$cnt.max = 16           // Set maximum value for counter
x = $cnt.step           // Read an attribute
$cnt.step = x           // Write to an attribute
```

### 3. Method Calls

```
$seq.start()    // Start a sequence
$cnt.stop()     // Stop a counter
$cnt.reset()    // Reset a counter
$note.trigger() // Trigger a MIDI note
```

### 4. Parallel Execution

```
$seq.start() | $cnt.start()   // Start both objects in parallel
```

## MIDI Functionality

Reelia supports direct MIDI output to control external synthesizers:

### MIDI Device Management

```
@midi.list          // List available MIDI devices
@midi.device = 0    // Select MIDI output device
```

### MIDI Notes

```
$note = @midi_note     // Create MIDI note
$note.channel = 0      // Set MIDI channel (0-15)
$note.note = 60        // Set note number (60 = C4)
$note.velocity = 100   // Set velocity (0-127)
$note.duration = 1     // Set duration in ticks
$note.trigger()        // Play the note
```

### MIDI Control Changes

```
$cc = @midi_cc         // Create MIDI CC controller
$cc.channel = 0        // Set MIDI channel
$cc.controller = 1     // Set CC number (1 = modulation)
$cc.value = 64         // Set value (0-127) and send
```

### MIDI Sequences

```
$mseq = @midi_seq       // Create a MIDI sequence
$mseq.data = b10101010  // Set pattern
$mseq.midi_channel = 0  // Set MIDI channel
$mseq.note_base = 60    // Set base note (C4)
$mseq.midi_velocity = 100  // Set note velocity
$mseq.start()           // Start the sequence
```

## Running Reelia

### Keyboard Shortcuts

- `Ctrl+T`: Manually advance one tick
- `Ctrl+A`: Toggle auto-tick mode
- `Ctrl+S`: Change tick interval
- `Ctrl+M`: MIDI device configuration
- `Ctrl+L`: Clear screen
- `Ctrl+H` or `?`: Show help
- `Ctrl+D`: Dump variables
- `Ctrl+X`: Exit

## Building from Source

To build Reelia from source, you need a C++17 compatible compiler:

```bash
git clone https://github.com/your-username/reelia.git
cd reelia
make
./reelia_simulator
```

RtMidi library is automatically downloaded and integrated during the build process.

### OS-Specific Requirements

- **macOS**: No additional requirements
- **Linux**: ALSA development libraries (`sudo apt-get install libasound2-dev`)
- **Windows**: No additional requirements (uses Windows MIDI API)

## Examples

### Basic Pattern Sequence

```
$seq = @seq
$seq.data = b10101010
$seq.start()
```

### MIDI Drum Pattern

```
@midi.list
@midi.device = 0          // Select your MIDI device

$kick = @midi_note
$kick.channel = 9         // MIDI channel 10 (drums)
$kick.note = 36           // C1 = kick drum in GM
$kick.velocity = 100

$snare = @midi_note
$snare.channel = 9
$snare.note = 38          // D1 = snare drum in GM
$snare.velocity = 90

$drum = @seq
$drum.data = b10100000    // Kick-snare pattern
$drum.start()

// Tick handler for drum patterns
// When the drum seq has a "1", trigger the kick, otherwise trigger the snare
```

### MIDI Bass Line with Counter

```
$notes = @midi_seq
$notes.data = b10110010        // 8-step pattern
$notes.midi_channel = 0        // Synth channel
$notes.note_base = 36          // Start at C2
$notes.midi_velocity = 100

$cnt = @count                  // Counter for transposition
$cnt.min = 0
$cnt.max = 12
$cnt.step = 1
$cnt.start()

// Start the sequence
$notes.start() | $cnt.start()
```

### MIDI CC Automation

```
$cc1 = @midi_cc
$cc1.channel = 0
$cc1.controller = 1      // Modulation wheel
$cc1.value = 0           // Start value

$mod = @count
$mod.min = 0
$mod.max = 127
$mod.step = 5
$mod.start()

// Link the counter to the CC value
```

## Current Status

Reelia includes MIDI output functionality and is actively being developed. Future plans include:

- More sequence generation algorithms (Euclidean, etc.)
- Pattern transformation operations (rotate, mirror, etc.)
- External control via OSC
- Visual interface and pattern visualization
- MIDI clock synchronization
- Expanded set of generators and effects

## License

MIT

## Contributing

Contributions are welcome! Feel free to submit pull requests or open issues to improve Reelia.
