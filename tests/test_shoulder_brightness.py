"""Execute the firmware's actual shoulder-key handler on a host compiler."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class ShoulderBrightnessTest(unittest.TestCase):
    def test_edges_bounds_save_and_status(self):
        source = (ROOT / "firmware/HappyJarzMini/HappyJarzMini.ino").read_text()
        handler = source[source.index("static void hjServiceShoulderBrightness"):source.index("// Extra controls report independent")]
        routing = source[source.index("static void serviceInputs()"):source.index("// USB serial protocol")]
        self.assertLess(routing.index("hjServiceShoulderBrightness(q)"), routing.index("uiScreen == UI_GAMES"))
        self.assertLess(routing.index("printInputTelemetry()"), routing.index("return;"))
        cpp = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <algorithm>
enum { IN_L=6, IN_R=7, IN_START=8, INPUT_COUNT=9 };
static bool latched[INPUT_COUNT]={};
static uint8_t brightnessPercent=25;
static bool oledDirty=false;
static int saves=0, statusUpdates=0;
template<class T> T constrain(T x,T lo,T hi){return std::min(hi,std::max(lo,x));}
void hjSetBrightness(uint8_t p){brightnessPercent=p;}
void persistSettings(){saves++;}
std::string statusLine(){return "HJ|STATUS|brightness="+std::to_string(brightnessPercent);}
struct {void println(const std::string &s){assert(s.find("HJ|STATUS|brightness=")==0);statusUpdates++;}} Serial;
''' + handler + r'''
int main(){
 bool q[INPUT_COUNT]={};
 q[IN_L]=true;hjServiceShoulderBrightness(q);
 assert(brightnessPercent==20 && saves==1 && statusUpdates==1 && oledDirty);
 latched[IN_L]=true;hjServiceShoulderBrightness(q);assert(saves==1);
 q[IN_L]=false;latched[IN_L]=false;q[IN_R]=true;
 hjServiceShoulderBrightness(q);assert(brightnessPercent==25 && saves==2);
 brightnessPercent=50;hjServiceShoulderBrightness(q);assert(brightnessPercent==50 && saves==2);
 q[IN_R]=false;q[IN_L]=true;brightnessPercent=0;
 hjServiceShoulderBrightness(q);assert(brightnessPercent==0 && saves==2);
 q[IN_R]=true;brightnessPercent=25;hjServiceShoulderBrightness(q);
 assert(brightnessPercent==25 && saves==2);
 q[IN_L]=false;q[IN_R]=false;q[IN_START]=true;
 hjServiceShoulderBrightness(q);assert(brightnessPercent==25 && saves==2);
}
'''
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"shoulders.cpp"; exe=Path(d)/"shoulders"
            p.write_text(cpp)
            subprocess.run(["g++","-std=c++17",str(p),"-o",str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

if __name__ == "__main__":
    unittest.main()
