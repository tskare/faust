process = environment {
	import("../effects/layout2.dsp");
	import("virtualAnalog-code.dsp");
}.process;

echo = environment {
	//import("../echo/mypow2.dsp");
	import("../echo/mypow2ForBrowser.dsp");
	import("../effects/layout2.dsp");
	import("../echo/echo-code.dsp");
}.process;

flanger = environment {
	import("../effects/layout2.dsp");
	import("../flanger/flanger-code.dsp");
}.process;

chorus = environment {
	import("../effects/layout2.dsp");
	import("../chorus/chorus-code.dsp");
}.process;

freeverb = environment {
	import("../effects/layout2.dsp");
	import("../freeverb/freeverb-code.dsp");
}.process;

effect = _,_ : + : echo : flanger : chorus : freeverb;
