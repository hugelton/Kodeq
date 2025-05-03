#include "module.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

//------------------------------------------------------------------------------
// PAT Module Implementation
//------------------------------------------------------------------------------

int PatternModule::getValue() const {
  // Get the value of the bit at the current index position
  return (pattern >> (index % 32)) & 1;
}

void PatternModule::setParameter(const std::string &name, int value) {
  if (name == "P") {
    // Set pattern value
    pattern = value;
  } else if (name == "I") {
    // Set index/position
    index = value;
  }
}

Module *PatternModule::clone() const {
  PatternModule *clone = new PatternModule();
  clone->pattern = this->pattern;
  clone->index = this->index;
  return clone;
}

std::string PatternModule::getVisualRepresentation() const {
  std::string rep = "Pattern: ";

  // Binary representation (most significant bit first)
  for (int i = 7; i >= 0; i--) {
    rep += ((pattern >> i) & 1) ? "1" : "0";
  }

  rep += "\nIndex: " + std::to_string(index % 8);
  rep += "\nCurrent Bit: " + std::to_string((pattern >> (index % 8)) & 1);

  // Pattern visualization (LSB on left, MSB on right)
  rep += "\n[";
  for (int i = 0; i < 8; i++) {
    if (i == index % 8) {
      rep += ((pattern >> i) & 1) ? "*" : ".";
    } else {
      rep += ((pattern >> i) & 1) ? "o" : "-";
    }
  }
  rep += "]";

  return rep;
}

//------------------------------------------------------------------------------
// EUC Module Implementation (Euclidean Rhythm)
//------------------------------------------------------------------------------

int EuclideanModule::getValue() const {
  if (steps == 0)
    return 0;
  if (hits >= steps)
    return 1;
  if (hits == 0)
    return 0;

  // Euclidean rhythm calculation
  return ((index * hits) % steps) < hits ? 1 : 0;
}

void EuclideanModule::setParameter(const std::string &name, int value) {
  if (name == "K") {
    // Set hits (K)
    hits = std::max(0, value);
  } else if (name == "N") {
    // Set steps (N)
    steps = std::max(1, value); // Prevent division by zero
  } else if (name == "I") {
    // Set index/position
    index = value;
  }
}

Module *EuclideanModule::clone() const {
  EuclideanModule *clone = new EuclideanModule();
  clone->hits = this->hits;
  clone->steps = this->steps;
  clone->index = this->index;
  return clone;
}

// Euclidean rhythm module visual representation
std::string EuclideanModule::getVisualRepresentation() const {
  std::string rep =
      "Euclidean: " + std::to_string(hits) + "/" + std::to_string(steps);
  rep += "\nIndex: " + std::to_string(index % steps);

  // Current value
  bool current = ((index % steps) * hits) % steps < hits;
  rep += "\nCurrent Value: " + std::to_string(current ? 1 : 0);

  // Pattern visualization
  rep += "\n[";
  for (int i = 0; i < steps; i++) {
    if (i == index % steps) {
      rep += ((i * hits) % steps < hits) ? "*" : ".";
    } else {
      rep += ((i * hits) % steps < hits) ? "o" : "-";
    }
  }
  rep += "]";

  return rep;
}

//------------------------------------------------------------------------------
// SIN Module Implementation (Sine Wave)
//------------------------------------------------------------------------------

int SineModule::getValue() const {
  // Simple sine wave lookup table (0-255)
  static const int sinTable[16] = {128, 176, 218, 245, 255, 245, 218, 176,
                                   128, 80,  38,  11,  0,   11,  38,  80};

  int idx = (pos % length) * 16 / length;
  return 128 + ((sinTable[idx] - 128) * amp) / 127;
}

void SineModule::setParameter(const std::string &name, int value) {
  if (name == "LEN") {
    length = std::max(1, value);
  } else if (name == "POS") {
    pos = value;
  } else if (name == "A") {
    amp = std::min(127, std::max(0, value));
  }
}

Module *SineModule::clone() const {
  SineModule *clone = new SineModule();
  clone->length = this->length;
  clone->pos = this->pos;
  clone->amp = this->amp;
  return clone;
}

// Sine wave module visual representation
std::string SineModule::getVisualRepresentation() const {
  std::string rep = "Sine Wave";
  rep += "\nLength: " + std::to_string(length);
  rep += "\nPosition: " + std::to_string(pos % length);
  rep += "\nAmplitude: " + std::to_string(amp);

  // Waveform simple display
  rep += "\n";
  int value = getValue();
  int normalized = (value - 128) * 10 / 128; // Normalize to -10 to +10 range

  for (int y = 10; y >= -10; y--) {
    if (y == 0) {
      rep += "|";
    } else if (y == normalized) {
      rep += "*";
    } else {
      rep += " ";
    }
  }

  return rep;
}

//------------------------------------------------------------------------------
// TRI Module Implementation (Triangle Wave)
//------------------------------------------------------------------------------

int TriangleModule::getValue() const {
  int normalizedPos = (pos % length) * 256 / length;
  int halfCycle = 128;
  int value;

  if (normalizedPos < halfCycle) {
    // Rising
    value = normalizedPos * 255 / halfCycle;
  } else {
    // Falling
    value = 255 - ((normalizedPos - halfCycle) * 255 / halfCycle);
  }

  return 128 + ((value - 128) * amp) / 127;
}

void TriangleModule::setParameter(const std::string &name, int value) {
  if (name == "LEN") {
    length = std::max(1, value);
  } else if (name == "POS") {
    pos = value;
  } else if (name == "A") {
    amp = std::min(127, std::max(0, value));
  }
}

Module *TriangleModule::clone() const {
  TriangleModule *clone = new TriangleModule();
  clone->length = this->length;
  clone->pos = this->pos;
  clone->amp = this->amp;
  return clone;
}

std::string TriangleModule::getVisualRepresentation() const {
  std::string rep = "Triangle Wave";
  rep += "\nLength: " + std::to_string(length);
  rep += "\nPosition: " + std::to_string(pos % length);
  rep += "\nAmplitude: " + std::to_string(amp);

  // Waveform simple display
  rep += "\n";
  int value = getValue();
  int normalized = (value - 128) * 10 / 128; // Normalize to -10 to +10 range

  for (int y = 10; y >= -10; y--) {
    if (y == 0) {
      rep += "-";
    } else if (y == normalized) {
      rep += "*";
    } else {
      rep += " ";
    }
  }

  return rep;
}

//------------------------------------------------------------------------------
// SAW Module Implementation (Sawtooth Wave)
//------------------------------------------------------------------------------

int SawtoothModule::getValue() const {
  int value = (pos % length) * 255 / length;
  return 128 + ((value - 128) * amp) / 127;
}

void SawtoothModule::setParameter(const std::string &name, int value) {
  if (name == "LEN") {
    length = std::max(1, value);
  } else if (name == "POS") {
    pos = value;
  } else if (name == "A") {
    amp = std::min(127, std::max(0, value));
  }
}

Module *SawtoothModule::clone() const {
  SawtoothModule *clone = new SawtoothModule();
  clone->length = this->length;
  clone->pos = this->pos;
  clone->amp = this->amp;
  return clone;
}

// Sawtooth wave module visual representation
std::string SawtoothModule::getVisualRepresentation() const {
  std::string rep = "Sawtooth Wave";
  rep += "\nLength: " + std::to_string(length);
  rep += "\nPosition: " + std::to_string(pos % length);
  rep += "\nAmplitude: " + std::to_string(amp);

  // Waveform simple display (horizontal)
  rep += "\n";
  int normalizedPos = (pos % length) * 16 / length;

  for (int i = 0; i < 16; i++) {
    if (i == normalizedPos) {
      rep += "X"; // Current position
    } else {
      int height = i * 8 / 16;       // 0-7 height
      rep += std::to_string(height); // Height as number
    }
  }

  // Vertical waveform display
  rep += "\n";
  for (int y = 7; y >= 0; y--) {
    for (int x = 0; x < 16; x++) {
      int height = x * 8 / 16;
      if (x == normalizedPos) {
        rep += "X"; // Current position
      } else if (height == y) {
        rep += "*"; // Waveform
      } else {
        rep += " ";
      }
    }
    rep += "\n";
  }

  return rep;
}

//------------------------------------------------------------------------------
// SQR Module Implementation (Square Wave)
//------------------------------------------------------------------------------

int SquareModule::getValue() const {
  int normalizedPos = (pos % length) * 100 / length;
  int value = (normalizedPos < duty) ? 255 : 0;
  return 128 + ((value - 128) * amp) / 127;
}

void SquareModule::setParameter(const std::string &name, int value) {
  if (name == "LEN") {
    length = std::max(1, value);
  } else if (name == "POS") {
    pos = value;
  } else if (name == "A") {
    amp = std::min(127, std::max(0, value));
  } else if (name == "D") {
    duty = std::min(100, std::max(0, value));
  }
}

Module *SquareModule::clone() const {
  SquareModule *clone = new SquareModule();
  clone->length = this->length;
  clone->pos = this->pos;
  clone->amp = this->amp;
  clone->duty = this->duty;
  return clone;
}

// Square wave module visual representation
std::string SquareModule::getVisualRepresentation() const {
  std::string rep = "Square Wave";
  rep += "\nLength: " + std::to_string(length);
  rep += "\nPosition: " + std::to_string(pos % length);
  rep += "\nAmplitude: " + std::to_string(amp);
  rep += "\nDuty Cycle: " + std::to_string(duty) + "%";

  // Waveform simple display
  rep += "\n";
  int value = getValue();
  int normalized = (value - 128) * 10 / 128; // Normalize to -10 to +10 range

  for (int y = 10; y >= -10; y--) {
    if (y == 0) {
      rep += "-";
    } else if (y == normalized) {
      rep += "*";
    } else {
      rep += " ";
    }
  }

  return rep;
}

//------------------------------------------------------------------------------
// RND Module Implementation (Random Pattern)
//------------------------------------------------------------------------------

void RandomModule::generatePattern() {
  pattern.clear();
  pattern.resize(length);

  std::uniform_int_distribution<int> dist(1, 100);
  for (int i = 0; i < length; i++) {
    pattern[i] = (dist(rng) <= probability);
  }
}

int RandomModule::getValue() const {
  if (pos % length == 0 && regenerateOnCycle && pos > 0) {
    // We need to regenerate the pattern, but can't modify state in a const
    // method So we cast away const for this operation
    RandomModule *self = const_cast<RandomModule *>(this);
    self->generatePattern();
  }

  return pattern[pos % length] ? 1 : 0;
}

void RandomModule::setParameter(const std::string &name, int value) {
  if (name == "P") {
    probability = std::min(100, std::max(0, value));
    generatePattern(); // Regenerate pattern when probability changes
  } else if (name == "LEN") {
    length = std::max(1, value);
    generatePattern(); // Regenerate pattern when length changes
  } else if (name == "POS") {
    pos = value;
  } else if (name == "SEED") {
    seed = value;
    rng.seed(seed);
    generatePattern(); // Regenerate pattern when seed changes
  } else if (name == "REGEN") {
    regenerateOnCycle = (value != 0);
  }
}

Module *RandomModule::clone() const {
  RandomModule *clone = new RandomModule();
  clone->probability = this->probability;
  clone->seed = this->seed;
  clone->length = this->length;
  clone->pos = this->pos;
  clone->regenerateOnCycle = this->regenerateOnCycle;
  clone->pattern = this->pattern;
  clone->rng.seed(this->seed);
  return clone;
}

std::string RandomModule::getVisualRepresentation() const {
  std::string rep = "Random Generator";
  rep += "\nProbability: " + std::to_string(probability) + "%";
  rep += "\nLength: " + std::to_string(length);
  rep += "\nPosition: " + std::to_string(pos % length);
  rep += "\nSeed: " + std::to_string(seed);
  rep += "\nRegenerate: " + std::string(regenerateOnCycle ? "Yes" : "No");

  // Pattern visualization
  rep += "\n[";
  for (int i = 0; i < length; i++) {
    if (i == pos % length) {
      rep += pattern[i] ? "*" : ".";
    } else {
      rep += pattern[i] ? "o" : "-";
    }
  }
  rep += "]";

  return rep;
}

//------------------------------------------------------------------------------
// SEQ Module Implementation (Sequencer)
//------------------------------------------------------------------------------

int SequencerModule::getValue() const {
  if (pos >= length && !looping) {
    return 0; // Return 0 when outside range if not looping
  }
  return steps[pos % length];
}

void SequencerModule::setParameter(const std::string &name, int value) {
  if (name == "POS") {
    pos = value;
  } else if (name == "LEN") {
    length = std::min(16, std::max(1, value));
  } else if (name == "LOOP") {
    looping = (value != 0);
  } else if (name.size() > 1 && name[0] == 'S') {
    // S1-S16 step value settings
    try {
      int stepIdx = std::stoi(name.substr(1)) - 1; // S1 is index 0
      if (stepIdx >= 0 && stepIdx < 16) {
        steps[stepIdx] = value;
      }
    } catch (...) {
      // Parse failed, do nothing
    }
  }
}

Module *SequencerModule::clone() const {
  SequencerModule *clone = new SequencerModule();
  clone->steps = this->steps;
  clone->pos = this->pos;
  clone->length = this->length;
  clone->looping = this->looping;
  return clone;
}

void SequencerModule::setStep(int index, int value) {
  if (index >= 0 && index < 16) {
    steps[index] = value;
  }
}

int SequencerModule::getStep(int index) const {
  if (index >= 0 && index < 16) {
    return steps[index];
  }
  return 0;
}

std::string SequencerModule::getVisualRepresentation() const {
  std::string rep = "Sequencer";
  rep += "\nLength: " + std::to_string(length);
  rep += "\nPosition: " + std::to_string(pos % length);
  rep += "\nLooping: " + std::string(looping ? "Yes" : "No");

  // Step values visualization
  rep += "\nSteps:";
  for (int i = 0; i < length; i++) {
    rep += " " + std::to_string(steps[i]);
  }

  // Current position indicator
  rep += "\n";
  for (int i = 0; i < length; i++) {
    rep += (i == pos % length) ? "^ " : "  ";
  }

  return rep;
}

//------------------------------------------------------------------------------
// Module Factory Implementation
//------------------------------------------------------------------------------

Module *ModuleFactory::createModule(const std::string &type) {
  if (type == "PAT") {
    return new PatternModule();
  } else if (type == "EUC") {
    return new EuclideanModule();
  } else if (type == "SIN") {
    return new SineModule();
  } else if (type == "TRI") {
    return new TriangleModule();
  } else if (type == "SAW") {
    return new SawtoothModule();
  } else if (type == "SQR") {
    return new SquareModule();
  } else if (type == "RND") {
    return new RandomModule();
  } else if (type == "SEQ") {
    return new SequencerModule();
  }

  // Return nullptr for unknown types
  return nullptr;
}