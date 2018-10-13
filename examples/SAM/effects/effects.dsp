// All effects used by minimoog.dsp

echo = component("../echo/echo.dsp");
flanger = component("../flanger/flanger.dsp");
chorus = component("../chorus/chorus.dsp");
freeverb = component("../freeverb/freeverb.dsp");

process = _,_ : + : echo : flanger : chorus : freeverb;
