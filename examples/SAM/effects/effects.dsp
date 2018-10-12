// All effects used by minimoog.dsp

echo = environment {
	import("../echo/mypow2.dsp");
	import("layout2.dsp");
	import("../echo/echo-code.dsp");
}.process;

flanger = environment {
	import("layout2.dsp");
	import("../flanger/flanger-code.dsp");
}.process;

chorus = environment {
	import("layout2.dsp");
	import("../chorus/chorus-code.dsp");
}.process;

freeverb = environment {
	import("layout2.dsp");
	import("../freeverb/freeverb-code.dsp");
}.process;

process = _,_ : + : echo : flanger : chorus : freeverb;
