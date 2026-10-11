"""Run the firmware's actual touch routines on a host; not an ESP32 build."""
from pathlib import Path
import ast
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class TouchInputsTest(unittest.TestCase):
    def test_pi_capabilities_enable_extra_buttons_and_readings(self):
        source = (ROOT / "mini-app/controller_core.py").read_text()
        tree = ast.parse(source)
        app = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == "HappyJarzApp")
        methods = [n for n in app.body if isinstance(n, ast.FunctionDef) and n.name in {"_apply_input_capabilities", "_parse_fields", "_handle_line"}]
        namespace = {"MUTED": "muted", "TEXT": "text", "time": __import__("time")}
        for method in methods:
            exec(compile(ast.Module(body=[method], type_ignores=[]), str(ROOT / "mini-app/controller_core.py"), "exec"), namespace)
        class Value:
            def __init__(self): self.value = "N/A"
            def get(self): return self.value
            def set(self, value): self.value = value
            def configure(self, **kwargs): self.__dict__.update(kwargs)
        class App:
            _apply_input_capabilities = namespace["_apply_input_capabilities"]
            _parse_fields = staticmethod(namespace["_parse_fields"])
            _handle_line = namespace["_handle_line"]
            def _log(self, line): pass
        app = App()
        app.input_names = ("up", "down", "left", "right", "a", "b", "x", "y", "l", "r", "start", "select")
        for field in ("input_name_labels", "input_value_labels", "mini_buttons", "touch_vars"):
            setattr(app, field, {key: Value() for key in app.input_names})
        app._handle_line("HJ|CAPS|controls=up,down,left,right,a,b,l,r,start")
        for key in ("l", "r", "start"): self.assertEqual(app.mini_buttons[key].state, "normal")
        for key in ("x", "y", "select"): self.assertEqual(app.mini_buttons[key].state, "disabled")
        app._handle_line("HJ|INPUT|l=111|r=112|start=113")
        self.assertEqual([app.touch_vars[key].get() for key in ("l", "r", "start")], ["111", "112", "113"])
        app._handle_line("HJ|CAPS|controls=up,down,left,right,a,b")
        for key in ("l", "r", "start"):
            self.assertEqual(app.mini_buttons[key].state, "disabled")
            self.assertEqual(app.touch_vars[key].get(), "N/A")

    def test_pi_refreshes_inputs_when_streaming_stalls(self):
        source = (ROOT / "mini-app/app.py").read_text()
        app_class = next(n for n in ast.parse(source).body if isinstance(n, ast.ClassDef) and n.name == "HappyJarzApp")
        method = next(n for n in app_class.body if isinstance(n, ast.FunctionDef) and n.name == "_poll_inputs")
        from types import SimpleNamespace
        namespace = {"time": SimpleNamespace(monotonic=lambda: 10.0)}
        exec(compile(ast.Module(body=[method], type_ignores=[]), "app.py", "exec"), namespace)
        class App:
            _poll_inputs = namespace["_poll_inputs"]
            def __init__(self, opened, last):
                self.sent = []
                self.scheduled = []
                self._last_input_received = last
                self.link = SimpleNamespace(ser=SimpleNamespace(is_open=opened), send=self.sent.append)
            def after(self, delay, callback):
                self.scheduled.append(delay)
        for opened, last, expected in [(True, 0, ["GET INPUT"]), (True, 9.8, []), (False, 0, [])]:
            with self.subTest(opened=opened, last=last):
                app = App(opened, last)
                app._poll_inputs()
                self.assertEqual(app.sent, expected)
                self.assertEqual(app.scheduled, [1000])

    def test_nine_pad_calibration_qualification_telemetry_and_events(self):
        source = (ROOT / "firmware/HappyJarzMini/HappyJarzMini.ino").read_text()
        pins = source[source.index("static constexpr uint8_t INPUT_A_PIN"):source.index("// -----------------------------\n// LED layer")]
        touch = source[source.index("enum InputIndex"):source.index("// -----------------------------\n// Settings / persistence")]
        helper = source[source.index("static void hjEmitExtraInputEvents"):source.index("static void serviceInputs()")]
        # All UI early returns come after extra-event emission. Original tail
        # excludes extras so those physical events are not emitted twice.
        routing = source[source.index("static void serviceInputs()"):source.index("// USB serial protocol")]
        self.assertLess(routing.index("hjEmitExtraInputEvents(q)"), routing.index("uiScreen == UI_GAMES"))
        self.assertIn("for (uint8_t i=0;i<IN_L;++i)", routing)
        stub = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <string>
#include <algorithm>
#include <sstream>
struct String : std::string {
  using std::string::string;
  void trim() { auto a=find_first_not_of(" \t\r\n"); auto b=find_last_not_of(" \t\r\n"); if(a==npos)clear(); else assign(substr(a,b-a+1)); }
  void toUpperCase() { for(auto &c:*this)c=::toupper(c); }
};
struct SerialMock {
  std::ostringstream text;
  template<class T> void print(T value){text<<value;}
  template<class T> void println(T value){text<<value<<'\n';}
  void println(){text<<'\n';}
  void clear(){text.str("");text.clear();}
} Serial;
static unsigned long nowMs=0;
static uint32_t raw[49];
unsigned long millis(){return nowMs;}
void delay(unsigned long ms){nowMs+=ms;}
uint32_t touchRead(uint8_t pin){return raw[pin];}
'''
        checks = r'''
int main(){
  static_assert(INPUT_COUNT==9);
  const uint8_t expected[]={4,5,9,10,1,2,11,12,13};
  for(size_t i=0;i<INPUT_COUNT;i++){assert(inputs[i].pin==expected[i]);raw[expected[i]]=10000+expected[i]*100;}
  calibrateInputs();
  assert(nowMs==1800);
  for(uint8_t i=0;i<INPUT_COUNT;i++){
    auto &p=inputs[i]; assert(p.baseline==raw[p.pin]);assert(p.threshold==p.baseline+p.baseline/5);
    assert(!updateInputState(i));
    raw[p.pin]=p.threshold; assert(!updateInputState(i));
    nowMs+=59;assert(!updateInputState(i));nowMs++;assert(updateInputState(i));
    nowMs+=100;assert(updateInputState(i));
    raw[p.pin]=p.baseline;assert(!updateInputState(i));assert(!p.touching&&!p.qualified);
    assert(p.enteredMs==0&&p.qualifiedMs==0);
  }
  assert(hjHostKeyIndex(" l ")==IN_L);assert(hjHostKeyIndex("R")==IN_R);assert(hjHostKeyIndex("start")==IN_START);
  assert(hjHostKeyIndex("SELECT")==-1);assert(hjHostKeyIndex("A")==IN_A);
  hjPrintCaps();assert(Serial.text.str().find("controls=up,down,left,right,a,b,l,r,start|")!=std::string::npos);
  Serial.clear();printInputTelemetry();auto line=Serial.text.str();
  for(const auto &p:inputs)assert(line.find(std::string("|")+p.name+"="+std::to_string(raw[p.pin]))!=std::string::npos);
  Serial.clear();printTouchCalibration();line=Serial.text.str();
  for(const auto &p:inputs){assert(line.find(std::string("|")+p.name+"_base=")!=std::string::npos);assert(line.find(std::string("|")+p.name+"_thr=")!=std::string::npos);}
  bool q[INPUT_COUNT]={};q[IN_L]=q[IN_R]=q[IN_START]=true;
  Serial.clear();hjEmitExtraInputEvents(q);
  assert(Serial.text.str()=="HJ|EVENT|input=l\nHJ|EVENT|input=r\nHJ|EVENT|input=start\n");
  for(size_t i=0;i<INPUT_COUNT;i++)latched[i]=q[i];Serial.clear();hjEmitExtraInputEvents(q);assert(Serial.text.str().empty());
  for(size_t i=0;i<INPUT_COUNT;i++)latched[i]=false;Serial.clear();hjEmitExtraInputEvents(q);assert(!Serial.text.str().empty());
}
'''
        with tempfile.TemporaryDirectory() as temp:
            cpp = Path(temp) / "touch.cpp"
            binary = Path(temp) / "touch"
            cpp.write_text(stub + pins + touch + "\nstatic bool latched[INPUT_COUNT] = {};\n" + helper + checks)
            subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", str(cpp), "-o", str(binary)], check=True, capture_output=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
