// All effects used by minimoog.dsp

echo = component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/echo/echoForBrowser.dsp");
flanger = component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/flanger/flangerForBrowser.dsp");
chorus = component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/chorus/chorusForBrowser.dsp");
freeverb = component("https://raw.githubusercontent.com/grame-cncm/faust/master-dev/examples/SAM/freeverb/freeverbForBrowser.dsp");

process = _,_ : + : echo : flanger : chorus : freeverb;
