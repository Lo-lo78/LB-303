// TEST2.4.41: compile against the real SequencerState.cpp and the small JUCE shim.
#include "SequencerState.h"
#include <cstdlib>
#include <iostream>
#include <memory>
using namespace lb303;
// CHECK must evaluate its expression in Release builds too. Standard CHECK()
// is compiled out under NDEBUG; it previously skipped required Base64
// decoding/restoring and caused an access violation in GitHub Actions.
#define CHECK(expression) do { \
    if (!(expression)) { \
        std::cerr << "FAIL: " << #expression << " (line " << __LINE__ << ")\n"; \
        return EXIT_FAILURE; \
    } \
} while (false)
static void selectVelocity(SequencerState& bank, int pat,int i){
 const auto current=bank.getStep(pat,i);
 bank.setStepValue(pat,i,SequencerLayer::velocity,
                   static_cast<float>(current.velocity==127?current.velocityBeforeMax:127));
}
static void selectNote(SequencerState& bank,int pat,int i){
 auto step=bank.getStep(pat,i);
 bank.setStepValue(pat,i,SequencerLayer::note,static_cast<float>(step.note==128?step.noteBeforePause:128));
}
static void selectLength(SequencerState& bank,int pat,int i){
 auto step=bank.getStep(pat,i);
 bank.setStepValue(pat,i,SequencerLayer::length,static_cast<float>(step.length==101?step.lengthBeforeLegato:101));
}
int main(){
 auto bankStorage=std::make_unique<SequencerState>(); auto& bank=*bankStorage;
 // Distinct steps, also the boundary values 0 and 126.
 const int before[]={85, 46, 0, 126};
 for(int i=0;i<4;++i)bank.setStepValue(0,i,SequencerLayer::velocity,static_cast<float>(before[i]));
 for(int i=0;i<4;++i){selectVelocity(bank,0,i);CHECK(bank.getStep(0,i).velocity==127);}
 for(int i=0;i<4;++i){CHECK(bank.getStep(0,i).velocityBeforeMax==before[i]);}
 selectVelocity(bank,0,0);CHECK(bank.getStep(0,0).velocity==85);
 selectVelocity(bank,0,0);CHECK(bank.getStep(0,0).velocity==127);
 // Notes and lengths retain their previously selected data.
 bank.setStepValue(0,0,SequencerLayer::note,72);selectNote(bank,0,0);selectNote(bank,0,0);
 CHECK(bank.getStep(0,0).note==72);
 bank.setStepValue(0,0,SequencerLayer::length,37);selectLength(bank,0,0);selectLength(bank,0,0);
 CHECK(bank.getStep(0,0).length==37);
 // Each pattern has its own memory.
 bank.setStepValue(1,0,SequencerLayer::velocity,52);selectVelocity(bank,1,0);
 CHECK(bank.getStep(1,0).velocity==127);
 CHECK(bank.getStep(0,0).velocityBeforeMax==85);
 // Copy, Undo-style replace, and new-format serialization all preserve memory.
 auto copyStorage=std::make_unique<SequencerState>(); auto& copy=*copyStorage;
 copy.copySequenceFrom(bank,0,4);
 CHECK(copy.getStep(4,0).velocityBeforeMax==85);
 auto saved=bank.getStep(0,0);
 bank.resetStep(0,0);bank.replaceStep(0,0,saved);
 CHECK(bank.getStep(0,0).velocityBeforeMax==85);
 auto restoredStorage=std::make_unique<SequencerState>(); auto& restored=*restoredStorage;
 CHECK(restored.restoreFromBase64(bank.serialiseToBase64()));
 for(int i=0;i<4;++i){CHECK(restored.getStep(0,i).velocityBeforeMax==before[i]);}
 selectVelocity(restored,0,2);CHECK(restored.getStep(0,2).velocity==0);
 selectVelocity(restored,1,0);CHECK(restored.getStep(1,0).velocity==52);
 // Rebuild an old v6 sequence from the v7 snapshot by removing the one
 // extra byte per step, retaining the rest of the binary layout unchanged.
 juce::MemoryOutputStream decoded;
 CHECK(juce::Base64::convertFromBase64(decoded, bank.serialiseToBase64()));
 const auto& src=decoded.bytes;
 juce::MemoryOutputStream old;
 CHECK(src.size()>16);
 old.write(src.data(), 4); old.writeInt(6); old.write(src.data()+8, 8);
 size_t offset=16;
 for(int pat=0;pat<16;++pat){
  // 21 four-byte fields and one boolean in the pattern configuration.
  constexpr size_t configBytes=21*4+1;
  CHECK(offset+configBytes<=src.size());
  old.write(src.data()+offset,configBytes);offset+=configBytes;
  for(int i=0;i<SequencerState::stepsPerSequence;++i){
   CHECK(offset+16<=src.size());
   old.write(src.data()+offset,10);offset+=10;
   offset+=5; // discard v10 velocity memory, two send states, two send levels.
   const auto count=src[offset];
   old.write(src.data()+offset,1);++offset;
   CHECK(offset+count*8<=src.size());
   old.write(src.data()+offset,count*8);offset+=count*8;
  }
 }
 CHECK(offset<src.size()); // v6 had 16 patterns; v10 stores 64.
 auto legacyStorage=std::make_unique<SequencerState>(); auto& legacy=*legacyStorage;
 CHECK(legacy.restoreFromBase64(juce::Base64::toBase64(old.getData(),old.getDataSize())));
 CHECK(legacy.getStep(0,1).velocity==127);
 CHECK(legacy.getStep(0,1).velocityBeforeMax==100); // no old memory stored.
 selectVelocity(legacy,0,1);CHECK(legacy.getStep(0,1).velocity==100);
 CHECK(legacy.getStep(0,0).velocityBeforeMax==100); // already 127 in old save.
 CHECK(legacy.getStep(1,0).velocityBeforeMax==100);
 std::cout<<"PASS: Velocity, Note, Length, copy, replace, v10 saves, and legacy v6 load\n";
}
