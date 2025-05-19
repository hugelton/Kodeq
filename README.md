# Reelia

Reelia is a live-coding environment for sequence generation with a focus on MIDI output and real-time performance. It's designed with a clean, intuitive syntax that makes it easy to create and manipulate patterns during live performances.

## Features

- **Intuitive Syntax**: Simple and clean syntax inspired by modern programming languages
- **Object-Oriented Design**: Create and manipulate sequence objects with attributes and methods
- **Real-Time Performance**: Tick-based system with automatic and manual control
- **Pattern Generation**: Binary patterns, counters, and sequencers
- **Parallel Execution**: Run multiple commands simultaneously with pipe syntax

## Syntax Overview

Reelia features a clean, expressive syntax:

### 1. Object Creation

```
$seq = @seq     // Create a sequence object
$cnt = @count   // Create a counter object
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
```

### 4. Parallel Execution

```
$seq.start() | $cnt.start()   // Start both objects in parallel
```

## Running Reelia

### Keyboard Shortcuts

- `Ctrl+T`: Manually advance one tick
- `Ctrl+A`: Toggle auto-tick mode
- `Ctrl+S`: Change tick interval
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

## Examples

### Basic Pattern Sequence

```
$seq = @seq
$seq.data = b10101010
$seq.start()
```

### Counter with Custom Range

```
$cnt = @count
$cnt.min = 0
$cnt.max = 7
$cnt.step = 1
$cnt.start()
```

### Pattern Manipulation

```
$pat = @binary
$pat = b10110010
$seq = @seq
$seq.data = $pat
$seq.start()
```

### Parallel Sequences

```
$drum = @seq
$bass = @seq
$drum.data = b10100101
$bass.data = b10010010
$drum.start() | $bass.start()
```

## Current Status

Reelia is currently in active development. Future plans include:

- MIDI output implementation
- More sequence generation algorithms (Euclidean, etc.)
- Pattern transformation operations (rotate, mirror, etc.)
- External control via OSC/MIDI
- Visual interface

## License

MIT

## Contributing

Contributions are welcome! Feel free to submit pull requests or open issues to improve Reelia.
