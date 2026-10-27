#pragma once
#include <cmath>
#include <cstddef>
#include <algorithm>
namespace juce {
class String { public: String()=default; String(const char*){}; };
struct AudioProcessorValueTreeState { struct ParameterLayout; };
template<typename T> struct MathConstants { static constexpr T pi = T(3.1415926535897932384626433832795); };
template<typename T> int roundToInt(T x) { return static_cast<int>(std::round(x)); }
template<typename T> T jlimit(T lo, T hi, T value) { return std::clamp(value,lo,hi); }
class MidiMessage {
public:
 enum Kind { On, Off, Controller, Invalid };
 static MidiMessage noteOn(int n,int v) { return {On,n,v}; }
 static MidiMessage noteOff(int n) { return {Off,n,0}; }
 static MidiMessage controllerEvent(int n) { return {Controller,n,0}; }
 bool isController() const { return kind==Controller; }
 int getControllerNumber() const { return note; }
 bool isNoteOn() const { return kind==On; }
 bool isNoteOff() const { return kind==Off; }
 int getNoteNumber() const { return note; }
 int getVelocity() const { return value; }
private:
 MidiMessage(Kind k,int n,int v):kind(k),note(n),value(v){}
 Kind kind; int note,value;
};
}
