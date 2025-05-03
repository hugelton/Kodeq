#ifndef KODEQ_MODULE_HPP
#define KODEQ_MODULE_HPP

#include <map>
#include <random>
#include <string>
#include <vector>

/**
 * Base Module class
 * Provides the interface for all KODEQ modules
 */
class Module {
public:
  virtual ~Module() {}

  // Get the current value of the module
  virtual int getValue() const = 0;

  // Set a parameter of the module
  virtual void setParameter(const std::string &name, int value) = 0;

  // Create a clone of the module
  virtual Module *clone() const = 0;

  // Get the module type name
  virtual std::string getType() const = 0;

  // Get a string representation of the module's state
  virtual std::string getVisualRepresentation() const = 0;
};

/**
 * PAT Module (Bit Pattern)
 * Generates patterns based on bit values in the pattern
 */
class PatternModule : public Module {
private:
  int pattern; // Bit pattern
  int index;   // Current index

public:
  PatternModule() : pattern(0), index(0) {}

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "PAT"; }
  std::string getVisualRepresentation() const override;
};

/**
 * EUC Module (Euclidean Rhythm)
 * Generates patterns based on Euclidean rhythm algorithm
 */
class EuclideanModule : public Module {
private:
  int hits;  // Number of hits
  int steps; // Number of steps
  int index; // Current index

public:
  EuclideanModule() : hits(0), steps(8), index(0) {}

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "EUC"; }
  std::string getVisualRepresentation() const override;
};

/**
 * SIN Module (Sine Wave)
 * Generates a sine wave pattern
 */
class SineModule : public Module {
private:
  int length; // Pattern length
  int pos;    // Current position
  int amp;    // Amplitude (0-127)

public:
  SineModule() : length(16), pos(0), amp(127) {}

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "SIN"; }
  std::string getVisualRepresentation() const override;
};

/**
 * TRI Module (Triangle Wave)
 * Generates a triangle wave pattern
 */
class TriangleModule : public Module {
private:
  int length; // Pattern length
  int pos;    // Current position
  int amp;    // Amplitude (0-127)

public:
  TriangleModule() : length(16), pos(0), amp(127) {}

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "TRI"; }
  std::string getVisualRepresentation() const override;
};

/**
 * SAW Module (Sawtooth Wave)
 * Generates a sawtooth wave pattern
 */
class SawtoothModule : public Module {
private:
  int length; // Pattern length
  int pos;    // Current position
  int amp;    // Amplitude (0-127)

public:
  SawtoothModule() : length(16), pos(0), amp(127) {}

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "SAW"; }
  std::string getVisualRepresentation() const override;
};

/**
 * SQR Module (Square Wave)
 * Generates a square wave pattern with variable duty cycle
 */
class SquareModule : public Module {
private:
  int length; // Pattern length
  int pos;    // Current position
  int amp;    // Amplitude (0-127)
  int duty;   // Duty cycle (0-100%)

public:
  SquareModule() : length(16), pos(0), amp(127), duty(50) {}

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "SQR"; }
  std::string getVisualRepresentation() const override;
};

/**
 * RND Module (Random Pattern)
 * Generates random patterns based on probability
 */
class RandomModule : public Module {
private:
  int probability; // Probability of 1 (0-100%)
  int seed;        // Random seed
  int length;      // Pattern length
  int pos;         // Current position

  std::mt19937 rng;          // Random number generator
  bool regenerateOnCycle;    // Whether to regenerate on each cycle
  std::vector<bool> pattern; // Pre-generated pattern

public:
  RandomModule()
      : probability(50), seed(0), length(16), pos(0), regenerateOnCycle(true) {
    rng.seed(seed);
    generatePattern();
  }

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "RND"; }
  std::string getVisualRepresentation() const override;

private:
  void generatePattern();
};

/**
 * SEQ Module (Sequencer)
 * Stores and plays back a sequence of values
 */
class SequencerModule : public Module {
private:
  std::vector<int> steps; // Sequence steps (up to 16 steps)
  int pos;                // Current position
  int length;             // Sequence length
  bool looping;           // Whether to loop

public:
  SequencerModule() : pos(0), length(8), looping(true) {
    // Create default 8-step sequence with 0 values
    steps.resize(16, 0);
  }

  int getValue() const override;
  void setParameter(const std::string &name, int value) override;
  Module *clone() const override;
  std::string getType() const override { return "SEQ"; }
  std::string getVisualRepresentation() const override;

  // Set step value
  void setStep(int index, int value);
  int getStep(int index) const;
};

/**
 * Module Factory
 * Creates modules by type name
 */
class ModuleFactory {
public:
  static Module *createModule(const std::string &type);
};

#endif // KODEQ_MODULE_HPP