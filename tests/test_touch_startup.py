"""Exercise the real calibration functions with simulated startup transients."""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which("g++"), "g++ is needed for the calibration harness")
class TouchStartupTests(unittest.TestCase):
    def test_all_channels_settle_before_baselines_are_recorded(self):
        sketch = (ROOT / "firmware/HappyJarzMini/HappyJarzMini.ino").read_text()
        start = sketch.index("static void prepareTouchInputs()")
        end = sketch.index("static void printTouchCalibration()", start)
        functions = sketch[start:end]
        names = ["UP", "DOWN", "LEFT", "RIGHT", "A", "B", "L", "R", "START"]
        pins = [int(re.search(r"INPUT_" + n + r"_PIN = (\d+);", sketch).group(1)) for n in names]
        source = r"""
#include <cassert>
#include <cstdint>
#include <cstdio>
struct Pad { uint8_t pin; uint32_t baseline, threshold; bool touching, qualified;
             unsigned long enteredMs, qualifiedMs; };
unsigned long clockMs = 0;
unsigned long millis() { return clockMs; }
void delay(unsigned long ms) { clockMs += ms; }
unsigned counts[14] = {};
bool bHomeSent = true;
uint32_t readTouchPin(uint8_t pin) {
  ++counts[pin];
  return clockMs < 1000 ? 300000U : 10000U + pin;
}
Pad inputs[] = {PADS};
FUNCTIONS
int main() {
  calibrateInputs();
  assert(clockMs == 1000UL + 9UL * 40UL * 5UL);
  for (auto &pad : inputs) {
    assert(counts[pad.pin] == 1U + 200U + 40U);
    assert(pad.baseline == 10000U + pad.pin);
    assert(pad.threshold == pad.baseline + pad.baseline / 5U);
    assert(!pad.touching && !pad.qualified);
    assert(pad.enteredMs == 0 && pad.qualifiedMs == 0);
  }
  assert(!bHomeSent);
  // TEST INPUT uses the same preparation on a second calibration.
  calibrateInputs();
  for (auto &pad : inputs) assert(counts[pad.pin] == 2U * 241U);
  puts("Nine channels prepared; startup transients excluded; original thresholds retained.");
}
""".replace("PADS", ",".join("{" + str(pin) + ",0,0,true,true,99,99}" for pin in pins)).replace("FUNCTIONS", functions)
        with tempfile.TemporaryDirectory() as tmp:
            cpp, exe = Path(tmp) / "touch.cpp", Path(tmp) / "touch"
            cpp.write_text(source)
            subprocess.run(["g++", "-std=c++11", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

if __name__ == "__main__":
    unittest.main()
